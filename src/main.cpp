#include <unistd.h>
#include "tesla_var.h"
#include "logger.h"
#include "calendar.h"
#include <iomanip>


using std::string;

static Log lg("Main", Log::LogLevel::Debug);
static Log lgw("WakeLoop", Log::LogLevel::Debug);



time_t nowTime_secs = time(&nowTime_secs);

// Should be blank by default
string actionToDo = "";


// Initialize car
car Tesla;




// CURL stuff
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
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
			lg.e("CURL Error Code: ", curlRes);
			lg.d("HTTP Error Code (will be 0 if absolutely can't connect): ", response_code);
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
	// VERIFY LOGGER HERE
	if (lg.ReadLevel() == Log::Programming)
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




int main()
{
	try
	{
		// Read settings initially for Slack Channel (without output results)
		settings::readSettings("silent");
	}
	catch (string e) {
		lg.en("Critical failure (before start), program stopping: " + e);
		return EXIT_FAILURE;
	}


	// Empty lines to make logging file more clear
	lg.b("\n.\n..\n...\n....\n.....\n......\n.......\n........\n.........\n..........\nTCS app initiated" + tcs_versionInfo + "\n");


	lg.i("\nTCS  Copyright (C) 2020  Philippe Hewett"
		"\nThis program comes with ABSOLUTELY NO WARRANTY;"
		"\nThis is free software, and you are welcome to redistribute it"
		"\nunder certain conditions.\n\n");
	lg.n("TCS app initiated" + tcs_versionInfo);


	// Start of program, Always loop everything
	while (true) {
		lg.b("\n>>>>>>>------------------------------PROGRAM STARTS HERE----------------------------<<<<<<<\n\n");
		// Everything is based on the time at program start
		nowTime_secs = time(&nowTime_secs); // update to current time
		lg.i("Runtime date-time (this loop): " + return_current_time_and_date() + " LOCAL\n");

		// Verify internet connection on every loop
		if (InternetConnected()) {
			try
			{
				// Only run the program if the settings file is readable, will throw exception if its not
				settings::readSettings();

				// Get & parse calendar data
				initiateCal();


				// Verify if any event matches the event checking parameters (Wake loop)
				bool carAwokenOnce = false;
				bool actionDone = false;
				bool actionCancelDone = false;
				int tempTimeMod;
				int wakeLoopTimer = settings::intrepeatDelay / 2; // by default repeat-delay/2
				do
				{
					if (actionToDo == "wake")
					{
						Tesla.getData(true); // Wake car and pull all data from it
						if (Tesla.carOnline) {
							if (!carAwokenOnce) {
								lgw.i("Car is awake and int temp is: " + Tesla.carData_s["inside_temp"]);
								tempTimeMod = Tesla.calcTempMod(std::stoi(Tesla.carData_s["inside_temp"]));
								lgw.in("Trigger ", tempTimeMod, " mins before depart time ",
									"if all parameters are met");
								lgw.i("Car seems to be located at ", Tesla.location);
								settings::inttriggerTimer = settings::intcommuteTime + tempTimeMod;
								carAwokenOnce = true;
							}
							else { lgw.i("waiting"); }
						}
						else {
							lgw.e("Could not wake car?? Car still reporting as offline after wakeLoop");
							// Could print a raw tesla-feed here for debugging if this triggers one day?
						}
					}
					else if ((actionToDo == "home") || (actionToDo == "work"))
					{
						Tesla.getData(true); // Make sure you have most up to date data with car awake
						lgw.i("Event triggered: ", actionToDo, ", car location: ", Tesla.location);
						if (actionToDo == Tesla.location)
						{
							if (!actionDone) // only here to retry action if car moved/stopped driving
							{
								lgw.i("EVENT & LOCATION VALID: (", actionToDo, ")");
								if (Tesla.carOnline && Tesla.Tshift_state == "P")
								{
									json hvac_result = Tesla.teslaPOST(settings::teslaVURL + "command/auto_conditioning_start");
									bool state_after_hvac = hvac_result["result"]; // should return true

									std::vector<string> seats_defrost = Tesla.coldCheckSet();
									string firstWord = (seats_defrost[1] == "1") ? "MAX DEFROST" : "HVAC";
									lg.in(seats_defrost[0]);
									lg.in(seats_defrost[1]);

									if (state_after_hvac)
									{
										// If we're here, it's success
										lgw.b();
										lgw.in(firstWord, " ON\nSeat Heater: ", seats_defrost[0], "\nInside Temp: ",
											Tesla.Tinside_temp, "C\nOutside Temp: ", Tesla.Toutside_temp, "C");


										// Update the triggered event to prevent it from re-running
										if (actionToDo == "home") { calEvent::lastTriggeredEvent->homeDone = true; }
										if (actionToDo == "work") { calEvent::lastTriggeredEvent->workDone = true; }

									}
									else
									{
										lgw.en("Could not turn HVAC on??");
									}
									actionDone = true;
								}
								else
								{
									lgw.in("Event and location was valid, but carOnline: ", Tesla.carOnline, " and",
										" shift_state: ", Tesla.Tshift_state);
								}
							}
						}
						else
						{
							if (!actionCancelDone)
							{
								lgw.in("Event triggered for ", actionToDo, ", but car location is ", Tesla.location,
									", not activating HVAC.");
								actionCancelDone = true;
							}
						}
					}
					else if (actionToDo == "duplicate")
					{
						wakeLoopTimer = 180; // wait a bit longer to avoid cluttering log file
						string kindOfEvent;
						tm datetime;
						if (calEvent::lastTriggeredEvent->homeDone)
						{
							kindOfEvent = "home (shift start)";
							datetime = calEvent::lastTriggeredEvent->start;
						}
						else if (calEvent::lastTriggeredEvent->workDone)
						{
							kindOfEvent = "work (shift end)";
							datetime = calEvent::lastTriggeredEvent->end;
						}
						lg.d("This event trigger for ", kindOfEvent, " has already ran.");
						lg.d("The event trigger datetime is:"
							"\nYear=", datetime.tm_year,
							"\nMonth=", datetime.tm_mon,
							"\nDay=", datetime.tm_mday,
							"\nTime=", datetime.tm_hour, ":", datetime.tm_min, "\n");
					}
					// settings::intwakeTimer = 3900; // for testing
					// settings::inttriggerTimer = 3624; // for testing
					lgw.d("Running eventTimeCheck with wakeTimer: ", settings::intwakeTimer,
						"mins, triggerTimer: ", settings::inttriggerTimer, "mins");
					lgw.d("triggerTimer might not be calculated if wakeTimer hasn't run yet!!");
					actionToDo = calEvent::eventTimeCheck(settings::intwakeTimer, settings::inttriggerTimer);
					// actionToDo = "home"; // for testing
					if (!actionToDo.empty())
					{
						lgw.i("Waiting ", wakeLoopTimer, " seconds and re-running wakeLoop");
						sleep(wakeLoopTimer);
					}
				} while (!actionToDo.empty());

			}
			catch (string e) {
				lg.en("Critical failure, program stopping: " + e);
				return EXIT_FAILURE;
			}


		}
		else {
			lg.i("\nProgram requires internet to run, will keep retrying.");
		}
		lg.b("\n<<<<<<<---------------------------PROGRAM TERMINATES HERE--------------------------->>>>>>>\n");
		calEvent::cleanup(); // to avoid calendar vectors overflowing

		lg.i("\nWaiting for " + settings::u_repeatDelay + " seconds...\n\n\n\n\n\n\n\n\n");
		sleep(settings::intrepeatDelay);
	}
	return 0;
}