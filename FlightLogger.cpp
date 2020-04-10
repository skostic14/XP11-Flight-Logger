#include "../SDK/CHeaders/XPLM/XPLMDataAccess.h"
#include "../SDK/CHeaders/XPLM/XPLMProcessing.h"
#include <string>
#include <iostream>
#include <fstream>

#if IBM
#include <windows.h>
#endif
#if LIN
#include <GL/gl.h>
#elif __GNUC__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#pragma warning(disable : 4996)

using namespace std;

XPLMDataRef latitudeRef;
XPLMDataRef longitudeRef;
XPLMDataRef elevationRef;
XPLMDataRef groundspeedRef;
XPLMDataRef zuluRef;

ofstream outputFile;

const char* filename = "outputFile.csv";
const char* latitudeRefString = "sim/flightmodel/position/latitude";
const char* longitudeRefString = "sim/flightmodel/position/longitude";
const char* elevationRefString = "sim/flightmodel/position/elevation";
const char* groundspeedRefString = "sim/flightmodel/position/groundspeed";
const char* zuluRefString = "sim/time/zulu_time_sec";

static float writeData(float, float, int, void*);

PLUGIN_API int XPluginStart (char* outName, char* outSig, char* outDesc) {
	strcpy(outName, "Mini location logger");
	strcpy(outSig, "skostic1.logger.alpha");
	strcpy(outDesc, "Mini location logger by skostic14. Using XPLM300 SDK.");

	latitudeRef = XPLMFindDataRef(latitudeRefString);
	longitudeRef = XPLMFindDataRef(longitudeRefString);
	elevationRef = XPLMFindDataRef(elevationRefString);
	groundspeedRef = XPLMFindDataRef(groundspeedRefString);
	zuluRef = XPLMFindDataRef(zuluRefString);

	XPLMRegisterFlightLoopCallback(writeData, 5.0, NULL);

	return 1;
}

PLUGIN_API void XPluginStop(void) {
	outputFile.close();
	XPLMUnregisterFlightLoopCallback(writeData, NULL);
}

PLUGIN_API int XPluginEnable(void) {
	outputFile.open(filename);
	outputFile << "TIME,LATITUDE,LONGITUDE,ELEVATION,SPEED\n";
	return 1;
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFrom, int inMsg, void* inParam) { }

static float writeData(float elapsedSinceLastCall, float elapsedSinceLastFlightLoop, int counter, void* refcon) {
	double userLatitude = 0.0;
	double userLongitude = 0.0;
	float userAltitude = 0.0f;
	float userGroundspeed = 0.0f;
	int userZuluTime = 0;

	float nextCallInterval = 1.0f;

	userLatitude = XPLMGetDatad(latitudeRef);
	userLongitude = XPLMGetDatad(longitudeRef);
	userAltitude = XPLMGetDataf(elevationRef);
	userGroundspeed = XPLMGetDataf(groundspeedRef);
	userZuluTime = int(XPLMGetDataf(zuluRef));

	int hours = userZuluTime / 3600;
	int minutes = (userZuluTime / 60) % 60;
	int seconds = userZuluTime % 60;

	outputFile << hours << ":" << minutes << ":" << seconds << "," << userLatitude << "," << userLongitude << "," << userAltitude << "," << userGroundspeed / 2 << "\n";
	outputFile.flush();

	if (userGroundspeed < 1) {
		nextCallInterval = 30;
	}
	else if (userGroundspeed < 5) {
		nextCallInterval = 3;
	}
	else if (userGroundspeed < 30) {
		nextCallInterval = 5;
	}
	else if (userGroundspeed < 75) {
		nextCallInterval = 10;
	}
	else if (userGroundspeed < 150) {
		nextCallInterval = 15;
	}
	else {
		nextCallInterval = 20;
	}

	return nextCallInterval;
}