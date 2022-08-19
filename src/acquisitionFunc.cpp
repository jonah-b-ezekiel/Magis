/* File written by Jonah Ezekiel (jezekiel@stanford.edu), with many segments copied
from parts of the spinnaker SDK examples. Program allows user to test out key
functionality of working with the Spinnaker SDK API to interface with Flir
Blackfly s cameras over software. In running takes no inputs, but generates a
CLI in order to trigger the cameras, and between photos optionally
modify the exposure time, adc bit depth, shutter mode, etc. Photo files stored in /magis/data/DIS/lab_images.
Please make sure that this directory is created before running this file. For reference, the location of
this file is /magis/MAGIS_DIS/src/acquisitionFunc.cpp

Call order in case of one camera detected:

main calls runSingleCamera
	runSingleCamera calls PrintDeviceInfo
	runSingleCamera calls setAcquisitionMode
	runSingleCamera calls getImage, setExposureTime, or setShutterMode, etc. 
		
See function comments for more details. */

#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"
#include <iostream>
#include <sstream>
#include <typeinfo>
#include <ctime>
#include "include/features.cpp"
#include "include/relayControl.cpp"

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;
using namespace std;

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

/* Function called by main in case where only one camera connected. Function calls PrintDeviceInfo, setupGPIO,
and setAcquisitionMode, setting up the camera and generates cli allowing user to take photos via
function getImages, change exposure time via function setExposureTime, change adc bit depth via function 
setADCBitDepth, change shutter mode via function setShutterMode, and changes any other camera variable which has string options
via function set */
int runSingleCamera(CameraPtr pCam, SystemPtr system, CameraList camList) {
    INodeMap& nodeMapTLDevice = pCam->GetTLDeviceNodeMap();
    PrintDeviceInfo(nodeMapTLDevice);
    pCam->Init();
    INodeMap& nodeMap = pCam->GetNodeMap();
    set("AcquisitionMode", pCam, nodeMap, nodeMapTLDevice, "SingleFrame");
    setupGPIO(pCam, nodeMap, nodeMapTLDevice);
    closeRelay();
    setPixelFormat(pCam, nodeMap, nodeMapTLDevice, 8);
#ifdef _DEBUG
        cout << endl << endl << "*** DEBUG ***" << endl << endl;

        // If using a GEV camera and debugging, should disable heartbeat first to prevent further issues
        if (DisableHeartbeat(nodeMap, nodeMapTLDevice) != 0)
        {
            return -1;
        }

        cout << endl << endl << "*** END OF DEBUG ***" << endl << endl;
#endif

    cout << "camera in Acquisition Mode" << endl; 
    //modify device settings here 
    while (true) {
    	pCam->BeginAcquisition();
	    cout << "press c to take a photo, s to modify shutter mode, p to modify pixel format, e to modify exposure time, b to modify adc bit depth, a to modify any camera attribute, o to turn camera off, and x to quit" << endl;
        char input;
        cin >> input;
        cout << "Input: " << input << endl;
        if (input == 'c') {
            getImage(pCam, nodeMap, nodeMapTLDevice);
            cout << "Image acquired" << endl;
	        pCam->EndAcquisition();
        } else if (input == 'x') {
	        pCam->EndAcquisition();
	        break;
        } else if (input == 'a') {
            pCam->EndAcquisition();
            gcstring attribute;
            cout << "Please enter the attribute you'd like to modify (see technical reference)" << endl;
            cin >> attribute;
            gcstring value;
            cout << "Please enter the value you'd like to change " << attribute << " to." << endl;
            cin >> value;
            cout << "Attempting to change " << attribute << " to " << value << endl;
            if(set(attribute, pCam, nodeMap, nodeMapTLDevice, value) == 0) {
                cout << "Success" << endl;
            } 
        } else if (input == 'e') {
	        pCam->EndAcquisition();
            int exposureTime;
            cout << "new exposure time (microseconds):" << endl;
            cin >> exposureTime;
            setExposureTime(pCam, nodeMap, nodeMapTLDevice, exposureTime);
        } else if (input == 'b') {
            pCam->EndAcquisition();
	        int bitDepth;
	        cout << "bit10: 10, bit12: 12, bit14: 14" << endl;
	        cin >> bitDepth;
	        setADCBitDepth(pCam, nodeMap, nodeMapTLDevice, bitDepth);
	    } else if (input == 's') {
            pCam->EndAcquisition();
            int shutterMode;
            cout << "Rolling: 0, Global Reset: 1" << endl;
            cin >> shutterMode;
            setShutterMode(pCam, nodeMap, nodeMapTLDevice, shutterMode);
        } else if (input == 'p') {
            pCam->EndAcquisition();
            int pixelFormat;
            cout << "Mono8: 8, Mono16: 16" << endl;
            cin >> pixelFormat;
            setPixelFormat(pCam, nodeMap, nodeMapTLDevice, pixelFormat);
        } else if (input == 'o') {
            pCam->EndAcquisition();
            /*pCam->DeInit();
            pCam = nullptr;
            camList.Clear();
            system->ReleaseInstance();*/
            openRelay();
	    set("UserOutputSelector", pCam, nodeMap, nodeMapTLDevice, "UserOutput0");
	    setBool("UserOutputValue", pCam, nodeMap, nodeMapTLDevice, false);
            while(true) {
                char input;
                cout << "press o to turn back on" << endl;
                cin >> input;
                if(input == 'o') {
                    closeRelay();
		    setBool("UserOutputValue", pCam, nodeMap, nodeMapTLDevice, true);
                    //main(0, "")
                    /* cout << 1 << endl;
                    camList = system->GetCameras();
                    cout << 2 << endl;
                    pCam = camList.GetByIndex(0);
                    cout << 3 << endl;
                    nodeMapTLDevice = pCam->GetTLDeviceNodeMap();
                    cout << 4 << endl;
                    pCam->Init();
                    cout << 5 << endl;
                    nodeMap = pCam->GetNodeMap();
                    cout << 6 << endl;
                    setAcquisitionMode(pCam, nodeMap, nodeMapTLDevice);
                    cout << 7 << endl;
                    setPixelFormat(pCam, nodeMap, nodeMapTLDevice, 8);
                    cout << 8 << endl; */
                    break;
                }
            }
        } else {
            pCam->EndAcquisition();
            cout << "Input not valid" << endl;
        }
    }
    pCam->DeInit();
    return 0;
}

int runFake() {
    wiringPiSetup();
    pinMode(5, OUTPUT);
    closeRelay();
    while(true) {
	char input;
	cin >> input;
	if (input == 'o') {
	    openRelay();
	    while(true) {
		char input2;
		cin >> input2;
		if (input2 == 'o') {
		    closeRelay();
		    break;
		}
	    }
	}
    }
}

/* Main function takes as input no arguments. Determines number of cameras and calls corresponding run
camera(s) function. */
int main(int /*argc*/, char** /*argv*/) {
    FILE* tempFile = fopen("test.txt", "w+"); // checks if we have permission to write to the current folder
    if (tempFile == nullptr)
    {
        cout << "Permission denied to write to the current foldr. Press Enter to exit" << endl;
        getchar();
        return -1;
    }
    fclose(tempFile);
    remove("test.txt");



    runFake();






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
    if (numCameras == 0)
    {
        // Clear camera list before releasing system
        camList.Clear();

        // Release system
        system->ReleaseInstance();

        cout << "Not enough cameras! Press Enter to exit." << endl;
        getchar();
        return -1;
    } else if (numCameras == 1) {
        cout << "One camera detected" << endl;

        CameraPtr pCam = camList.GetByIndex(0);
        runSingleCamera(pCam, system, camList);

        pCam = nullptr;
    } else if (numCameras == 2) {
        cout << "Two cameras detected" << endl;

        CameraPtr pCam1 = camList.GetByIndex(0);
        CameraPtr pCam2 = camList.GetByIndex(1);

        pCam1 = nullptr;
        pCam2 = nullptr;
    }
    openRelay();
    camList.Clear();
    
    system->ReleaseInstance();
    return 0;
}
