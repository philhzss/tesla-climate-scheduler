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
	static json settingsForm;
	// Initial breakdown into separate json objects
	static json teslaSettings, calendarSettings, generalSettings, carSettings, tempConfigs;


	// Important stuff, Teslemetry Token
	static string authHeader;
	static string teslemURL;
	

	// Parse settings.json and save its contents in the program's memory
	static void readSettings(string silent = "");

	// Write settings to file
	static void writeSettings(string key, bool value);


	// u_ser defined settings (from settings.json):

	// General Setting
	// Slack Channel for Slack notifications
	static string u_slackChannel;
	static bool slackEnabled;
	// General Setting
	// (Bool) If true, logs all logger message to file
	static bool u_logToFile;
	// General Setting
	// (Seconds) How long to wait before the program loops entirely
	static int u_repeatDelay;
	// General Setting
	// [lat, long] Home coordinates
	static std::vector<string> u_homeCoords;
	// General Setting
	// [lat, long] Work coordinates
	static std::vector<string> u_workCoords;


	// Tesla Account Setting
	// Teslemetry Token from console
	static string u_teslemToken;
	// Tesla VIN (also available in Teslemtry console
	static string u_teslaVIN;




	// Car Setting
	// Limiter is to stop climate if you called sick / if still at home X amount of time before shift in MINS
	static int u_shutoffTimer;
	// Car Setting
	// (default=5) (Minutes) The higher this is, the more time your car's HVAC will run. More details in docs
	static int u_default20CMinTime;
	// Car Setting
	// If inside car temperature is above this number and below noActivateUpperLimitTemp, HVAC won't activate
	static int u_noActivateLowerLimitTemp;
	// Car Setting
	// If inside car temperature is above noActivateLowerLimitTemp and below this number, HVAC won't activate
	static int u_noActivateUpperLimitTemp;	
	// Car Setting
	// The hottest temperature that will trigger heated seats level 3
	static int u_heatseat3temp;
	// Car Setting
	// The hottest temperature that will trigger heated seats level 2
	static int u_heatseat2temp;
	// Car Setting
	// The hottest temperature that will trigger heated seats level 1. Above this, no heated seats.
	static int u_heatseat1temp;
	// Car Setting
	// (commute + max HVAC preheat time + buffer) How long before shift start to wake car to check car temps
	static int wakeTimer;
	// Car Setting
	// (commute + HVAC preheat time) Determined based on temperature, settings.
	static int triggerTimer;
	// Car Setting
	// Real amount of time BEFORE shift (event) you leave from home
	static int shiftStartTimer;
	// Car Setting
	// Real amount of time AFTER shift (event) end you leave from work
	static int shiftEndTimer;


	// Calendar Setting
	// URL to Calendar file for event triggers
	static string u_calendarURL;
	// Calendar Setting
	// (Minutes) If you leave home to target arriving at work earlier than event start, enter a NEGATIVE number.
	// Ex: Want to arrive at work 15 mins before event start? Enter -15. EXCLUDES commute time. 
	// The car will be ready at (commuteTime - shiftStartBias) mins before event start.
	static int u_shiftStartBias;
	// Calendar Setting
	// (Minutes), if you leave work when you calendar event ends, this should be 0.
	// If you leave work early, enter a negative number, if late, positive number.
	static int u_shiftEndBias;
	// Calendar Setting
	// (Minutes) Drive time between home and work
	static int u_commuteTime;
	// Calendar Setting
	// Enter words that will cause the program to IGNORE events containing them
	static std::vector<string> u_wordsToIgnore;
	static bool ignoredWordsExist();
	static string ignoredWordsPrint();


	// API
	// Mutex lock for reading and clearing the settings file
	static std::mutex settingsMutex;
	// API
	// Returns true if can lock for this thread, false if timeout
	static bool settingsMutexLockSuccess(string reason, int customTimeSeconds=5);
	// API
	// API Port for webserver, must restart app to change
	static int u_apiPort;
	// API
	// Manually activate HVAC upon request
	static bool doManualActivateHVAC();
	// API
	// If not 0, will activate HVAC for (1,2) ppl next loop. Auto-resets
	static int numberOfSeatsActivateNow;


	// Temp configs
	// If false, all triggers will not activate HVAC (except manual requests)
	static bool u_allowTriggers;
	// Temp configs
	// If true, all triggers will force max defrost unless temp is above noDefrostAbove
	static bool u_encourageDefrost;
	// Temp configs
	// If true, all triggers will force max defrost unless temp is above noDefrostAbove
	static int u_noDefrostAbove;
};