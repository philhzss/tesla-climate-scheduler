#include <fstream>
#include <unistd.h>
#include "tesla_var.h"
#include "logger.h"
#include "calendar.h"

using std::string;

static Log lg("Main");

bool crashed = false;

time_t nowTime_secs = time(&nowTime_secs);

// Settings definitions
json settings::teslaSettings, settings::calendarSettings, settings::notifSettings, settings::carSettings;

// Car
int settings::intwakeTimer;
int settings::inttriggerTimer;
string settings::u_commuteTime;
int settings::intcommuteTime;
string settings::u_shutoffTimer;
int settings::intshutoffTimer;
string settings::u_garageEnabled;
string settings::u_garageBias;
// Notif
string settings::u_slackChannel;
// Cal
string settings::u_calendarURL;
string settings::shiftStartBias;
int settings::intshiftStartBias;
string settings::shiftEndBias;
int settings::intshiftEndBias;
// Tesla
string settings::u_teslaEmail;
string settings::u_teslaPassword;
string settings::u_teslaFiToken;
string settings::tfiURL;
string settings::teslaURL;
string settings::teslaHeader;





int Log::m_LogLevel;

// CURL stuff
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}
string curl_GET(string url)
{
	const char* const url_to_use = url.c_str();
	CURL* curl;
	CURLcode res;
	// Buffer to store result temporarily:
	string readBuffer;
	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();
	if (curl) {
		// The tFI URL is here, Phil:
		curl_easy_setopt(curl, CURLOPT_URL, url_to_use);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
		/* Perform the request, res will get the return code */
		res = curl_easy_perform(curl);
		/* Check for errors */
		if (res != CURLE_OK)
		{
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
				curl_easy_strerror(res));
			throw "curl_easy_perform() failed: " + std::to_string(res);
		}

		/* always cleanup */
		curl_easy_cleanup(curl);
	}
	curl_global_cleanup();
	return readBuffer;
}
string curl_POST(string url, string message)
{
	const char* const url_to_use = url.c_str();
	CURL* curl;
	CURLcode res;
	// Buffer to store result temporarily:
	string readBuffer;
	long response_code;

	/* In windows, this will init the winsock stuff */
	curl_global_init(CURL_GLOBAL_ALL);

	/* get a curl handle */
	curl = curl_easy_init();

	if (curl) {
		/* First set the URL that is about to receive our POST. This URL can
		   just as well be a https:// URL if that is what should receive the
		   data. */
		curl_easy_setopt(curl, CURLOPT_URL, url_to_use);
		/* Now specify the POST data */
		struct curl_slist* headers = nullptr;
		headers = curl_slist_append(headers, "Content-Type: application/json");

		string data = "{\"text\":\"" + message + "\"}";
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.length());
		curl_easy_setopt(curl, CURLOPT_POST, 1);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

		/* Perform the request, res will get the return code */
		res = curl_easy_perform(curl);
		/* Check for errors */
		if (res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
				curl_easy_strerror(res));
		if (res == CURLE_OK) {
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
		}

		/* always cleanup */
		curl_easy_cleanup(curl);
	}
	curl_global_cleanup();
	return std::to_string(response_code);
}

// Make sure internet connection works
bool InternetConnected() {
	lg.p("Running internet_connected() function");
	bool internet_res;
	CURL* curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, "http://www.google.com/");
		/* Disable the console output of the HTTP request data as we don't care: */
		curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
		int curlRes = curl_easy_perform(curl);
		if (curlRes == 0) {
			internet_res = true;
			lg.i("~Internet connection check successful~");
		}
		else {
			internet_res = false;
			lg.e("***** INTERNET DOWN *****");
			lg.e("***** INTERNET DOWN *****");
			lg.e("***** INTERNET DOWN *****");
			long response_code;
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
			lg.e("CURL Error Code: " + std::to_string(curlRes));
			lg.d("HTTP Error Code (will be 0 if absolutely can't connect): " + std::to_string(response_code));
			lg.d("See https://curl.haxx.se/libcurl/c/libcurl-errors.html for details.");
		}
	}
	lg.p("End of internet_connected() function\n");
	return internet_res;
}

// Time functions
const string return_current_time_and_date()
{
	struct tm tstruct;
	char buf[80];
	tstruct = *localtime(&nowTime_secs);
	if (Log::ReadLevel() == 3)
	{
		cout << "TEST NOW TIME STRUCT" << endl;
		cout << "nowTime_secs before conversions, should be the same after:" << endl;
		cout << nowTime_secs << endl;
		lg.d
		(
			"::TEST NOW TIME STRUCT::"
			"\nYear=" + (std::to_string(tstruct.tm_year)) +
			"\nMonth=" + (std::to_string(tstruct.tm_mon)) +
			"\nDay=" + (std::to_string(tstruct.tm_mday)) +
			"\nTime=" + (std::to_string(tstruct.tm_hour)) + ":" + (std::to_string(tstruct.tm_min)) + "\n"
		);
		time_t time_since_epoch = mktime(&tstruct);
		cout << "nowTime in seconds:" << endl;
		cout << time_since_epoch << endl << endl;
	}
	strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);
	return buf;
}
const string string_time_and_date(tm tstruct)
{
	time_t tempSeconds = mktime(&tstruct) - timezone;
	tm localStruct = *localtime(&tempSeconds);
	char buf[80];
	strftime(buf, sizeof(buf), "%Y-%m-%d %R Local", &localStruct);
	return buf;
}

void settings::readSettings(string silent = "")
{
	json settingsForm;
	try {
		std::ifstream stream("settings.json");
		stream >> settingsForm;

		// Get the settings cubcategories
		teslaSettings = settingsForm["Tesla & TeslaFi account settings"];
		calendarSettings = settingsForm["Calendar Settings"];
		notifSettings = settingsForm["Notification Settings"];
		carSettings = settingsForm["Car settings"];

		//Save the data from settings in the program variables
		{
			// CAR SETTINGS
			u_commuteTime = carSettings["commuteTime"];
			intcommuteTime = std::stoi(u_commuteTime);
			u_shutoffTimer = carSettings["shutoffTimer"];
			intshutoffTimer = std::stoi(u_shutoffTimer);
			u_garageEnabled = carSettings["garageEnabled"];
			u_garageBias = carSettings["garageBias"];

			// NOTIFICATION SETTINGS
			u_slackChannel = notifSettings["slackChannel"];

			// CALENDAR SETTINGS
			u_calendarURL = calendarSettings["calendarURL"];
			u_shiftStartBias = calendarSettings["shiftStartBias"];
			intshiftStartBias = std::stoi(u_shiftStartBias);
			u_shiftEndBias = calendarSettings["shiftEndBias"];
			intshiftEndBias = std::stoi(u_shiftEndBias);

			// TESLA & TESLAFI ACCOUNT SETTINGS
			u_teslaEmail = teslaSettings["teslaEmail"];
			u_teslaPassword = teslaSettings["teslaPassword"];
			u_teslaFiToken = teslaSettings["teslaFiToken"];
			lg.d("Settings file settings.json successfully read.");
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
			"\nCalendar URL: " + u_calendarURL +
			"\nTesla Email: " + u_teslaEmail +
			"\nTeslaFi Token: " + u_teslaFiToken +
			"\nCommute time setting: " + u_commuteTime + " minutes."
			"\nCar is therefore ready: \n" + std::to_string(intcommuteTime + intshiftEndBias) + " minutes relative to calendar event start,"
			"\nAnd " + u_shiftEndBias + " minutes relative to calendar event end time."
			"\nGarage mode enabled: " + u_garageEnabled + "."
			"\nHVAC will be shut down if car still home " + u_shutoffTimer + " minutes before shift start.\n"
		);
	}
	// Apply/calculate other setting values
	tfiURL = ("https://www.teslafi.com/feed.php?&token=" + u_teslaFiToken);
	teslaURL = "https://owner-api.teslamotors.com/";
	teslaHeader = {};

	// Default to 0 to avoid an error in eventTimeCheck, calculate after waketimer
	inttriggerTimer = 0;

	// Wake the care approx 45  mins before leaving for shift, to check car temp.
	intwakeTimer = intcommuteTime + 45;
	return;
}



int main()
{
	// From least to most info: LevelError(0), LevelInfo(1), LevelDebug(2), LevelProgramming(3).
	Log::SetLevel(Log::LogLevel::LevelDebug);

	try
	{
		// Read settings initially for Slack Channel (without output results)
		settings::readSettings("silent");
	}
	catch (string e) {
		lg.e("Critical failure (before start), program stopping: " + e, true);
		return EXIT_FAILURE;
	}

	lg.n("TCS app initiated" + tcs_versionInfo);


	// Set or reset initial variables
	lg.d("\n\"Crashed\" variable set to " + std::to_string(crashed));
	bool carblock_ran_success = false;
	lg.d("\"carblock_ran_success\" variable set to " + std::to_string(carblock_ran_success));
	lg.d("Reason: Initial values should be False\n\n");

	// Start of program, Always loop everything
	while (true) {
		cout << "\n>>>>>>>------------------------------PROGRAM STARTS HERE----------------------------<<<<<<<\n\n";
		// Everything is based on the time at program start
		lg.i("Runtime date-time: " + return_current_time_and_date() + " LOCAL\n");
		// Verify internet connection on every loop
		if (InternetConnected()) {
			try
			{
				// Only run the program if the settings file is readable, will throw exception if its not
				settings::readSettings();

				// Get & parse calendar data
				initiateCal();

				// Verify if any event matches the event checking parameters
				string actionToDo = calEvent::eventTimeCheck(settings::intwakeTimer, settings::inttriggerTimer);

				//testFunc();

			}
			catch (string e) {
				lg.e("Critical failure, program stopping: " + e, true);
				return EXIT_FAILURE;
			}


		}
		else {
			cout << "\nProgram requires internet to run, will keep retrying.";
		}
		cout << "\n<<<<<<<---------------------------PROGRAM TERMINATES HERE--------------------------->>>>>>>\n";
		cout << "\nWaiting for " << repeatDelay << " seconds...\n\n\n\n\n\n\n\n\n";
		cin.get();
		sleep(repeatDelay);
	}
	return 0;
}