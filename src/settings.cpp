#include "logger.h"
#include "settings.h"
#include <thread>

static Log lg("Settings", Log::LogLevel::Debug);



// Settings definitions
json settings::settingsForm;
json settings::teslaSettings, settings::calendarSettings, settings::generalSettings, settings::carSettings, settings::tempConfigs;

// General
string settings::u_slackChannel;
bool settings::slackEnabled;
bool settings::u_logToFile;
int settings::u_repeatDelay;
std::vector<string> settings::u_homeCoords;
std::vector<string> settings::u_workCoords;
int settings::u_apiPort;
bool settings::u_allowTriggers;
int settings::numberOfSeatsActivateNow;
// Car
int settings::wakeTimer;
int settings::triggerTimer;
int settings::u_shutoffTimer;
int settings::u_default20CMinTime;
int settings::u_noActivateLowerLimitTemp;
int settings::u_noActivateUpperLimitTemp;
int settings::u_heatseat3temp;
int settings::u_heatseat2temp;
int settings::u_heatseat1temp;
int settings::shiftStartTimer;
int settings::shiftEndTimer;
// Cal
string settings::u_calendarURL;
int settings::u_shiftStartBias;
int settings::u_shiftEndBias;
int settings::u_commuteTime;
std::vector<string> settings::u_wordsToIgnore;
// Tesla
string settings::teslaOwnerURL;
string settings::teslaAuthURL;
string settings::teslaVURL;
string settings::teslaVID;
string settings::teslaAuthString;
string settings::u_teslaAccessToken;
string settings::u_teslaRefreshToken;


void settings::readSettings(string silent)
{
	try {
		std::ifstream stream("settings.json");
		stream >> settingsForm;

		// Get the settings cubcategories
		teslaSettings = settingsForm["Tesla account settings"];
		calendarSettings = settingsForm["Calendar Settings"];
		generalSettings = settingsForm["General Settings"];
		carSettings = settingsForm["Car settings"];
		tempConfigs = settingsForm["Temporary Configurables"];

		//Save the data from settings in the program variables
		{
			// GENERAL SETTINGS
			u_slackChannel = generalSettings["slackChannel"];
			u_logToFile = generalSettings["logToFile"];
			u_repeatDelay = generalSettings["repeatDelay"];
			generalSettings["homeCoords"].get_to(u_homeCoords);
			generalSettings["workCoords"].get_to(u_workCoords);
			u_apiPort = generalSettings["apiPort"];


			// CAR SETTINGS
			u_shutoffTimer = carSettings["shutoffTimer"];
			u_default20CMinTime = carSettings["default20CMinTime"];
			u_noActivateLowerLimitTemp = carSettings["noActivateLowerLimitTemp"];
			u_noActivateUpperLimitTemp = carSettings["noActivateUpperLimitTemp"];
			u_heatseat3temp = carSettings["heatseat3IfColderThan"];
			u_heatseat2temp = carSettings["heatseat2IfColderThan"];
			u_heatseat1temp = carSettings["heatseat1IfColderThan"];


			// CALENDAR SETTINGS
			u_commuteTime = calendarSettings["commuteTime"];
			u_calendarURL = calendarSettings["calendarURL"];
			u_shiftStartBias = calendarSettings["shiftStartBias"];
			u_shiftEndBias = calendarSettings["shiftEndBias"];
			calendarSettings["wordsToIgnore"].get_to(u_wordsToIgnore);

			// TESLA ACCOUNT SETTINGS
			u_teslaAccessToken = teslaSettings["teslaAccessToken"];
			u_teslaRefreshToken = teslaSettings["teslaRefreshToken"];

			// TEMP CONFIG SETTINGS
			u_allowTriggers = tempConfigs["allowTriggers"];

			// Get and set the auth string directly from settings
			settings::teslaAuthString = "Authorization: Bearer " + u_teslaAccessToken;

			lg.b();
			lg.d("Settings file settings.json successfully read.");

		}
	}
	catch (nlohmann::detail::parse_error)
	{
		lg.en("readSettings json parsing error, verify that settings.json exists in same directory as binary");
		throw string("settings.json parse_error");
	}
	/*catch (nlohmann::detail::type_error)
	{
		lg.en("readSettings json type error, settings.json most likely corrupt, verify example");
		throw string("settings.json type_error");
	}*/

	// Calculate shift timers (real time before & after events car has to be ready for)
	shiftStartTimer = -u_commuteTime + u_shiftStartBias;
	shiftEndTimer = u_shiftEndBias;


	// Print this unless reading settings silently
	if (silent != "silent")
	{
		lg.b();

		lg.i("Settings imported from settings.json:"
			"\nGen: Slack Channel: " + u_slackChannel +
			"\nGen: Logging to file: ", u_logToFile,
			"\nGen: Program repeats every ", settings::u_repeatDelay, " seconds");
		if (settings::ignoredWordsExist())
		{
			string ignoredString = settings::ignoredWordsPrint();
			lg.b("Cal: Calendar URL: " + u_calendarURL +
				"\nCal: Word(s) to ignore events in calendar (", u_wordsToIgnore.size(), "): " + ignoredString);
		}
		else {
			lg.b("Cal: Calendar URL: " + u_calendarURL);
			lg.b("No ignored words were specified.");
		}
		lg.b("Cal: Commute time setting: ", u_commuteTime, " minutes.");
		// Scope for clarity
		{
			// Keywords for better logging
			string startKw;
			string endKw;
			startKw = (shiftStartTimer < 0) ? "BEFORE" : "AFTER";
			endKw = (shiftEndTimer < 0) ? "BEFORE" : "AFTER";
			
			if (u_allowTriggers) {
				lg.b("\nCar will be ready for driving: \n", abs(shiftStartTimer), " minutes ", startKw, " calendar event start time."
					"\n", abs(shiftEndTimer), " minutes ", endKw, " calendar event end time."
					"\nHVAC will be shut down if car still home ", u_shutoffTimer, " minutes before shift start."
				);
			}
			else {
				lg.b("Scheduled triggers are currently BLOCKED, only manual HVAC requests will trigger!!!"
				);
			}
			lg.b("Car: Default time value @ 20C interior temp: ", u_default20CMinTime, " minutes.");
		}
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

	teslaOwnerURL = "https://owner-api.teslamotors.com/";
	teslaAuthURL = "https://auth.tesla.com/oauth2/v3/";
	// Note: Tesla VID and VURL defined in carblock

	// Default to the bare minimum (2mins is tempTimeMod min) to avoid an error in eventTimeCheck, calculate after waketimer has obtained the car temp.
	triggerTimer = 2;


	// Figure out if its event start or event end that makes the car HVAC start earlier
	int longest_timer = (std::abs(shiftStartTimer) > std::abs(shiftEndTimer) ? shiftStartTimer : shiftEndTimer);
	lg.p("Longest timer (shift start or end): ", longest_timer, " minutes.");

	// Longest timer is often negative as its a DELTA time to the end/start of event on cal.
	// Wake the care approx 5  mins before the earliest it would need to turn on HVAC, to check car temp.
	// No point in waking up car earlier than: -longest_timer + max TempTimeModifier(32) + 5 min buffer
	wakeTimer = -longest_timer + 32 + 5;
	lg.p("Wake timer (longest timer + 32 + 5): ", wakeTimer, " minutes.");
	return;
}

bool settings::ignoredWordsExist()
{
	return (u_wordsToIgnore.empty()) ? false : true;
}

string settings::ignoredWordsPrint()
{
	auto* comma = ", ";
	auto* sep = "";
	std::ostringstream stream;
	for (string word : u_wordsToIgnore)
	{
		stream << sep << word;
		sep = comma;
	}
	return stream.str();
}

bool settings::settingsMutexLockSuccess(string reason, int customTimeSeconds) {
	int counter = 0;
	while (!settings::settingsMutex.try_lock()) {
		lg.d("Mutex locked (", reason, "), WAITING FOR UNLOCK, have looped ", counter, " times.");
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
		counter++;
		// Enter seconds*5 (it waits for 200ms per loop, 5*200ms = 1s)
		if (counter > customTimeSeconds * 5) {
			lg.e("Mutex timer (", reason, ") overlimit, settingsMutexLockSuccess returning LockSuccess=FALSE");
			return false;
		}
	}
	return true;
}


bool settings::doManualActivateHVAC() {
	// Get the mutex before reading settings value
	if (!settings::settingsMutexLockSuccess("before checking doManualActivateHVAC?")) {
		throw "Mutex timeout in main thread (before checking doManualActivateHVAC?)";
	}
	lg.p("!!!MAIN: MUTEX LOCKED (before checking doManualActivateHVAC?)!!!");

	bool triggerHVAC = false;

	if (settings::numberOfSeatsActivateNow != 0) {
		triggerHVAC = true;
	}

	settings::settingsMutex.unlock();
	lg.p("Mutex UNLOCKED by main after checking doManualActivateHVAC?");

	return triggerHVAC;
}


void settings::writeSettings(string key, bool value)
{
	readSettings("silent"); // Make sure it's up to date

	settingsForm["Temporary Configurables"][key] = value;

	std::ofstream file("settings.json");
	file << std::setw(4) << settingsForm;
	file.close();

	lg.d("Wrote ",key, ":",value, " to Temporary Configurables");

	readSettings("silent"); // Re-update the settings in program from file
}