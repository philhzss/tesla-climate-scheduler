#include "tesla_var.h"




using std::cout;
using std::endl;
using std::cin;
using std::string;



// Log file name for console messages
static Log lg("Carblock", Log::LogLevel::Debug);




std::map<string, string> car::getData(bool wakeCar, bool manualWakeWait)
{
	json teslaGetData;


	try
	{
		teslaGetData = car::teslaGET("vehicle_data");
		// Mutex must be locked AFTER teslaGET, or we could stay stuck in the teslaGET 30 sec wait loop
	}
	catch (string e)
	{
		lg.e("CURL Tesla API exception: ", e);
		throw string("Can't get car data from Tesla API. (CURL problem?)");
	}
	catch (nlohmann::detail::type_error e)
	{
		string errorString = "Problem getting data from Tesla API. Car updating? Tesla API down? nlohmann::detail::type_error";
		lg.e(errorString);
		throw string(errorString);
	}

	// Get the mutex before writing to any car variable
	if (!settings::settingsMutexLockSuccess("after succesful teslaGET in getData")) {
		throw string("settingsMutex timeout in main thread (after succesful teslaGET in getData)");
	}
	lg.d("!!!MAIN: settingsMutex LOCKED (before first teslaGET in getData)!!!");

	// Check if we got the full API response, if not then car is not awake / connected
	if (timeoutButSleeping(teslaGetData.dump())) {

		// If here, "vehicle unavailable: vehicle is offline or asleep"		
		carOnline = false;
	}
	else {
		// If here, teslaGetData does not contain "vehicle unavailable"
		carOnline = true;
	}

	carData_s["Car awake"] = std::to_string(carOnline);
	Tconnection_state = lg.prepareOnly(carOnline); // Tconnection_state is thread safe but carOnline is not

	settings::settingsMutex.unlock();
	lg.d("settingsMutex UNLOCKED before waking car");

	if (wakeCar)
	{
		bool initialWakeState = carOnline; // if it was sleeping, must delay Tinside_temp
		do {
			// Run wake function until carOnline is true
			car::wake();
		} while (!carOnline);

		if (!initialWakeState)
		{
			lg.d("Car was not awake, waiting 10 seconds after wake before getting data.");
			sleepWithAPIcheck(10, manualWakeWait);
		}

		// Now that the car is online, we can get more data
		json response = teslaGET("vehicle_data");

		// New endpoint required for location data
		json responseDrive = teslaGET("vehicle_data?endpoints=location_data");

		// Mutex must be locked AFTER teslaGET, or we could stay stuck in the teslaGET 30 sec wait loop
		// Get the mutex before getting more data
		if (!settings::settingsMutexLockSuccess("before teslaGET CAR AWOKEN in getData")) {
			throw string("settingsMutex timeout in main thread (before teslaGET CAR AWOKEN in getData)");
		}
		lg.d("!!!MAIN: settingsMutex LOCKED (before teslaGET CAR AWOKEN in getData)!!!");

		// lg.d(response.dump()); // Debug

		json vehicle_state = response["vehicle_state"];
		Tvehicle_name = vehicle_state["vehicle_name"];

		json climate_state = response["climate_state"];
		Tinside_temp = climate_state["inside_temp"];
		Toutside_temp = climate_state["outside_temp"];
		Tdriver_temp_setting = climate_state["driver_temp_setting"];
		Tis_climate_on = climate_state["is_climate_on"];

		json charge_state = response["charge_state"];
		Tusable_battery_level = charge_state["usable_battery_level"];
		Tbattery_level = charge_state["battery_level"];

		json drive_state = responseDrive["drive_state"];
		if (drive_state["shift_state"].is_null()) {
			Tshift_state = "P";
		}
		else {
			Tshift_state = drive_state["shift_state"];
			lg.i("Shift state is not null, is: " + Tshift_state);
		}
		Tlat = drive_state["latitude"];
		Tlong = drive_state["longitude"];
		location = checkCarLocation();

		// Save the time at which data was obtained for the API
		teslaDataUpdateTime = date_time_str_from_time_t() + " LOCAL";

		// remove trailing 0s?
		carData_s["vehicle_name"] = Tvehicle_name;
		carData_s["shift_state"] = Tshift_state;
		// not good
		carData_s["inside_temp"] = std::to_string(Tinside_temp);
		carData_s["outside_temp"] = std::to_string(Toutside_temp);
		carData_s["driver_temp_setting"] = std::to_string(Tdriver_temp_setting);
		carData_s["is_climate_on"] = std::to_string(Tis_climate_on);
		carData_s["usable_battery_level"] = std::to_string(Tusable_battery_level);
		carData_s["battery_level"] = std::to_string(Tbattery_level);
		carData_s["location_home_or_work"] = location;

		carData_s["Car awake"] = std::to_string(carOnline);
		Tconnection_state = lg.prepareOnly(carOnline); // Tconnection_state is thread safe but carOnline is not

		settings::settingsMutex.unlock();
		lg.d("settingsMutex UNLOCKED after waking car");

	}

	// Print map contents if LogLevelDebug
	if (lg.ReadLevel() >= Log::Debug)
	{
		lg.b();
		lg.d("carData_s map contents:");
		for (std::pair<string, string> element : carData_s)
		{
			lg.d(element.first + ": " + element.second);
		}
	}

	datapack = lg.prepareOnly("\nInside Temp: ", Tinside_temp,
		"C\nOutside Temp: ", Toutside_temp,
		"C\nBattery %: ", (int)Tbattery_level, " (", (int)Tusable_battery_level, ")");

	return carData_s;
}


json car::teslaPOST(string specifiedUrlPage, json bodyPackage)
{
	json jsonReadBuffer;
	bool response_code_ok;
	string tempUrlPage;
	int errorRetryCount = 0;
	do
	{
		if (errorRetryCount > 7) {
			throw string("TeslaPOST errored too many times, aborting");
		}

		// Reset URL value before modifying
		tempUrlPage = specifiedUrlPage;
		// We need "command/" for every single command except wake_up
		if (tempUrlPage != "wake_up") { tempUrlPage = "command/" + tempUrlPage; }

		string fullUrl = settings::teslemURL + tempUrlPage;
		const char* const url_to_use = fullUrl.c_str();
		// lg.d("teslaPOSTing to this URL: " + fullUrl); // disabled for clutter

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

			curl_easy_setopt(curl, CURLOPT_USERAGENT, tcs_userAgent.c_str());
			/* Now specify the POST data */
			struct curl_slist* headers = nullptr;
			// This should only be specified false on teslaAuth as we dont have token yet
			headers = curl_slist_append(headers, "Content-Type: application/json");
			lg.p("Sending auth header: " + settings::authHeader);
			const char* authHeader_c = settings::authHeader.c_str();
			headers = curl_slist_append(headers, authHeader_c);

			// Serialize the body/package json to string
			string data = bodyPackage.dump();
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
			curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.length());
			curl_easy_setopt(curl, CURLOPT_POST, 1);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);


			/* Perform the request, res will get the return code */
			res = curl_easy_perform(curl);
			/* Check for errors */
			if (res != CURLE_OK)
			{
				fprintf(stderr, "curl_easy_perform() failed: %s\n",
					curl_easy_strerror(res));
				lg.p("Before error throw, curl error code: ", res);
				lg.d(curl_easy_strerror(res));

				/* always cleanup */
				curl_easy_cleanup(curl);
				curl_global_cleanup();

				if (res == 28) {
					lg.d("The error is a curl 28 timeout error, continuing");
					response_code_ok = false;
					continue;
				}
				else {
					lg.d("The error is NOT a curl 28 timeout error, throwing string");
					throw string("curl_easy_perform() failed: " + std::to_string(res));
				}
			}
			if (res == CURLE_OK) {
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
				lg.i(response_code, "=response code for ", fullUrl);
				lg.p("readBuffer (before jsonify): " + readBuffer);


				if (response_code != 200)
				{
					// If not 200, maybe a problem
					if (timeoutButSleeping(readBuffer)) {
						// But if we're just timed out but sleeping, no problem
						response_code_ok = true;
						lg.d("408 timed out but vehicle sleeping");
					}
					else {
						lg.en("Abnormal server response (", response_code, ") for POST ", fullUrl);
						if (response_code == 408) {
							lg.i("TIMEOUT");
						}
						else if (response_code == 401) {
							lg.i("UNAUTHORIZED");
						}
						else if (response_code == 503) {
							lg.i("SERVICE UNAVAILABLE");
						}
						else {
							lg.in("IS TOKEN EXPIRED???");
						}
						lg.d("readBuffer for incorrect: " + readBuffer);
						response_code_ok = false;
						lg.i("Waiting 30 secs and retrying (teslaPOST)");
						sleepWithAPIcheck(30); // wait a little before redoing the curl request
						errorRetryCount++;
						continue;
					}
				}
				else {
					response_code_ok = true;
				}
			}

			/* always cleanup */
			curl_easy_cleanup(curl);
		}
		curl_global_cleanup();
		lg.d("TeslaPOST readBuffer passed to jsonObject (res should be 200 success)");
		jsonReadBuffer = json::parse(readBuffer);
		jsonReadBuffer = jsonReadBuffer["response"];
	} while (!response_code_ok);
	return jsonReadBuffer;
}


json car::teslaGET(string specifiedUrlPage)
{
	json responseObject;
	bool response_code_ok;
	int errorRetryCount = 0;
	do
	{
		if (errorRetryCount > 7) {
			throw string("TeslaGET errored too many times, aborting");
		}
		
		string fullUrl = settings::teslemURL + specifiedUrlPage;
		const char* const url_to_use = fullUrl.c_str();
		CURL* curl;
		CURLcode res;
		// Buffer to store result temporarily:
		string readBuffer;
		long response_code;

		curl_global_init(CURL_GLOBAL_ALL);
		curl = curl_easy_init();
		if (curl) {


			// Header for already authorized request
			struct curl_slist* headers = nullptr;
			lg.p("Sending auth header: " + settings::authHeader);
			const char* authHeader_c = settings::authHeader.c_str();
			headers = curl_slist_append(headers, authHeader_c);
			headers = curl_slist_append(headers, "Content-Type: application/json");
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);


			curl_easy_setopt(curl, CURLOPT_URL, url_to_use);
			curl_easy_setopt(curl, CURLOPT_USERAGENT, tcs_userAgent.c_str());

			/* use a GET to fetch this */
			curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

			/* Perform the request, res will get the return code */
			res = curl_easy_perform(curl);
			/* Check for errors */
			if (res != CURLE_OK)
			{
				fprintf(stderr, "curl_easy_perform() failed: %s\n",
					curl_easy_strerror(res));
				lg.p("Before error throw, curl error code: ", res);
				lg.d(curl_easy_strerror(res));

				/* always cleanup */
				curl_easy_cleanup(curl);
				curl_global_cleanup();

				if (res == 28) {
					lg.d("The error is a curl 28 timeout error, continuing");
					response_code_ok = false;
					continue;
				}
				else {
					lg.d("The error is NOT a curl 28 timeout error, throwing string");
					throw string("curl_easy_perform() failed: " + std::to_string(res));
				}
			}
			if (res == CURLE_OK) {
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
				lg.i(response_code, "=response code for GET ", fullUrl);
				lg.p("readBuffer (before jsonify): ", readBuffer);

				if (response_code != 200)
				{
					// If not 200, maybe a problem
					if (timeoutButSleeping(readBuffer)) {
						// But if we're just timed out but sleeping, no problem
						response_code_ok = true;
						lg.d("408 timed out but vehicle sleeping");
					}
					else {
						lg.e("Abnormal server response (", response_code, ") for GET ", fullUrl);
						if (response_code == 408) {
							lg.i("TIMEOUT");
						}
						else if (response_code == 401) {
							lg.i("UNAUTHORIZED");
						}
						else if (response_code == 503) {
							lg.i("SERVICE UNAVAILABLE");
						}
						else {
							lg.in("IS TOKEN EXPIRED???");
						}
						lg.d("readBuffer for incorrect: " + readBuffer);
						response_code_ok = false;
						lg.i("Waiting 10 secs and retrying (teslaGET)");
						sleepWithAPIcheck(10); // wait a little before redoing the curl request
						// Are we sleeping with API check while having mutex LOCKED? Not sure but it seems to work?
						errorRetryCount++;
						continue;
					}
				}
				else {
					response_code_ok = true;
				}
			}

			/* always cleanup */
			curl_easy_cleanup(curl);
		}
		curl_global_cleanup();


		json jsonReadBuffer = json::parse(readBuffer);
		if (jsonReadBuffer["response"].is_array()) {
			/* Inside "response" is an array, one item PER VEHICLE. Currently only supports 1 vehicle
			* But I will ahve to change this code to check for multiple vehicles eventually*/
			responseObject = (jsonReadBuffer["response"])[0];
		}
		else {
			responseObject = (jsonReadBuffer["response"]);
		}

	} while (!response_code_ok);
	return responseObject;
}


void car::wake()
{
	lg.b();
	json wake_result = teslaPOST("wake_up");
	string init_state_after_wake = wake_result["state"];
	lg.d("Wake command sent while state was: " + init_state_after_wake);
	carOnline = (init_state_after_wake == "online") ? true : false;
	if (carOnline) {
		return;
	}
	sleep(5); // No API checking during this sleep
	return;
}



// Parabola equation to figure out tempModifier from car's interior temp
int car::calcTempMod(int interior_temp)
{
	double rawTempTimeModifier;
	int finalTempTimeModifier;
	int defaultMinTime = settings::u_default20CMinTime;

	// Choose which curve to use
	if (interior_temp > 10)
	{
		lg.p("Interior temp is ", interior_temp, "C, using summer curve.");
		rawTempTimeModifier = pow(interior_temp, 2) / 40 - interior_temp + (10 + defaultMinTime);
		if (rawTempTimeModifier >= 16)
		{
			// Capped at 16 mins when super hot in car
			lg.p("Calculated temp time is ", rawTempTimeModifier, " mins, capping at 16 mins.");
			rawTempTimeModifier = 16;
		}
	}
	else
	{
		lg.p("Interior temp is ", interior_temp, "C, using winter curve.");
		rawTempTimeModifier = pow(interior_temp, 2) / 300 - interior_temp / static_cast<double>(2) + (9 + defaultMinTime);
		if (rawTempTimeModifier >= 32)
		{
			// Capped at 32 mins when super cold in car
			lg.p("Calculated temp time is ", rawTempTimeModifier, " mins, capping at 32 mins.");
			rawTempTimeModifier = 32;
		}
	}

	// Round up
	finalTempTimeModifier = static_cast<int>(rawTempTimeModifier + 0.5);
	if (finalTempTimeModifier <= 2)
	{
		lg.d("rawTempTimeModifier was calculated as ", rawTempTimeModifier, " mins, probably because default20CMinTime is set to ",
			settings::u_default20CMinTime, " mins.");
		finalTempTimeModifier = 2;
		lg.d("Calculated TempTimeModifier too low, bottoming out at 2 minutes");
	}
	lg.d("Interior car temp is ", interior_temp, "C, tempTimeModifier is: ", finalTempTimeModifier, +" minutes");

	return finalTempTimeModifier;
}


string car::checkCarLocation()
{
	float distanceRadius = 0.005; // Degrees
	float homeLat = std::stof(settings::u_homeCoords[0]);
	float homeLong = std::stof(settings::u_homeCoords[1]);
	float workLat = std::stof(settings::u_workCoords[0]);
	float workLong = std::stof(settings::u_workCoords[1]);
	string latRes;
	string longRes;
	string locationRes;

	// Compare lat
	if ((Tlat - homeLat + distanceRadius) * (Tlat - homeLat - distanceRadius) <= 0) // true if in range
	{
		latRes = "home";
	}
	else if ((Tlat - workLat + distanceRadius) * (Tlat - workLat - distanceRadius) <= 0)
	{
		latRes = "work";
	}
	else
	{
		latRes = "unknown";
	}

	// Compare long
	if ((Tlong - homeLong + distanceRadius) * (Tlong - homeLong - distanceRadius) <= 0) // true if in range
	{
		longRes = "home";
	}
	else if ((Tlong - workLong + distanceRadius) * (Tlong - workLong - distanceRadius) <= 0)
	{
		longRes = "work";
	}
	else
	{
		longRes = "unknown";
	}

	// Verify and return
	return (latRes == longRes) ? longRes : "unknown";
}

string car::triggerAllowed()
{
	bool batteryGood;
	bool tempGood;
	bool carOnlineGood;
	bool shiftStateGood;
	batteryGood = (Tusable_battery_level < 22) ? false : true;
	tempGood = ((Tinside_temp > settings::u_noActivateLowerLimitTemp) && (Tinside_temp < settings::u_noActivateUpperLimitTemp)) ? false : true;
	carOnlineGood = carOnline;
	shiftStateGood = (Tshift_state == "P") ? true : false;

	if (batteryGood && tempGood && carOnlineGood && shiftStateGood)
	{
		return "continue";
	}
	else if (batteryGood && carOnlineGood && shiftStateGood) {
		// If the only reason it failed is temp, do not keep trying
		return "tempNotGood";
	}
	else
	{
		string resultString = lg.prepareOnly("Temp verification passed: ", tempGood,
			"\nBattery verification passed: ", batteryGood,
			"\nShift State verification passed: ", shiftStateGood,
			"\nCarOnline verification passed: ", carOnlineGood);
		return resultString;
	}
}

std::vector<string> car::coldCheckSet()
{
	int requestedSeatHeat; // power, 0-1-2-3
	bool max_defrost_on = false; // if if triggers it gets set to true
	std::vector<string> resultVector;
	if (Tinside_temp <= settings::u_heatseat3temp)
	{
		requestedSeatHeat = 3;
	}
	else if (Tinside_temp <= settings::u_heatseat2temp)
	{
		requestedSeatHeat = 2;
	}
	else if (Tinside_temp < settings::u_heatseat1temp)
	{
		requestedSeatHeat = 1;
	}
	else
	{
		requestedSeatHeat = 0;
	}

	// Send the heat-seat request, turning the heated seat off if it's hot enough
	json seat_result = teslaPOST("remote_seat_heater_request", json{ {"seat_position", 0}, {"level", requestedSeatHeat } });

	// If seats should be 0, turn off ALL heated seats in car
	if (requestedSeatHeat == 0) {
		for (int seatNumber = 1; seatNumber <= 4; seatNumber++) {
			teslaPOST("remote_seat_heater_request", json{ {"seat_position", seatNumber}, {"level", requestedSeatHeat } });
		}
	}


	if (seat_result["result"]) {
		resultVector.push_back(std::to_string(requestedSeatHeat));
		lg.d("seat_result: ", seat_result["result"]);
	}
	else {
		resultVector.push_back("heated seats error");
	}

	if (Tinside_temp <= -10 || (settings::u_encourageDefrost && (Tinside_temp <= settings::u_noDefrostAbove)))
	{
		json jdefrost_result = teslaPOST("set_preconditioning_max", json{ {"on", true } });
		max_defrost_on = jdefrost_result["result"];
	}
	lg.d("Tinside_temp: ", Tinside_temp, ", encourageDefrost: ", settings::u_encourageDefrost);
	lg.d("max_defrost_on: ", max_defrost_on);

	resultVector.push_back(std::to_string(max_defrost_on));
	lg.d("resultVector, seat: ", resultVector[0], "// defrost_on: ", resultVector[1]);
	return resultVector;
}


bool car::timeoutButSleeping(string readBuffer) {
	size_t exists = readBuffer.find("vehicle unavailable");
	return exists != string::npos;
}
