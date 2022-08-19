/* File written by Jonah Ezekiel (jezekiel@stanford.edu). Simple script to log whenever a
specified GPIO input pin registers a change in the signal it is recieving. Used in camera trigger
studies to read output from camera gpio and determine precisely when the camera begins and stops
exposure. 
	
See function comments for more details. */

#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"
#include <iostream>
#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>

#define CAM_NUM 4 // somehow pin 4 digitally  maps to pin 23 on the pi

using namespace std;
using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;

int main(int argc, char **argv) {
    wiringPiSetup();
    pinMode(CAM_NUM, INPUT);
    int current = digitalRead(CAM_NUM);
    cout << current << endl;
    while (true) {
	int now = digitalRead(CAM_NUM);
        if(now != current) {
            cout << now << endl;
            current = now;
        }
    }
}











