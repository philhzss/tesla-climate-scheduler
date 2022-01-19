#pragma once
#include <nlohmann/json.hpp>
#include <fstream>
#include <mutex>

using std::string;
using json = nlohmann::json;

// Stores program settings, including but not limited to settings.json stuff
class settings
{
public:
	// Initial breakdown into separate json objects
	static json teslaSettings, calendarSettings, generalSettings, carSettings;


	// Important stuff, Tesla official token, Tesla URL
	static string teslaOwnerURL;
	static string teslaAuthURL;
	static string teslaVURL;
	static string teslaVID;
	static json authReqPackage;
	static string teslaAuthString;


	// Parse settings.json and save its contents in the program's memory
	static void readSettings(string silent = "");




	// u_ser defined settings (from settings.json):

	// General Setting
	// Slack Channel for Slack notifications
	static string u_slackChannel;
	static bool slackEnabled;
	// General Setting
	// (Bool) If true, logs all logger message to file
	static string u_logToFile;
	// General Setting
	// (Seconds) How long to wait before the program loops entirely
	static string u_repeatDelay;
	static int intrepeatDelay;
	// General Setting
	// [lat, long] Home coordinates
	static std::vector<string> u_homeCoords;
	// General Setting
	// [lat, long] Work coordinates
	static std::vector<string> u_workCoords;



	// Car Setting
	// Limiter is to stop climate if you called sick / if still at home X amount of time before shift in MINS
	static string u_shutoffTimer;
	static int intshutoffTimer;
	// Car Setting
	// (default=5) (Minutes) The higher this is, the more time your car's HVAC will run. More details in docs
	static string u_default20CMinTime;
	static int intdefault20CMinTime;
	// Car Setting
	// (commute + max HVAC preheat time + buffer) How long before shift start to wake car to check car temps
	static int intwakeTimer;
	// Car Setting
	// (commute + HVAC preheat time) Determined based on temperature, settings.
	static int inttriggerTimer;
	// Car Setting
	// Real amount of time BEFORE shift (event) you leave from home
	static int intshiftStartTimer;
	// Car Setting
	// Real amount of time AFTER shift (event) end you leave from work
	static int intshiftEndTimer;


	// Calendar Setting
	// URL to Calendar file for event triggers
	static string u_calendarURL;
	// Calendar Setting
	// (Minutes) If you leave home to target arriving at work earlier than event start, enter a NEGATIVE number.
	// Ex: Want to arrive at work 15 mins before event start? Enter -15. EXCLUDES commute time. 
	// The car will be ready at (commuteTime - shiftStartBias) mins before event start.
	static string u_shiftStartBias;
	static int intshiftStartBias;
	// Calendar Setting
	// (Minutes), if you leave work when you calendar event ends, this should be 0.
	// If you leave work early, enter a negative number, if late, positive number.
	static string u_shiftEndBias;
	static int intshiftEndBias;
	// Calendar Setting
	// (Minutes) Drive time between home and work
	static string u_commuteTime;
	static int intcommuteTime;
	// Calendar Setting
	// Enter words that will cause the program to IGNORE events containing them
	static std::vector<string> u_wordsToIgnore;
	static bool ignoredWordsExist();
	static string ignoredWordsPrint();


	// Tesla Setting
	// Tesla official API email
	static string u_teslaEmail;
	// Tesla Setting
	// Tesla official API password
	static string u_teslaPassword;
	// Tesla Setting
	// Owner API Access Token
	static string u_teslaAccessToken; // not declared!!!

	// API
	// Mutex lock for reading and clearing the settings file
	static std::mutex settingsMutex;
	// API
	// API Port for webserver, must restart app to change
	static int u_apiPort;
};