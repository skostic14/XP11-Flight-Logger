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

XPLMDataRef latitude;
XPLMDataRef longitude;
XPLMDataRef elevation;

ofstream outputFile;
const char* filename = "outputFile.csv";
const char* latitudeRef = "sim/flightmodel/position/latitude";
const char* longitudeRef = "sim/flightmodel/position/longitude";
const char* elevationRef = "sim/flightmodel/position/elevation";

static float writeData(float, float, int, void*);

PLUGIN_API int XPluginStart (char* outName, char* outSig, char* outDesc) {
	strcpy(outName, "Mini location logger");
	strcpy(outSig, "skostic1.logger.alpha");
	strcpy(outDesc, "Mini location logger by skostic14. Using XPLM300 SDK.");

	latitude = XPLMFindDataRef(latitudeRef);
	longitude = XPLMFindDataRef(longitudeRef);
	elevation = XPLMFindDataRef(elevationRef);

	XPLMRegisterFlightLoopCallback(writeData, 5.0, NULL);

	return 1;
}

PLUGIN_API void XPluginStop(void) {
	outputFile.close();
	XPLMUnregisterFlightLoopCallback(writeData, NULL);
}

PLUGIN_API int XPluginEnable(void) {
	outputFile.open(filename);
	outputFile << "TIME,LATITUDE,LONGITUDE,ELEVATION\n";
	return 1;
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFrom, int inMsg, void* inParam) { }

static float writeData(float elapsedSinceLastCall, float elapsedSinceLastFlightLoop, int counter, void* refcon) {
	double userLatitude = 0.0;
	double userLongitude = 0.0;
	float userAltitude = 0.0f;

	userLatitude = XPLMGetDatad(latitude);
	userLongitude = XPLMGetDatad(longitude);
	userAltitude = XPLMGetDataf(elevation);

	outputFile << elapsedSinceLastCall << "," << userLatitude << "," << userLongitude << "," << userAltitude << "\n";
	outputFile.flush();

	return 5.0;
}