#include <unistd.h>
#include "tesla_var.h"
#include "logger.h"
#include "carblock.h"
#include "picosha2.h"
#include "base64.h"



using std::cout;
using std::endl;
using std::cin;
using std::string;




// Log file name for console messages
static Log lg("Carblock", Log::LogLevel::Debug);


std::map<string, string> car::tfiGetData(bool wakeCar, bool manualWakeWait) {

	tfiQueryCar();

	// Check wake status and wake if required
	if (wakeCar)
	{
		// Start by resuming Tfi polling if required
		// IS THIS REALLY REQUIRED?? KEEPING IN CASE::
		/*if (carData_s["Tesla Fi Name"] == "") {
			lg.d("Restarting Tesla Fi polling");
			string readBufferData = curl_GET(settings::tfiURL + "&command=wake");
			sleepWithAPIcheck(10);
			while (tfiVehicle_name == "") {
				// Then recheck Tfi data
				car::tfiQueryCar();
				lg.i("WAITING");
				sleep(15);
			}
		} */


		bool initialWakeState = carOnline;
		do {
			// Run wake function until carOnline is true
			car::wake();
		} while (!carOnline);

		// if it was sleeping, must delay Tinside_temp
		if (!initialWakeState)
		{
			lg.d("Car was not awake, waiting 10 seconds after wake before getting data.");
			sleepWithAPIcheck(10, manualWakeWait);
		}

		// Now that the car is online, we can get more data
		tfiQueryCar();

		location = checkCarLocation();

		// This is just home-work, cannot directly copy Tfi location in here:
		carData_s["location_home_or_work"] = location;
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

	datapack = lg.prepareOnly("\nInside Temp: ", tfiInside_temp,
		"C\nOutside Temp: ", tfiOutside_temp,
		"C\nBattery %: ", tfiBattery_level, " (", tfiUsable_battery_level, ")");

	return carData_s;

}


json car::tfiInternetOperation(string url) {
	string readBufferData = curl_GET(settings::tfiURL + url);

	// Parse raw data from tfi, after making sure you're authorized
	lg.b();
	lg.d("Raw data from TeslaFi: " + readBufferData);
	if (readBufferData.find("unauthorized") != string::npos)
	{
		lg.e("TeslaFi access denied.");
		throw string("TeslaFi token incorrect or expired.");
	}

	return json::parse(readBufferData);
}


void car::tfiQueryCar() {
	try
	{
		// Get the mutex before writing to any car variable, for API
		if (!settings::settingsMutexLockSuccess("before tfiGetData")) {
			throw string("settingsMutex timeout in main thread (before tfiGetData)");
		}
		lg.d("!!!MAIN: settingsMutex LOCKED (before tfiGetData)!!!");

		json jsonTfiData = tfiInternetOperation();

		tfiConnection_state = jsonTfiData["state"];
		tfiDate = jsonTfiData["Date"];
		lg.d("tfiDate: " + tfiDate);

		if (jsonTfiData["display_name"].type() == json::value_t::string) {
			// Car is not sleeping, no need to wake
			lg.d("Car display_name is a string (good), car is awake. Getting more data.");
			tfiVehicle_name = jsonTfiData["display_name"];
			lg.d("tfiVehicle_name: " + tfiVehicle_name);
			tfiCar_state_activity = jsonTfiData["carState"];
			lg.d("tfiCar_state_activity: " + tfiCar_state_activity);
			tfiConnection_state = jsonTfiData["state"];
			lg.d("tfiState (connection state): " + tfiConnection_state);

			try {
				string tfiShift_state = jsonTfiData["shift_state"];
			}
			catch (nlohmann::detail::type_error)
			{
				lg.d("json type_error for shift_state in TeslaFi, car assumed to be in park, manually writing");
				tfiShift_state = "P";
			}
			lg.d("tfiShift_state: " + tfiShift_state);

			tfiLocation = jsonTfiData["location"];
			lg.d("tfiLocation: " + tfiLocation);

			tfiInside_temp = std::stof(string(jsonTfiData["inside_temp"]));
			lg.d("tfiInside_temp: ", tfiInside_temp);
			tfiOutside_temp = std::stof(string(jsonTfiData["outside_temp"]));
			lg.d("tfiOutside_temp: ", tfiOutside_temp);
			tfiDriver_temp_setting = std::stof(string(jsonTfiData["driver_temp_setting"]));
			lg.d("tfiDriver_temp_setting: ", tfiDriver_temp_setting);

			// Cast string to int before converting to boolean
			tfiIs_climate_on = std::stoi(string(jsonTfiData["is_climate_on"]));
			lg.d("tfiIs_climate_on: " + tfiIs_climate_on);

			tfiUsable_battery_level = std::stof(string(jsonTfiData["usable_battery_level"]));
			lg.d("tfiUsable_battery_level: ", tfiUsable_battery_level);
			tfiBattery_level = std::stof(string(jsonTfiData["battery_level"]));
			lg.d("tfiBattery_level :", tfiBattery_level);

			carOnline = true;
		}
		else if ((tfiConnection_state == "online") and (jsonTfiData["display_name"].type() != json::value_t::string))
		{
			// If the connection state is reported as online, but no other data can be pulled, car is in Tesla Fi sleep mode attempt. 
			// Detect this and assume car is asleep if so:
			lg.i("The car is trying to sleep, sleep mode assumed, carOnline=False");
			carOnline = false;
		}
		else {
			// Car is sleeping or trying to sleep, will have to wake
			lg.d("Car display_name is null, not a string, car is most likely sleeping OR trying to sleep, carOnline=False");
			carOnline = false;
		}
		settings::settingsMutex.unlock();
		lg.d("settingsMutex UNLOCKED after tfiGetData");
	}
	catch (string e)
	{
		lg.e("CURL TeslaFi exception: " + e);
		settings::settingsMutex.unlock();
		lg.d("settingsMutex UNLOCKED after tfiGetData EXCEPTION");
		throw string("Can't get car data from Tesla Fi. (CURL problem?)");
	}
	catch (nlohmann::detail::type_error e)
	{
		lg.e("Problem getting data from Tesla Fi. Car updating? API down? nlohmann::detail::type_error");
		settings::settingsMutex.unlock();
		lg.d("settingsMutex UNLOCKED after tfiGetData EXCEPTION");
		throw string("Can't get car data from Tesla Fi. nlohmann::detail::type_error");
	}

	carData_s["Success"] = "True";
	carData_s["Car awake"] = std::to_string(carOnline);
	carData_s["Tesla Fi Date"] = tfiDate;
	carData_s["Tesla Fi Name"] = tfiVehicle_name;
	carData_s["Tesla Fi Car State"] = tfiCar_state_activity;
	carData_s["Tesla Fi Connection State"] = tfiConnection_state;
	carData_s["Tesla Fi Shift State"] = tfiShift_state;
	carData_s["Tesla Fi Location"] = tfiLocation;
	carData_s["Tesla Fi Inside temp"] = std::to_string(tfiInside_temp);
	carData_s["Tesla Fi Outside temp"] = std::to_string(tfiOutside_temp);
	carData_s["Tesla Fi Driver Temp Setting"] = tfiDriver_temp_setting;
	carData_s["Tesla Fi is HVAC On"] = std::to_string(tfiIs_climate_on);
	carData_s["Tesla Fi Usable Battery"] = std::to_string(tfiUsable_battery_level);
	carData_s["Tesla Fi Battery level"] = std::to_string(tfiBattery_level);


	return;
}



// Scheduled for deletion:

//json car::teslaPOST(string specifiedUrlPage, json bodyPackage)
//{
//	json responseObject;
//	bool response_code_ok;
//	do
//	{
//		string fullUrl = settings::teslaOwnerURL + specifiedUrlPage;
//		const char* const url_to_use = fullUrl.c_str();
//		// lg.d("teslaPOSTing to this URL: " + fullUrl); // disabled for clutter
//
//		CURL* curl;
//		CURLcode res;
//		// Buffer to store result temporarily:
//		string readBuffer;
//		long response_code;
//
//		/* In windows, this will init the winsock stuff */
//		curl_global_init(CURL_GLOBAL_ALL);
//
//		/* get a curl handle */
//		curl = curl_easy_init();
//
//		if (curl) {
//			/* First set the URL that is about to receive our POST. This URL can
//			   just as well be a https:// URL if that is what should receive the
//			   data. */
//			curl_easy_setopt(curl, CURLOPT_URL, url_to_use);
//
//			curl_easy_setopt(curl, CURLOPT_USERAGENT, "TCS");
//			/* Now specify the POST data */
//			struct curl_slist* headers = nullptr;
//			// This should only be specified false on teslaAuth as we dont have token yet
//			headers = curl_slist_append(headers, "Content-Type: application/json");
//			const char* token_c = settings::teslaAuthString.c_str();
//			lg.p("Sending auth header: " + settings::teslaAuthString);
//			headers = curl_slist_append(headers, token_c);
//
//
//			// Serialize the body/package json to string
//			string data = bodyPackage.dump();
//			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
//			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
//			curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.length());
//			curl_easy_setopt(curl, CURLOPT_POST, 1);
//			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
//			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
//			curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
//
//
//			/* Perform the request, res will get the return code */
//			res = curl_easy_perform(curl);
//			/* Check for errors */
//			if (res != CURLE_OK)
//			{
//				fprintf(stderr, "curl_easy_perform() failed: %s\n",
//					curl_easy_strerror(res));
//				lg.p("Before error throw, curl error code: ", res);
//				lg.d(curl_easy_strerror(res));
//
//				/* always cleanup */
//				curl_easy_cleanup(curl);
//				curl_global_cleanup();
//
//				if (res == 28) {
//					lg.d("The error is a curl 28 timeout error, continuing");
//					response_code_ok = false;
//					continue;
//				}
//				else {
//					lg.d("The error is NOT a curl 28 timeout error, throwing string");
//					throw string("curl_easy_perform() failed: " + std::to_string(res));
//				}
//			}
//
//			if (res == CURLE_OK) {
//				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
//				lg.i(response_code, "=response code for ", fullUrl);
//				lg.p("readBuffer (before jsonify): " + readBuffer);
//
//				if (response_code != 200)
//				{
//					lg.en("Abnormal server response (", response_code, ") for ", fullUrl);
//					lg.d("readBuffer for incorrect: " + readBuffer);
//					response_code_ok = false;
//					lg.i("Waiting 30 secs and retrying (teslaPOST)");
//					sleepWithAPIcheck(30); // wait a little before redoing the curl request
//					teslaAuth(); // to allow updating the token without restarting app, or to rerun auth.py
//					continue;
//				}
//				else {
//					response_code_ok = true;
//				}
//			}
//
//			/* always cleanup */
//			curl_easy_cleanup(curl);
//		}
//		curl_global_cleanup();
//		lg.d("TeslaPOST readBuffer passed to jsonObject (res should be 200 success)");
//		json jsonReadBuffer = json::parse(readBuffer);
//		// Inside "response" is an array, the first item is what contains the response:
//		responseObject = jsonReadBuffer["response"];
//	} while (!response_code_ok);
//	return responseObject;
//}




void car::wake()
{
	lg.b();
	json wake_result = tfiInternetOperation("&command=wake_up");
	string init_state_after_wake = wake_result["response"]["state"];
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
		rawTempTimeModifier = pow(interior_temp, 2) / 300 - interior_temp / 2 + (9 + defaultMinTime);
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
	string locationRes;

	if (tfiLocation == "Home") { locationRes = "home"; }
	else if (tfiLocation == "Work / Montreal ACC") { locationRes = "work"; }
	else { locationRes = "unknown"; }

	return locationRes;
}

string car::triggerAllowed()
{
	bool batteryGood;
	bool tempGood;
	bool carOnlineGood;
	bool shiftStateGood;
	batteryGood = (tfiUsable_battery_level < 22) ? false : true;
	tempGood = ((tfiInside_temp > settings::u_noActivateLowerLimitTemp) && (tfiInside_temp < settings::u_noActivateUpperLimitTemp)) ? false : true;
	carOnlineGood = carOnline;
	shiftStateGood = (tfiShift_state == "P") ? true : false;

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
	if (tfiInside_temp <= settings::u_heatseat3temp)
	{
		requestedSeatHeat = 3;
	}
	else if (tfiInside_temp <= settings::u_heatseat2temp)
	{
		requestedSeatHeat = 2;
	}
	else if (tfiInside_temp < settings::u_heatseat1temp)
	{
		requestedSeatHeat = 1;
	}
	else
	{
		requestedSeatHeat = 0;
	}


	// Send the heat-seat request, turning the heated seat off if it's hot enough
	json seat_result = tfiInternetOperation("&command=seat_heater&heater=0&level=" + std::to_string(requestedSeatHeat));

	// If seats should be 0, turn off ALL heated seats in car
	// Currently disabled because it seems Tesla Fi can't heat anything other than the drivers seat?
	/*if (requestedSeatHeat == 0) {
		for (int seatNumber = 1; seatNumber <= 4; seatNumber++) {
			teslaPOST(settings::teslaVURL + "command/remote_seat_heater_request", json{ {"heater", seatNumber}, {"level", requestedSeatHeat } });
		}
	} */


	if (seat_result["response"]["result"]) {
		resultVector.push_back(std::to_string(requestedSeatHeat));
		lg.d("seat_result: ", seat_result["response"]["result"]);
	}
	else {
		resultVector.push_back("heated seats error");
	}

	if (tfiInside_temp <= -10 || (settings::u_encourageDefrost && (tfiInside_temp <= settings::u_noDefrostAbove)))
	{
		json jdefrost_result = tfiInternetOperation("command=set_preconditioning_max&statement=true");
		max_defrost_on = jdefrost_result["response"]["result"];
	}
	lg.d("Tinside_temp: ", tfiInside_temp, ", encourageDefrost: ", settings::u_encourageDefrost);
	lg.d("max_defrost_on: ", max_defrost_on);

	resultVector.push_back(std::to_string(max_defrost_on));
	lg.d("resultVector, seat: ", resultVector[0], "// defrost_on: ", resultVector[1]);
	return resultVector;
}
