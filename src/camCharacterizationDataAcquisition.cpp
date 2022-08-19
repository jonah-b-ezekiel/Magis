/*Please see acquisitionFunc.cpp program before looking at this file as the functionality 
is similar, but better explained in that file. Program allows user to write scripts to modify
features of spinnaker SDK blackfly model s cameras and acquire images over software for purposes
of acquiring data for camera characterization purposes. Note: Program requires a config file to
be passed in via command line. See "config_files" directory for an example. 
        
NOTE: we've had problems with the camera crashing after taking too many images that are still
unsolved. For this purpose, when running this script, we've commonly had to acquire the data we
need in small chunks, resetting and rerunning the script regularely.

See function comments for more details. */

#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"
#include <iostream>
#include <sstream>
#include <typeinfo>
#include <ctime>
#include <vector>
#include <fstream>
#include "include/features.cpp"

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;
using namespace std;

int numPerSettings;  // number of photos to take at each exposure time
string DATA_BASE;
string DATA_DIR;
string RUN_NUM;

/* Function to generate custom vector of exposure times to test the camera. Should be customly written by
user aquiring data to suit their purposes. */
vector<int> get_exposure_times() {
    
    //For rolling shutter:
    //Set of exposure times: 25us to 5000us
    /*
    vector<int> exposureTimeList = {25, 50, 100, 200, 400, 600, 800};
    for (int t_exp = 1000; t_exp < 5000; t_exp += 500) {
        exposureTimeList.push_back(t_exp);
    }
    larger exposure times: 5ms onwards
    vector<int> exposureTimeList = {};
    for (int t_exp = 105000; t_exp <= 200000; t_exp += 5000) {
        exposureTimeList.push_back(t_exp);
    }*/


    //For global reset: (min. exposure time is 350us)
    //Set of exposure times: 700us to 5000us

    vector<int> exposureTimeList = {700, 800};
    for (int t_exp = 1000; t_exp < 5000; t_exp += 500) {
        exposureTimeList.push_back(t_exp);
    }
    larger exposure times: 5ms onwards
    for (int t_exp = 180000; t_exp <= 200000; t_exp += 5000) {
        exposureTimeList.push_back(t_exp);
    }

    return exposureTimeList;
}

int loadConfiguration(string filename) {
    ifstream in(filename);
    if (!in.is_open()) {
        cout << "Unable to open config file" << endl;
        return -1;
    }
    string param;
    while (!in.eof()) {
        in >> param;
        if (param == "numPerSettings") {
            int value;
            in >> value;
            numPerSettings = value;
        } else if (param == "DATA_BASE") {
            string value;
            in >> value;
            DATA_BASE = value;
        } else if (param == "DATA_DIR") {
            string value;
            in >> value;
            DATA_DIR = value;
        } else if (param == "RUN_NUM") {
            string value;
            in >> value;
            RUN_NUM = value;
        }
    }
    in.close();
    return 0;
}

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
nodeMapTLDevice. Also takes as input exposre time, adc bit depth, and the photo index
for purposes of naming the file descriptavely. Acquires a single image and saves this
image in the specified directory. Works under assumption that camera is in Acquisition
mode. */
int getImage(CameraPtr pCam, INodeMap& nodeMap, INodeMap& nodeMapTLDevice, int exposureTime, int bitDepth, int numPhoto) {
    ImagePtr pResultImage = pCam->GetNextImage(1000);
    if (pResultImage->IsIncomplete()) {
        cout << "Image incomplete: " << Image::GetImageStatusDescription(pResultImage->GetImageStatus()) << endl;
        return -1;
    } else {
        //optionally convert image at this point to different format
        ostringstream filename;
        filename << DATA_BASE << DATA_DIR << RUN_NUM << '/';
        // filename << "/home/pi/magis/data/DIS/lab_images/20220722/run_01/";
        filename << "img";
        filename << "_exp" << exposureTime << "us";
        filename << "_bitDepth" << bitDepth;
        char ind[12];
        sprintf(ind, "%03d", numPhoto + 1); // index is zero-based, but I'd like files to be one-based
        filename << "_" << ind;
        filename << ".raw";
        pResultImage->Save(filename.str().c_str());
        cout << "Image saved" << endl;
        pResultImage->Release();
        return 0;
    }
}

/* Function called by main in case where only one camera connected. Can be modified
to write custom script to run camera to acquire certain sets of images. Takes as input
camera pointer. After collecting data, pushes data to Sanha's SLAC SDF space and removes
it from local machine. NOTE: we've had problems with the camera crashing after taking too
many images that are still unsolved. For this purpose, when running this script, we've commonly
had to acquire the data we need in small chunks, resetting and rerunning the script regularely. */
int runSingleCamera(CameraPtr pCam) {
    INodeMap& nodeMapTLDevice = pCam->GetTLDeviceNodeMap();
    PrintDeviceInfo(nodeMapTLDevice);

#ifdef _DEBUG
        cout << endl << endl << "*** DEBUG ***" << endl << endl;

        // If using a GEV camera and debugging, should disable heartbeat first to prevent further issues
        if (DisableHeartbeat(nodeMap, nodeMapTLDevice) != 0)
        {
            return -1;
        }

        cout << endl << endl << "*** END OF DEBUG ***" << endl << endl;
#endif

    vector<int> exposureTimeList = get_exposure_times()

    cout << "beginning data acquisition" << endl;

    for (int t_exp : exposureTimeList) {
        pCam->Init();
        INodeMap& nodeMap = pCam->GetNodeMap();
        setAcquisitionMode(pCam, nodeMap, nodeMapTLDevice);
        setPixelFormat(pCam, nodeMap, nodeMapTLDevice, 16);
        setShutterMode(pCam, nodeMap, nodeMapTLDevice, 1);
        setExposureTime(pCam, nodeMap, nodeMapTLDevice, t_exp);

        for(int bit_depth = 10; bit_depth <= 14; bit_depth += 2) {
            setADCBitDepth(pCam, nodeMap, nodeMapTLDevice, bit_depth);
            for(int i = 0; i < numPerSettings; i++) {
                pCam->BeginAcquisition();
                getImage(pCam, nodeMap, nodeMapTLDevice, t_exp, bit_depth, i);
                pCam->EndAcquisition();
            }
        }
        
        cout << "Uploading data to Sanha's GFPS space" << endl;
        ostringstream scp_command;
        scp_command << "sshpass -p \"Dbslrtm^#81\" ";
        scp_command << "scp -r " << DATA_BASE << DATA_DIR << RUN_NUM << "/* ";
        scp_command << "sanha@centos7.slac.stanford.edu:/gpfs/slac/atlas/fs1/u/sanha/gpfs-storage/data_storage/magis/data/" << DATA_DIR << RUN_NUM << "/";
        system(scp_command.str().c_str());
        cout << "Upload complete!" << endl;

        cout << "Removing data from local space" << endl;
        ostringstream rm_command;
        rm_command << "rm " << DATA_BASE << DATA_DIR << RUN_NUM << "/*";
        system(rm_command.str().c_str());
        cout << "Removal complete!" << endl;

        pCam->DeInit();
    }
    return 0;
}

/* Main function takes as input config file. calls loadConfiguration with the config file as an argument, then Determines number of cameras and calls corresponding run
camera(s) function. */
int main(int argc, char** argv) {
    if (argc < 2) {
        cout << "Please pass in a config file" << endl;
        return -1;
    }
    if(loadConfiguration(argv[1]) == -1) {
        return -1;
    }
    FILE* tempFile = fopen("test.txt", "w+"); // checks if we have permission to write to the current folder
    if (tempFile == nullptr)
    {
        cout << "Permission denied to write to the current folder. Press Enter to exit" << endl;
        getchar();
        return -1;
    }
    fclose(tempFile);
    remove("test.txt");

    // Print application build information
    cout << "Application build date: " << __DATE__ << " " << __TIME__ << endl << endl;

    // Retrieve singleton reference to system object
    SystemPtr system = System::GetInstance();

    // Print out current library version
    const LibraryVersion spinnakerLibraryVersion = system->GetLibraryVersion();
    cout << "Spinnaker library version: " << spinnakerLibraryVersion.major << "." << spinnakerLibraryVersion.minor
         << "." << spinnakerLibraryVersion.type << "." << spinnakerLibraryVersion.build << endl
         << endl;

    // Retrieve list of cameras from the system
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
        runSingleCamera(pCam);

        pCam = nullptr;
    } else if (numCameras == 2) {
        cout << "Two cameras detected" << endl;
        
        CameraPtr pCam1 = camList.GetByIndex(0);
        CameraPtr pCam2 = camList.GetByIndex(1);



        pCam1 = nullptr;
        pCam2 = nullptr;
    }
    camList.Clear();
    system->ReleaseInstance();
    return 0;
}
