#include "logger.h"
#include "settings.h"

static Log lg("Settings");



// Settings definitions
json settings::teslaSettings, settings::calendarSettings, settings::generalSettings, settings::carSettings;

// General
string settings::u_slackChannel;
string settings::u_logToFile;
string settings::u_repeatDelay;
int settings::intrepeatDelay;
// Car
int settings::intwakeTimer;
int settings::inttriggerTimer;
string settings::u_commuteTime;
int settings::intcommuteTime;
string settings::u_shutoffTimer;
int settings::intshutoffTimer;
string settings::u_default20CMinTime;
int settings::intdefault20CMinTime;
// Cal
string settings::u_calendarURL;
string settings::u_shiftStartBias;
int settings::intshiftStartBias;
string settings::u_shiftEndBias;
int settings::intshiftEndBias;
string settings::u_ignoredWord1;
string settings::u_ignoredWord2;
// Tesla
string settings::u_teslaEmail;
string settings::u_teslaPassword;
string settings::u_teslaFiToken;
json settings::teslaAuthHeader;
string settings::tfiURL;
string settings::teslaURL;
string settings::teslaHeader;
string settings::u_teslaClientID;
string settings::u_teslaClientSecret;
json settings::authPackage;


void settings::readSettings(string silent)
{
	json settingsForm;
	try {
		std::ifstream stream("settings.json");
		stream >> settingsForm;

		// Get the settings cubcategories
		teslaSettings = settingsForm["Tesla & TeslaFi account settings"];
		calendarSettings = settingsForm["Calendar Settings"];
		generalSettings = settingsForm["General Settings"];
		carSettings = settingsForm["Car settings"];

		//Save the data from settings in the program variables
		{
			// CAR SETTINGS
			u_commuteTime = carSettings["commuteTime"];
			intcommuteTime = std::stoi(u_commuteTime);
			u_shutoffTimer = carSettings["shutoffTimer"];
			intshutoffTimer = std::stoi(u_shutoffTimer);
			u_default20CMinTime = carSettings["default20CMinTime"];
			intdefault20CMinTime = std::stoi(u_default20CMinTime);

			// GENERAL SETTINGS
			u_slackChannel = generalSettings["slackChannel"];
			u_logToFile = generalSettings["logToFile"];
			u_repeatDelay = generalSettings["repeatDelay"];
			intrepeatDelay = std::stoi(u_repeatDelay);

			// CALENDAR SETTINGS
			u_calendarURL = calendarSettings["calendarURL"];
			u_shiftStartBias = calendarSettings["shiftStartBias"];
			intshiftStartBias = std::stoi(u_shiftStartBias);
			u_shiftEndBias = calendarSettings["shiftEndBias"];
			intshiftEndBias = std::stoi(u_shiftEndBias);
			u_ignoredWord1 = calendarSettings["ignoredWord1"];
			u_ignoredWord2 = calendarSettings["ignoredWord2"];

			// TESLA & TESLAFI ACCOUNT SETTINGS
			u_teslaEmail = teslaSettings["teslaEmail"];
			u_teslaPassword = teslaSettings["teslaPassword"];
			u_teslaFiToken = teslaSettings["teslaFiToken"];
			u_teslaClientID = teslaSettings["TESLA_CLIENT_ID"];
			u_teslaClientSecret = teslaSettings["TESLA_CLIENT_SECRET"];
			lg.d("Settings file settings.json successfully read.");

			settings::authPackage =
			{ {"grant_type", "password"},
			{"client_id", settings::u_teslaClientID},
			{"client_secret", settings::u_teslaClientSecret},
			{"email", settings::u_teslaEmail},
			{"password", settings::u_teslaPassword} };
		}
	}
	catch (nlohmann::detail::parse_error)
	{
		lg.e("readSettings json parsing error, verify that settings.json exists in same directory as binary", true);
		throw string("settings.json parse_error");
	}
	catch (nlohmann::detail::type_error)
	{
		lg.e("readSettings json type error, settings.json most likely corrupt, verify example", true);
		throw string("settings.json type_error");
	}

	// Print this unless reading settings silently
	if (silent != "silent")
	{
		lg.i("Settings imported from settings.json:"
			"\nSlack Channel: " + u_slackChannel +
			"\nLogging to file: " + u_logToFile +
			"\nCalendar URL: " + u_calendarURL +
			"\nCalendar words to ignore (0-2): " + settings::u_ignoredWord1 + ", " + settings::u_ignoredWord2 +
			"\nTesla Email: " + u_teslaEmail +
			"\nTeslaFi Token: " + u_teslaFiToken +
			"\nCommute time setting: " + u_commuteTime + " minutes."
			"\nCar is therefore ready: \n" + std::to_string(-intcommuteTime - intshiftStartBias) + " minutes relative to calendar event start,"
			"\nAnd " + u_shiftEndBias + " minutes relative to calendar event end time."
			"\nHVAC will be shut down if car still home " + u_shutoffTimer + " minutes before shift start."
			"\nDefault time value @ 20C interior temp: " + u_default20CMinTime + " minutes.\n"
		);
	}
	// Apply/calculate other setting values
	tfiURL = ("https://www.teslafi.com/feed.php?&token=" + u_teslaFiToken);
	teslaURL = "https://owner-api.teslamotors.com/";
	teslaHeader = {};

	// Default to 0 to avoid an error in eventTimeCheck, calculate after waketimer
	inttriggerTimer = 0;

	// Wake the care approx 45  mins before leaving for shift, to check car temp.
	// Max HVAC time should probably not exceed ~30-35 mins (excl commute), so waking car 45 mins earlier
	intwakeTimer = intcommuteTime + 45;
	return;
}