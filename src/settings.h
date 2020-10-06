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


	// Parse settings.json and save its contents in the program's memory
	static void readSettings(string silent = "");




	// u_ser defined settings (from settings.json):

	// Car Setting
	// (Minutes) Drive time between home and work
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
	// Calendar Setting
	// (Minutes), if you leave home when you calendar event ends, this should be 0.
	// If you leave home to arrive earlier than event start, enter a negative number.
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
};