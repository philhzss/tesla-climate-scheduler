#pragma once
#include <nlohmann/json.hpp>
#include <fstream>

using std::string;
using json = nlohmann::json;

// Stores program settings, including but not limited to settings.json stuff
class settings 
{
public:
	// Initial breakdown into separate json objects
	static json teslaSettings, calendarSettings, generalSettings, carSettings;

	// Car Setting
	// (commute + max HVAC preheat time + buffer) How long before shift start to wake car to check car temps
	static int intwakeTimer;
	// Car Setting
	// (commute + HVAC preheat time) Determined based on temperature, settings.
	static int inttriggerTimer;



	// Important stuff, Tesla official token, Tesla URL
	static string tfiURL;
	static string teslaURL;
	static string teslaHeader;
	static json authPackage;
	static json teslaAuthHeader;


	// Parse settings.json and save its contents in the program's memory
	static void readSettings(string silent = "");




	// u_ser defined settings (from settings.json):

	// General Setting
	// Slack Channel for Slack notifications
	static string u_slackChannel;
	// General Setting
	// (Bool) If true, logs all logger message to file
	static string u_logToFile;
	// General Setting
	// (Seconds) How long to wait before the program loops entirely
	static string u_repeatDelay;
	static int intrepeatDelay;


	// Car Setting
	// (Minutes) Drive time between home and work
	static string u_commuteTime;
	static int intcommuteTime;
	// Car Setting
	// Limiter is to stop climate if you called sick / if still at home X amount of time before shift in MINS
	static string u_shutoffTimer;
	static int intshutoffTimer;
	// Car Setting
	// (default=5) (Minutes) The higher this is, the more time your car's HVAC will run. More details in docs
	static string u_default20CMinTime;
	static int intdefault20CMinTime;


	// Calendar Setting
	// URL to Calendar file for event triggers
	static string u_calendarURL;
	// Calendar Setting
	// (Minutes) If you leave home to target arriving at work earlier than event start, enter a POSITIVE number.
	// Ex: Want to arrive at work 15 mins before event start? Enter 15. EXCLUDES commute time. 
	// The car will be ready at (commuteTime - shiftStartBias) mins before event start.
	static string u_shiftStartBias;
	static int intshiftStartBias;
	// Calendar Setting
	// (Minutes), if you leave work when you calendar event ends, this should be 0.
	// If you leave work early, enter a negative number, if late, positive number.
	static string u_shiftEndBias;
	static int intshiftEndBias;
	// Calendar Setting
	// Enter 0, 1 or 2 words that will cause the program to IGNORE events containing them
	static string u_ignoredWord1;
	static string u_ignoredWord2;



	// Tesla Setting
	// Tesla official API email
	static string u_teslaEmail;
	// Tesla Setting
	// Tesla official API password
	static string u_teslaPassword;
	// Tesla Setting
	// TeslaFi Token
	static string u_teslaFiToken;
	// Tesla Setting
	// CLIENT_ID
	static string u_teslaClientID;
	// Tesla Setting
	// CLIENT_SECRET
	static string u_teslaClientSecret;
};