/* File written by Jonah Ezekiel (jezekiel@stanford.edu). Program allows for software interfacing with
 * raspberry pi gpio pins. Intended to be used as a dependancy for files a directory higher, allowing 
 those files to use GPIO to trigger relay switches and hence power cycle cameras. 
 *		
 * See function comments for more details. */

#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"
#include <iostream>
#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>

#define RELAY_NUM 5 //Somehow pin 5 digitially maps to pin 24 on the pi

using namespace std;
using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;

/* closes relay, powering on camera */
void closeRelay() {
    cout << "Closing relay" << endl;
    digitalWrite(RELAY_NUM, true);
}
/* opens relay, powering off camera */
void openRelay() {
    cout << "Opening relay" << endl;
    digitalWrite(RELAY_NUM, false);
}
/* sets up gpio so that openRelay and closeRelay can be called. Function should be called once
at the beginning of a program. */
void setupGPIO(CameraPtr pCam, INodeMap& nodeMap, INodeMap& nodeMapTLDevice) {
    cout << "Setting up relay on gpio pin " << RELAY_NUM << endl;
    wiringPiSetup();
    pinMode(RELAY_NUM, OUTPUT);
<<<<<<< HEAD
    set("LineSelector", pCam, nodeMap, nodeMapTLDevice, "Line2");
    set("LineMode", pCam, nodeMap, nodeMapTLDevice, "Output");
    //set("UserOutputSelector", pCam, nodeMap, nodeMapTLDevice, "UserOutput0");
    //setBool("UserOutputValue", pCam, nodeMap, nodeMapTLDevice, true);
    set("LineSource", pCam, nodeMap, nodeMapTLDevice, "ExposureActive");
=======
>>>>>>> ca9db318c1ddbafe6863043cef07726a9745e1af
}











