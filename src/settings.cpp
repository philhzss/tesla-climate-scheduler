#include "logger.h"
#include "settings.h"

static Log lg("Settings");



// Settings definitions
json settings::teslaSettings, settings::calendarSettings, settings::generalSettings, settings::carSettings;

// General
string settings::u_slackChannel;
bool settings::slackEnabled;
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
int settings::intshiftStartTimer;
int settings::intshiftEndTimer;
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
			// GENERAL SETTINGS
			u_slackChannel = generalSettings["slackChannel"];
			u_logToFile = generalSettings["logToFile"];
			u_repeatDelay = generalSettings["repeatDelay"];
			intrepeatDelay = std::stoi(u_repeatDelay);

			// CAR SETTINGS
			u_commuteTime = carSettings["commuteTime"];
			intcommuteTime = std::stoi(u_commuteTime);
			u_shutoffTimer = carSettings["shutoffTimer"];
			intshutoffTimer = std::stoi(u_shutoffTimer);
			u_default20CMinTime = carSettings["default20CMinTime"];
			intdefault20CMinTime = std::stoi(u_default20CMinTime);
		

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

	// Calculate shift timers (real time before & after events car has to be ready for)
	intshiftStartTimer = -intcommuteTime - intshiftStartBias;
	intshiftEndTimer = intshiftEndBias;


	// Print this unless reading settings silently
	if (silent != "silent")
	{
		lg.b();
		lg.i("Settings imported from settings.json:"
			"\nSlack Channel: " + u_slackChannel +
			"\nLogging to file: " + u_logToFile +
			"\nCalendar URL: " + u_calendarURL +
			"\nCalendar words to ignore (0-2): " + settings::u_ignoredWord1 + ", " + settings::u_ignoredWord2 +
			"\nTesla Email: " + u_teslaEmail +
			"\nTeslaFi Token: " + u_teslaFiToken +
			"\nCommute time setting: " + u_commuteTime + " minutes."
			"\nCar is therefore ready: \n" + std::to_string(intshiftStartTimer) + " minutes relative to calendar event start, which is "
			"\nAnd " + std::to_string(intshiftEndTimer) + " minutes relative to calendar event end time. which is "
			"\nHVAC will be shut down if car still home " + u_shutoffTimer + " minutes before shift start."
			"\nDefault time value @ 20C interior temp: " + u_default20CMinTime + " minutes."
		);
	}

	// Check if Slack channel is empty
	if (u_slackChannel.empty())
	{
		slackEnabled = false;
		if (silent != "silent")
			lg.i("Mobile notifications are disabled.\n");
	}
	else
	{
		slackEnabled = true;
		lg.b();
	}

	// Apply/calculate other setting values
	tfiURL = ("https://www.teslafi.com/feed.php?&token=" + u_teslaFiToken);
	teslaURL = "https://owner-api.teslamotors.com/";
	teslaHeader = {};

	// Default to 0 to avoid an error in eventTimeCheck, calculate after waketimer has obtained the car temp.
	inttriggerTimer = 0;


	// Figure out if its event start or event end that makes the car HVAC start earlier
	int longest_timer = (intshiftStartTimer > intshiftEndTimer ? intshiftStartTimer : intshiftEndTimer);
	lg.p(std::to_string(longest_timer));
	
	// Longest timer is often negative as its a DELTA time to the end/start of event on cal.
	// Wake the care approx 5  mins before the earliest it would need to turn on HVAC, to check car temp.
	// No point in waking up car earlier than: -longest_timer + max TempTimeModifier(32) + 5 min buffer
	intwakeTimer = -longest_timer + 32 + 5;
	lg.p(std::to_string(intwakeTimer));
	return;
}