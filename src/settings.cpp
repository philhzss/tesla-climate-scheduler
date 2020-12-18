#include "logger.h"
#include "settings.h"

static Log lg("Settings", Log::LogLevel::Debug);



// Settings definitions
json settings::teslaSettings, settings::calendarSettings, settings::generalSettings, settings::carSettings;

// General
string settings::u_slackChannel;
bool settings::slackEnabled;
string settings::u_logToFile;
string settings::u_repeatDelay;
int settings::intrepeatDelay;
std::vector<string> settings::u_homeCoords;
std::vector<string> settings::u_workCoords;
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
std::vector<string> settings::u_wordsToIgnore;
// Tesla
string settings::u_teslaEmail;
string settings::u_teslaPassword;
string settings::teslaURL;
string settings::teslaVURL;
string settings::teslaVID;
string settings::u_teslaClientID;
string settings::u_teslaClientSecret;
json settings::authReqPackage;
string settings::teslaAuthString;


void settings::readSettings(string silent)
{
	json settingsForm;
	try {
		std::ifstream stream("settings.json");
		stream >> settingsForm;

		// Get the settings cubcategories
		teslaSettings = settingsForm["Tesla account settings"];
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
			generalSettings["homeCoords"].get_to(u_homeCoords);
			generalSettings["workCoords"].get_to(u_workCoords);


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
			calendarSettings["wordsToIgnore"].get_to(u_wordsToIgnore);

			// TESLA ACCOUNT SETTINGS
			u_teslaEmail = teslaSettings["teslaEmail"];
			u_teslaPassword = teslaSettings["teslaPassword"];
			u_teslaClientID = teslaSettings["TESLA_CLIENT_ID"];
			u_teslaClientSecret = teslaSettings["TESLA_CLIENT_SECRET"];

			lg.b();
			lg.d("Settings file settings.json successfully read.");

			settings::authReqPackage =
			{ {"grant_type", "password"},
			{"client_id", settings::u_teslaClientID},
			{"client_secret", settings::u_teslaClientSecret},
			{"email", settings::u_teslaEmail},
			{"password", settings::u_teslaPassword} };
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
	intshiftStartTimer = -intcommuteTime + intshiftStartBias;
	intshiftEndTimer = intshiftEndBias;


	// Print this unless reading settings silently
	if (silent != "silent")
	{
		lg.b();

		lg.i("Settings imported from settings.json:"
			"\nGen: Slack Channel: " + u_slackChannel +
			"\nGen: Logging to file: " + u_logToFile +
			"\nGen: Program repeats every ", settings::intrepeatDelay, " seconds");
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
		// Scope for clarity
		{
			string startKw;
			string endKw;
			startKw = (intshiftStartTimer < 0) ? "BEFORE" : "AFTER";
			endKw = (intshiftEndTimer < 0) ? "BEFORE" : "AFTER";
			lg.b("Car: Tesla Account Email: " + u_teslaEmail +
				"\nCar: Commute time setting: " + u_commuteTime + " minutes."
				"\nCar will be ready for driving: \n", abs(intshiftStartTimer), " minutes ", startKw, " calendar event start time."
				"\n", abs(intshiftEndTimer), " minutes ", endKw, " calendar event end time."
				"\nHVAC will be shut down if car still home " + u_shutoffTimer + " minutes before shift start."
				"\nCar: Default time value @ 20C interior temp: " + u_default20CMinTime + " minutes."
			);
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

	teslaURL = "https://owner-api.teslamotors.com/";
	// Note: Tesla VID and VURL defined in carblock

	// Default to the bare minimum to avoid an error in eventTimeCheck, calculate after waketimer has obtained the car temp.
	inttriggerTimer = intcommuteTime + 1;


	// Figure out if its event start or event end that makes the car HVAC start earlier
	int longest_timer = (std::abs(intshiftStartTimer) > std::abs(intshiftEndTimer) ? intshiftStartTimer : intshiftEndTimer);
	lg.p("Longest timer (shift start or end): ", longest_timer, " minutes.");

	// Longest timer is often negative as its a DELTA time to the end/start of event on cal.
	// Wake the care approx 5  mins before the earliest it would need to turn on HVAC, to check car temp.
	// No point in waking up car earlier than: -longest_timer + max TempTimeModifier(32) + 5 min buffer
	intwakeTimer = -longest_timer + 32 + 5;
	lg.p("Wake timer (longest timer + 32 + 5): ", intwakeTimer, " minutes.");
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

