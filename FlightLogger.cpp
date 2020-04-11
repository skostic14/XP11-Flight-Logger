#include "../SDK/CHeaders/XPLM/XPLMDataAccess.h"
#include "../SDK/CHeaders/XPLM/XPLMProcessing.h"
#include "../SDK/CHeaders/XPLM/XPLMMenus.h"
#include "../SDK/CHeaders/XPLM/XPLMDisplay.h"
#include "../SDK/CHeaders/XPLM/XPLMGraphics.h"

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

/* Disabling compiler warning for unsafe strcpy function */
#pragma warning(disable : 4996)

using namespace std;

/* Objects to store DataRefs from X-Plane */
XPLMDataRef latitudeRef;
XPLMDataRef longitudeRef;
XPLMDataRef elevationRef;
XPLMDataRef groundspeedRef;
XPLMDataRef zuluRef;

/* Objects to handle X-Plane menus */
XPLMMenuID loggerMenuId;
int startLoggingMenuIndex;
int stopLoggingMenuIndex;

ofstream outputFile;

const char* flightNumber = "Default";
const char* const fileExtension = "_log.csv";

/* Reference strings extracted from XP11 DataRef */
const char* const latitudeRefString = "sim/flightmodel/position/latitude";
const char* const longitudeRefString = "sim/flightmodel/position/longitude";
const char* const elevationRefString = "sim/flightmodel/position/elevation";
const char* const groundspeedRefString = "sim/flightmodel/position/groundspeed";
const char* const zuluRefString = "sim/time/zulu_time_sec";

/* Strings used in menu */
const char* const menuName = "Flight Logger";
const char* const startLoggingString = "Start logging";
const char* const stopLoggingString = "Stop logging";

/* Functions needed as callback methods */
static float writeData(float, float, int, void*);
static void	loggerMenuHandler(void*, void*);

PLUGIN_API int XPluginStart (char* outName, char* outSig, char* outDesc) {
	strcpy(outName, "Mini location logger");
	strcpy(outSig, "skostic1.logger.alpha");
	strcpy(outDesc, "Mini location logger by skostic14. Using XPLM300 SDK.");

	/* Using find data ref to find data for reference strings */
	latitudeRef = XPLMFindDataRef(latitudeRefString);
	longitudeRef = XPLMFindDataRef(longitudeRefString);
	elevationRef = XPLMFindDataRef(elevationRefString);
	groundspeedRef = XPLMFindDataRef(groundspeedRefString);
	zuluRef = XPLMFindDataRef(zuluRefString);

	/* Create plugin menu */
	int menuContainerID = XPLMAppendMenuItem(XPLMFindPluginsMenu(), menuName, 0, 0);
	loggerMenuId = XPLMCreateMenu(menuName, XPLMFindPluginsMenu(), menuContainerID, loggerMenuHandler, NULL);

	/* Appending buttons to menu */
	startLoggingMenuIndex = XPLMAppendMenuItem(loggerMenuId, startLoggingString, (void *) startLoggingString, 1);
	XPLMAppendMenuSeparator(loggerMenuId);
	stopLoggingMenuIndex =XPLMAppendMenuItem(loggerMenuId, stopLoggingString, (void*) stopLoggingString, 1);

	/* Disabling stop logging button */
	XPLMEnableMenuItem(loggerMenuId, stopLoggingMenuIndex, 0);

	return 1;
}

PLUGIN_API void XPluginStop(void) { 
	XPLMDestroyMenu(loggerMenuId);
}

PLUGIN_API int XPluginEnable(void) { return 1;}

PLUGIN_API void XPluginDisable(void) { }

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFrom, int inMsg, void* inParam) { }

/* Callback function for writing aircraft position */
static float writeData(float elapsedSinceLastCall, float elapsedSinceLastFlightLoop, int counter, void* refcon) {
	double userLatitude = 0.0;
	double userLongitude = 0.0;
	float userAltitude = 0.0f;
	float userGroundspeed = 0.0f;
	int userZuluTime = 0;

	float nextCallInterval = 1.0f;

	userLatitude = XPLMGetDatad(latitudeRef);
	userLongitude = XPLMGetDatad(longitudeRef);

	/* Converting meters to feet */
	userAltitude = XPLMGetDataf(elevationRef) * 3.281f;
	/* Converting m/s to knots */
	userGroundspeed = XPLMGetDataf(groundspeedRef) * 2;

	/* Getting zulu time in seconds, parsing to HH:MM:SS */
	userZuluTime = int(XPLMGetDataf(zuluRef));
	int hours = userZuluTime / 3600;
	int minutes = (userZuluTime / 60) % 60;
	int seconds = userZuluTime % 60;

	outputFile << hours << ":" << minutes << ":" << seconds << "," << userLatitude << "," << userLongitude << "," << userAltitude << "," << userGroundspeed << "\n";
	outputFile.flush();

	/* Deciding time for next call based on last recorded speed */
	if (userGroundspeed < 1.0f) {
		nextCallInterval = 30;
	}
	else if (userGroundspeed < 20.0f) {
		nextCallInterval = 10;
	}
	else if (userGroundspeed < 80.0f) {
		nextCallInterval = 20;
	}
	else if (userGroundspeed < 150.0f) {
		nextCallInterval = 40;
	}
	else if (userGroundspeed < 300.0f) {
		nextCallInterval = 50;
	}
	else {
		nextCallInterval = 30;
	}

	return nextCallInterval;
}

/* Callback for buttons pressed in menu */
static void	loggerMenuHandler(void * menuReference, void * itemReference) {

	/* If start logging is pressed */
	if (strcmp((const char*)itemReference, startLoggingString) == 0) {
		/* Dummy. Supposed to take flight number from user. */
		char outputFileName[256];
		strcpy(outputFileName, flightNumber);
		strcat(outputFileName, fileExtension);
		outputFile.open(outputFileName);
		outputFile << "TIME,LATITUDE,LONGITUDE,ELEVATION,SPEED\n";

		/* Registering callback function for data write */
		XPLMRegisterFlightLoopCallback(writeData, 5.0, NULL);
		/* Disabling start logging button and enabling stop logging button */
		XPLMEnableMenuItem(loggerMenuId, startLoggingMenuIndex, 0);
		XPLMEnableMenuItem(loggerMenuId, stopLoggingMenuIndex, 1);
	}

	/* If stop logging is pressed */
	else if (strcmp((const char*)itemReference, stopLoggingString) == 0) {
		/* Deregistering callback function for data write */
		outputFile.close();
		XPLMUnregisterFlightLoopCallback(writeData, NULL);

		/* Disabling stop logging button and enabling start logging button */
		XPLMEnableMenuItem(loggerMenuId, startLoggingMenuIndex, 1);
		XPLMEnableMenuItem(loggerMenuId, stopLoggingMenuIndex, 0);
	}
}