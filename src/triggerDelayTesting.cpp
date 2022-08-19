/* DO NOT USE FILE. THIS FILE IS STILL IN PRODUCTION


File written by Jonah Ezekiel (jezekiel@stanford.edu), with many segments copied
from parts of the spinnaker SDK examples. Program allows user to for testing of trigger
delay between trigger and beginnig of acquisition for Spinnaker Blackfly Camera S model
cameras by use of the Spinnaker SDK API to software trigger the cameras and GPIO output
from the cameras

See function comments for more details.*/

#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"
#include <iostream>
#include <sstream>
#include <typeinfo>
#include <ctime>
#include <chrono>
#include <string>
#include "include/features.cpp"

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;
using namespace std;

using std::chrono::duration_cast;
using std::chrono::nanoseconds;
typedef std::chrono::high_resolution_clock clock;

FILE *logFile;
#define pin 20

#ifdef _DEBUG
// Disables heartbeat on GEV cameras so debugging does not incur timeout errors

int DisableHeartbeat(INodeMap& nodeMap, INodeMap& nodeMapTLDevice)
{
    cout << "Checking device type to see if we need to disable the camera's heartbeat..." << endl << endl;

    //
    // Write to boolean node controlling the camera's heartbeat
    //
    // *** NOTES ***
    // This applies only to GEV cameras and only applies when in DEBUG mode.
    // GEV cameras have a heartbeat built in, but when debugging applications the
    // camera may time out due to its heartbeat. Disabling the heartbeat prevents
    // this timeout from occurring, enabling us to continue with any necessary debugging.
    // This procedure does not affect other types of cameras and will prematurely exit
    // if it determines the device in question is not a GEV camera.
    //
    // *** LATER ***
    // Since we only disable the heartbeat on GEV cameras during debug mode, it is better
    // to power cycle the camera after debugging. A power cycle will reset the camera
    // to its default settings.
    //
    CEnumerationPtr ptrDeviceType = nodeMapTLDevice.GetNode("DeviceType");
    if (!IsAvailable(ptrDeviceType) || !IsReadable(ptrDeviceType))
    {
        cout << "Error with reading the device's type. Aborting..." << endl << endl;
        return -1;
    }
    else
    {
        if (ptrDeviceType->GetIntValue() == DeviceType_GigEVision)
        {
            cout << "Working with a GigE camera. Attempting to disable heartbeat before continuing..." << endl << endl;
            CBooleanPtr ptrDeviceHeartbeat = nodeMap.GetNode("GevGVCPHeartbeatDisable");
            if (!IsAvailable(ptrDeviceHeartbeat) || !IsWritable(ptrDeviceHeartbeat))
            {
                cout << "Unable to disable heartbeat on camera. Continuing with execution as this may be non-fatal..."
                     << endl
                     << endl;
            }
            else
            {
                ptrDeviceHeartbeat->SetValue(true);
                cout << "WARNING: Heartbeat on GigE camera disabled for the rest of Debug Mode." << endl;
                cout << "         Power cycle camera when done debugging to re-enable the heartbeat..." << endl << endl;
            }
        }
        else
        {
            cout << "Camera does not use GigE interface. Resuming normal execution..." << endl << endl;
        }
    }
    return 0;
}
#endif

/* Called by run camera function. Takes as input camera, camera nodeMap, and cameras
nodeMapTLDevice. Acquires a single image and saves this image in the specified directory
with the name Acquisition-(Date & time).jpg. Works under assumption that camera is in
Acquisition mode. */
int getImage(CameraPtr pCam, INodeMap& nodeMap, INodeMap& nodeMapTLDevice) {
    cout << "Getting Image" << endl;
    auto t = clock::now();
    ImagePtr pResultImage = pCam->GetNextImage(1000);

    if (pResultImage->IsIncomplete()) {
        cout << "Image incomplete: " << Image::GetImageStatusDescription(pResultImage->GetImageStatus()) << endl;
        return -1;
    } else {
        //optionally convert image at this point to different format
        ostringstream filename;
    time_t now = time(0);
    filename << "/home/pi/magis/data/DIS/lab/";
        filename << "Acquisition-";
        filename << ctime(&now);
        filename << ".raw";
        cout << "Saving image" << endl;
        pResultImage->Save(filename.str().c_str());
        cout << "Image saved" << endl;
        pResultImage->Release();
        return 0;
    }
}

/* Function called by main in case where only one camera connected. Function calls PrintDeviceInfo
and setAcquisitionMode, setting up the camera and generates cli allowing user to take photos via
function getImages, change exposure time via function setExposureTime, change adc bit depth via function setADCBitDepth, and change shutter mode via function setShutterMode */
int runSingleCamera(CameraPtr pCam, SystemPtr system, CameraList camList) {
    INodeMap& nodeMapTLDevice = pCam->GetTLDeviceNodeMap();
    printDeviceInfo(nodeMapTLDevice);
    pCam->Init();
    INodeMap& nodeMap = pCam->GetNodeMap();
    set("AcquisitionMode", pCam, nodeMap, nodeMapTLDevice, "SingleFrame");
    setPixelFormat(pCam, nodeMap, nodeMapTLDevice, 16);

#ifdef _DEBUG
        cout << endl << endl << "*** DEBUG ***" << endl << endl;

        // If using a GEV camera and debugging, should disable heartbeat first to prevent further issues
        if (DisableHeartbeat(nodeMap, nodeMapTLDevice) != 0)
        {
            return -1;
        }

        cout << endl << endl << "*** END OF DEBUG ***" << endl << endl;
#endif

    for(int i = 0; i < numDataPoints; i++) {
        pCam->BeginAcquisition();
        getImage(pCam, nodeMap, nodeMapTLDevice);
        pCam->EndAcquisition();
    }

}


/* Main function takes as input no arguments. Determines number of cameras and calls corresponding run
camera(s) function. */
int main(int /*argc*/, char** /*argv*/) {
    logFile = fopen("triggerDelay-" + to_string(ctime(&now)) + ".txt", "w+");
    // Print application build information
    cout << "Application build date: " << __DATE__ << " " << __TIME__ << endl << endl;

    // Retrieve singleton reference to system object
    SystemPtr system = System::GetInstance();

    // Print out current library version
    const LibraryVersion spinnakerLibraryVersion = system->GetLibraryVersion();
    cout << "Spinnaker library version: " << spinnakerLibraryVersion.major << "." << spinnakerLibraryVersion.minor
         << "." << spinnakerLibraryVersion.type << "." << spinnakerLibraryVersion.build << endl
         << endl;

    CameraList camList = system->GetCameras();

    const unsigned int numCameras = camList.GetSize();

    cout << "Number of cameras detected: " << numCameras << endl << endl;

    // Finish if there are no cameras
    if (numCameras != 1) {
        // Clear camera list before releasing system
        camList.Clear();

        // Release system
        system->ReleaseInstance();
        fclose(logFile);
        cout << "Not enough or too many cameras! Press Enter to exit." << endl;
        getchar();
        return -1;
    } 

    CameraPtr pCam = camList.GetByIndex(0);
    runSingleCamera(pCam, system, camList);
    pCam = nullptr;
    camList.Clear();
    system->ReleaseInstance();
    fclose(logFile);
    return 0;
}




















