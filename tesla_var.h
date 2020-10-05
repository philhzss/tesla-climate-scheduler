#pragma once
#include <iostream>
#include <iostream>
#include <vector>
#include "nlohmann/json.hpp"


using json = nlohmann::json;
using std::cout;
using std::endl;
using std::cin;
using std::string;


// Program info
const string tcs_versionInfo = "\nVersion: _*3.0 inDev*_\nBuild Date: 2020.09.14 - First C++ version";


// Declarations
void testFunc();
string curl_GET(string url);
string curl_POST(string url, string message);
extern time_t nowTime_secs;
const string string_time_and_date(tm tstruct);



// Stores program settings, including but not limited to settings.json stuff
class settings {
public:
	// Initial breakdown into separate json objects
	static json teslaSettings, calendarSettings, notifSettings, carSettings;

	// Car Setting
	// (commute + HVAC preheat time + buffer) How long before shift start to wake car to check car temps
	static int intwakeTimer;
	// Car Setting
	// (commute + HVAC preheat time) Determined based on temperature, settings.
	static int inttriggerTimer;



	// Important stuff, TeslaFi(TT) token, Tesla URL
	static string tfiURL;
	static string teslaURL;
	static string teslaHeader;


	static void readSettings(string silent);




	// u_ser defined settings (from settings.json):

	// Car Setting
	// Real amount of MINS of drive time between home and work
	static string u_commuteTime;
	static int intcommuteTime;
	// Car Setting
	// Limiter is to stop climate if you called sick / if still at home X amount of time before shift in MINS
	static string u_shutoffTimer;
	static int intshutoffTimer;
	// Car Setting
	// If car is parked in garage at home, set the following to True to save power.
	static string u_garageEnabled;
	// Car Setting
	// Garage bias time is how many mins before LEAVING car should activate, if car in garage. Similar to comfy_deltaTime
	static string u_garageBias;

	// Notification Setting
	// Slack Channel for Slack notifications
	static string u_slackChannel;

	// Calendar Setting
	// URL to Calendar file for event triggers
	static string u_calendarURL;

	// Tesla Setting
	// Tesla official API email
	static string u_teslaEmail;
	// Tesla Setting
	// Tesla official API password
	static string u_teslaPassword;
	// Tesla Setting
	// TeslaFi Token
	static string u_teslaFiToken;
	};


// How long to wait before the program loops entirely, in SECONDS
const int repeatDelay = 7;



// Marker leave from work enabled or not. False will add + 30 mins to shift end times. See main CPP file for details
const bool markerEnabled = false;





/*Set time & temp parameters here, in DEGREES C and MINS:
	coldX_temp is the temp limit(will trigger if VALUE or colder), in DEGREES C
	cX_deltaTime is how long the car should start before YOU LEAVE, in MINS
	Higher cX_deltaTimes mean the car will start HVAC* earlier*/
const int cold3_temp = -20;
const int c3_deltaTime = 35;

const int cold2_temp = -8;
const int c2_deltaTime = 26;

const int cold1_temp = 2;
const int c1_deltaTime = 17;

// "Else" timer, if temp is above cold1Temp, preheats this amount of time before you LEAVE:
const int else_deltaTime = 13;

// comfy_deltaTime : If outside temp is between 17 and 24C, car will preheat this about of mins before you LEAVE to save power :
const int comfy_deltaTime = 7;