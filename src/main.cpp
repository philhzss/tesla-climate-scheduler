#include <unistd.h>
#include "tesla_var.h"
#include "logger.h"
#include "calendar.h"
#include <iomanip>

#define CROW_MAIN
#include <crow.h>


using std::string;

static Log lg("Main", Log::LogLevel::Debug);
static Log lgw("WakeLoop", Log::LogLevel::Debug);

std::mutex settings::settingsMutex;


time_t nowTime_secs = time(&nowTime_secs);

// Should be blank by default
string actionToDo = "";


// Initialize car
car Tesla;


// Wait for X number of time, checking the API for a request every second. Silent (no log)
void sleepWithAPIcheck(int totalSleepTime) {
	for (int i = 1; i < totalSleepTime; ++i) {
		// Get the mutex before reading the numberOfSeatsActivateNow value
		if (!settings::settingsMutexLockSuccess("before checking numberOfSeatsActivateNow")) {
			throw "Mutex timeout in main thread (before checking numberOfSeatsActivateNow)";
		}
		if (settings::numberOfSeatsActivateNow) {
			settings::settingsMutex.unlock(); // Always release
			// Stop the sleeping if the value is not 0
			break;
		}
		else {
			settings::settingsMutex.unlock(); // Always release

		}
		sleep(1);
	}
	return;
}



// CURL stuff
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

size_t header_callback(char* buffer, size_t size,
	size_t nitems, void* userdata)
{
	/* received header is nitems * size long in 'buffer' NOT ZERO TERMINATED */
	/* 'userdata' is set with CURLOPT_HEADERDATA */
	return nitems * size;
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
		/* always cleanup */
		curl_easy_cleanup(curl);
	}
	curl_global_cleanup();

	lg.p("End of internet_connected() function\n");
	return internet_res;
}

// Time functions
const string date_time_str_from_time_t(const char * format, time_t * time_t_secs)
{
	struct tm tstruct;
	char buf[80];
	if (time_t_secs == nullptr) {
		tstruct = *localtime(&nowTime_secs);
	}
	else {
		tstruct = *localtime(time_t_secs);
	}
	
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
	strftime(buf, sizeof(buf), format, &tstruct);
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

void DoCrowAPI(car* carPointer) {
	crow::SimpleApp app; //define your crow application

	//define your endpoint at the root directory
	CROW_ROUTE(app, "/api")([carPointer]() {
		crow::json::wvalue json;
		lg.d("Crow HTTP request");
		if (!settings::settingsMutexLockSuccess("crow request")) {
			return json["app"] = "ERROR";
		}
		lg.d("!!!CROW: Mutex LOCKED!!!");

		string carName = lg.prepareOnly(carPointer->Tdisplay_name);
		json["app"]["car_name"] = carName;
		json["app"]["TCS_version"] = tcs_version;

		json[carName]["last_car_data_update"] = lg.prepareOnly(carPointer->teslaDataUpdateTime);
		json[carName]["state_shift_gear"] = lg.prepareOnly(carPointer->Tshift_state);
		json[carName]["state_connection"] = lg.prepareOnly(carPointer->Tconnection_state);
		json[carName]["climate_temp_inside"] = lg.prepareOnly(carPointer->Tinside_temp);
		json[carName]["climate_temp_outside"] = lg.prepareOnly(carPointer->Toutside_temp);
		json[carName]["climate_driver_temp_setting"] = lg.prepareOnly(carPointer->Tdriver_temp_setting);
		json[carName]["climate_is_on"] = lg.prepareOnly(carPointer->Tis_climate_on);
		json[carName]["battery_level_usable"] = lg.prepareOnly(carPointer->Tusable_battery_level);
		json[carName]["battery_level"] = lg.prepareOnly(carPointer->Tbattery_level);



		settings::settingsMutex.unlock();
		lg.d("Mutex UNLOCKED by crow");


		return json;
		});


	CROW_ROUTE(app, "/activate")
		([](const crow::request& req) {

		lg.d("Crow HTTP request");
		if (!settings::settingsMutexLockSuccess("crow ACTIVATION request")) {
			return crow::response{ "ERROR MUTEX TIMEOUT" };
		}
		lg.d("!!!CROW: Mutex LOCKED!!!");

		string apiReturn;
		int configValue = 0;
		// To get a simple string from the url params
		// To see it in action /params?foo='blabla'
		if (req.url_params.get("ppl") != nullptr) {
			string inputClientValue = req.url_params.get("ppl");

			apiReturn = lg.prepareOnly("Received value ppl = ", inputClientValue, ". Processing request.");

			// Verify validity (if ppl was sent from client)
			if (inputClientValue == "1") {
				configValue = 1;
			}
			else if (inputClientValue == "2") {
				configValue = 2;
			}
			else {
				apiReturn = lg.prepareOnly("ppl value specified was invalid (", inputClientValue, ")");
				configValue = 0;
			}
		}
		else {
			apiReturn = "Incorrect or missing params";
		}
		lg.d(apiReturn);
		settings::numberOfSeatsActivateNow = configValue;

		settings::settingsMutex.unlock();
		lg.d("Mutex UNLOCKED by crow");

		return crow::response{ apiReturn };
			});


	// set the port, set the app to run on multiple threads, and run the app
	app.port(settings::u_apiPort).multithreaded().run();
	// app.port(30512).multithreaded().run();
	// 20512 main port, 30512 test port to not interfere with running version
}




int main()
{
	try
	{
		// Read settings initially for Slack Channel & API (without output results)
		settings::readSettings("silent");
	}
	catch (string e) {
		lg.en("Critical failure (before start), program stopping: " + e);
		return EXIT_FAILURE;
	}

	std::thread worker(DoCrowAPI, &Tesla);

	// Empty lines to make logging file more clear
	lg.b("\n.\n..\n...\n....\n.....\n......\n.......\n........\n.........\n..........\nTCS app initiated" + tcs_versionInfo + "\n");


	lg.i("\nTCS  Copyright (C) 2022  Philippe Hewett"
		"\nThis program comes with ABSOLUTELY NO WARRANTY;"
		"\nThis is free software, and you are welcome to redistribute it"
		"\nunder certain conditions.\n\n");
	lg.n("TCS app initiated" + tcs_versionInfo);

	int mainLoopCounter = 1;

	Tesla.getData(true); // Wake car and pull all data from it initially for API

	// Start of program, Always loop everything
	while (true) {
		lg.b("\n\n>>>>>>>------------------------------PROGRAM STARTS HERE----------------------------<<<<<<<\n");
		lg.b("TCS Version ", tcs_version);
		// Everything is based on the time at program start
		nowTime_secs = time(&nowTime_secs); // update to current time
		lg.i("Runtime date-time (this loop): " + date_time_str_from_time_t() + " LOCAL\n");
		lg.d("Loop run number since program start: ", mainLoopCounter);

		// Verify internet connection on every loop
		if (InternetConnected()) {
			// Count tries if error occurs and quit eventually
			int count = 0;
			int maxTries = 10;
			while (true)
			{
				try
				{
					// Only run the program if the settings file is readable, will throw exception if its not
					settings::readSettings();

					// Get & parse calendar data
					initiateCal();


					// Only run the wake loop if no manual activation is to be done
					if (!settings::doManualActivateHVAC())
					{
						// Verify if any event matches the event checking parameters (Wake loop)
						bool carAwokenOnce = false;
						bool actionDone = false;
						bool actionCancelDone = false;
						bool shutoffHasBeenCheckedOnce = false;
						int tempTimeMod;
						int wakeLoopTimer = settings::intrepeatDelay / 2; // by default repeat-delay/2
						// wakeLoopTimer = 5; // For Testing
						do
						{
							// Break out of loop if manual HVAC
							if (settings::doManualActivateHVAC()) {
								lgw.d("Breaking out of wake loop, manual HVAC activation requested");
								break;
							}

							nowTime_secs = time(&nowTime_secs); // always update to current time!
							calEvent::updateValidEventTimers(); // always update timers within wakeLoop!

							if (actionToDo != "")
							{
								lg.d("actionToDo is: ", actionToDo);
							}

							if (actionToDo == "wake")
							{
								if (!carAwokenOnce) {
									Tesla.getData(true); // Wake car and pull all data from it
									if (Tesla.carOnline) {
										lgw.i("Car is awake and int temp is: " + Tesla.carData_s["inside_temp"]);
										tempTimeMod = Tesla.calcTempMod(std::stoi(Tesla.carData_s["inside_temp"]));
										settings::inttriggerTimer = tempTimeMod;

										time_t driveInTimer_secs = calEvent::getNextWakeTimer(calEvent::lastWakeEvent);
										time_t triggerTimer_secs = driveInTimer_secs - 60*tempTimeMod;
										string triggerTimeString = date_time_str_from_time_t("%R", &triggerTimer_secs);

										lgw.in("Triggers at: ", triggerTimeString," (", tempTimeMod, " mins before drive)", Tesla.datapack);
										lgw.i("Car seems to be located at ", Tesla.location);
										carAwokenOnce = true; // to avoid notifying user multiple times & waking car every loop

									}
									else {
										lgw.e("Could not wake car?? Car still reporting as offline after wakeLoop");
										// Could print a raw tesla-feed here for debugging if this triggers one day?
									}
								}
								else { lgw.i("car already awoken, waiting"); }
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
										string triggerAllowedRes = Tesla.triggerAllowed();
										if (triggerAllowedRes == "continue") {

											json hvac_result = Tesla.teslaPOST(settings::teslaVURL + "command/auto_conditioning_start");
											bool state_after_hvac = hvac_result["result"]; // should return true

											std::vector<string> seats_defrost = Tesla.coldCheckSet();
											string firstWord = (seats_defrost[1] == "1") ? "MAX DEFROST" : "HVAC";


											if (state_after_hvac)
											{
												// If we're here, it's success
												lgw.b();
												lgw.in(firstWord, " ON\nSeat Heater: ", seats_defrost[0], Tesla.datapack);


												// Update the triggered event to prevent it from re-running
												if (actionToDo == "home") { calEvent::lastTriggeredEvent->homeDone = true; }
												if (actionToDo == "work") { calEvent::lastTriggeredEvent->workDone = true; }

											}
											else
											{
												lgw.en("Could not turn HVAC on??");
											}
										}
										else if (triggerAllowedRes == "tempNotGood") {
											// If here, the only parameter mismatched is temperature
											// We'll not keep trying because at this point the temp won't incrase or decrease enough to matter
											lg.in("Trigger skipped, temperature within no-activate range (to save power).");
											actionDone = true;
											actionCancelDone = true;
										}
										else {
											lgw.in("Event & location valid but parameter mismatch, will keep trying\n", triggerAllowedRes);
										}
										actionDone = true;
									}
								}
								else // Here if the location doesnt match the actionToDo:
								{
									if (!actionCancelDone)
									{
										lgw.in("Event triggered for ", actionToDo, ", but car location is ", Tesla.location,
											", location doesn't match action, not activating HVAC.");
										actionCancelDone = true;
									}
								}
							}
							else if (actionToDo == "duplicate")
							{
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
								lgw.d("This event trigger for ", kindOfEvent, " has already ran.");
								lgw.d("The event trigger datetime is:"
									"\nYear=", datetime.tm_year,
									"\nMonth=", datetime.tm_mon,
									"\nDay=", datetime.tm_mday,
									"\nTime=", datetime.tm_hour, ":", datetime.tm_min, "\n");
							}
							else if (actionToDo == "checkShutoff")
							{
								// Only check once, if the car is no longer home at shutOffTimer,
								// we can assume you're gone thus you didn't call sick
								if (!shutoffHasBeenCheckedOnce)
								{
									Tesla.getData(true); // We must have accurate location for this
									if (Tesla.location == "home") {
										// If we're here, car is still home within the set buffer for shutoffTimer
										// Either you called sick or you're gonna be fucking late. Turn HVAC off
										Tesla.teslaPOST(settings::teslaVURL + "command/auto_conditioning_stop");
										lgw.in("HVAC SHUTOFF, car still home!", Tesla.datapack);
									}
									// Wether HVAC was shutoff or not, we don't need to check for shutoff anymore:
									shutoffHasBeenCheckedOnce = true;
								} else {
									// If here, a previous loop shutdown the HVAC for this event
									// This should only print if the HVAC was shutdown for this event.
									lgw.d("checkShutoff has already verified for this event.");
								}
							}

							//settings::intwakeTimer = 100000; // for testing
							// settings::inttriggerTimer = 69; // for testing
							lgw.d("Running eventTimeCheck with wakeTimer: ", settings::intwakeTimer,
								"mins, triggerTimer: ", settings::inttriggerTimer, "mins");

							// Get the mutex before writing to calendar vars
							if (!settings::settingsMutexLockSuccess("before eventTimeCheck")) {
								throw "Mutex timeout in main thread (before eventTimeCheck)";
							}
							lgw.d("!!!MAIN: MUTEX LOCKED (before eventTimeCheck)!!!");

							actionToDo = calEvent::eventTimeCheck(settings::intwakeTimer, settings::inttriggerTimer);

							settings::settingsMutex.unlock();
							lgw.d("Mutex UNLOCKED by main after eventTimeCheck");

							// actionToDo = "home"; // for testing
							if (!actionToDo.empty())
							{
								lgw.i("End of wakeLoop, actionToBeDone is: ", actionToDo);
								lgw.i("Waiting ", wakeLoopTimer, " seconds and re-running wakeLoop");
								sleepWithAPIcheck(wakeLoopTimer);
							}
						} while (!actionToDo.empty());
						break; // Must exit maxTries loop if no error caught
					}
					if (settings::doManualActivateHVAC()) //-> DO a manual HVAC activation now
					{
						Tesla.getData(true); // Make sure you have most up to date data with car awake
						// This is important for coldCheckSet to work properly for the seats

						// Activate the car's HVAC
						json hvac_result = Tesla.teslaPOST(settings::teslaVURL + "command/auto_conditioning_start");
						bool state_after_hvac = hvac_result["result"]; // should return true

						std::vector<string> seats_defrost = Tesla.coldCheckSet();
						string firstWord = (seats_defrost[1] == "1") ? "MAX DEFROST" : "HVAC";

						if (state_after_hvac)
						{
							// If we're here, it's success
							lgw.b();
							lgw.in(firstWord, " ON\nSeat Heater: ", seats_defrost[0], Tesla.datapack);


							// Update the triggered event to prevent it from re-running
							if (actionToDo == "home") { calEvent::lastTriggeredEvent->homeDone = true; }
							if (actionToDo == "work") { calEvent::lastTriggeredEvent->workDone = true; }

						}
						else
						{
							lgw.en("Could not turn HVAC on??");
						}

						// Get the mutex before zeroing the seatsActivateNow value
						if (!settings::settingsMutexLockSuccess("before resetting numberOfSeatsActivateNow")) {
							throw "Mutex timeout in main thread (before resetting numberOfSeatsActivateNow)";
						}
						lg.d("!!!MAIN: MUTEX LOCKED (before resetting numberOfSeatsActivateNow)!!!");

						settings::numberOfSeatsActivateNow = 0;
						lg.d("ActivateHVAC next run has been reset.");

						settings::settingsMutex.unlock();
						lg.d("Mutex UNLOCKED by main after resetting numberOfSeatsActivateNow");


					}
					break; // Must exit maxTries loop if no error caught
				}
				catch (string e) {
					lg.en("Critical failure: ", e, "\nFailure #", count, ", waiting 1 min and retrying.");
					lg.i("Is internet connected?", InternetConnected());
					sleep(60);
					if (++count == maxTries)
					{
						lg.en("ERROR ", count, " out of max ", maxTries, "!!! Stopping, reason ->\n", e);
						throw e;
					}
				}
			}



		}
		else {
			lg.i("\nProgram requires internet to run, will keep retrying.");
		}
		lg.b("\n<<<<<<<---------------------------PROGRAM TERMINATES HERE--------------------------->>>>>>>\n");
		calEvent::cleanup(); // to avoid calendar vectors overflowing

		mainLoopCounter++;
		lg.b("Waiting for " + settings::u_repeatDelay + " seconds... (now -> ", date_time_str_from_time_t(), " LOCAL)\n\n\n\n\n\n\n\n\n");
		sleepWithAPIcheck(settings::intrepeatDelay);
	}
	return 0;
}