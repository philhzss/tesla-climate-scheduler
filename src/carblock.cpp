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


std::map<string, string> car::teslaFiGetData(bool wakeCar, bool manualWakeWait) {

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
			while (tfiName == "") {
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

	datapack = lg.prepareOnly("\nInside Temp: ", Tinside_temp,
		"C\nOutside Temp: ", Toutside_temp,
		"C\nBattery %: ", (int)Tbattery_level, " (", (int)Tusable_battery_level, ")");

	return carData_s;

}


void car::tfiQueryCar() {
	try
	{
		// Get the mutex before writing to any car variable, for API
		if (!settings::settingsMutexLockSuccess("before teslaFiGetData")) {
			throw string("settingsMutex timeout in main thread (before teslaFiGetData)");
		}
		lg.d("!!!MAIN: settingsMutex LOCKED (before teslaFiGetData)!!!");


		string readBufferData = curl_GET(settings::tfiURL);

		// Parse raw data from tfi, after making sure you're authorized
		lg.b();
		lg.d("Raw data from TeslaFi: " + readBufferData);
		if (readBufferData.find("unauthorized") != string::npos)
		{
			lg.e("TeslaFi access denied.");
			throw string("TeslaFi token incorrect or expired.");
		}

		
		json jsonTfiData = json::parse(readBufferData);

		tfiConnectionState = jsonTfiData["state"];
		tfiDate = jsonTfiData["Date"];
		lg.d("tfiDate: " + tfiDate);

		if (jsonTfiData["display_name"].type() == json::value_t::string) {
			// Car is not sleeping, no need to wake
			lg.d("Car display_name is a string (good), car is awake. Getting more data.");
			tfiName = jsonTfiData["display_name"];
			lg.d("tfiName: " + tfiName);
			tficarState = jsonTfiData["carState"];
			lg.d("tficarState: " + tficarState);
			tfiConnectionState = jsonTfiData["state"];
			lg.d("tfiState (connection state): " + tfiConnectionState);

			try {
				string tfiShift = jsonTfiData["shift_state"];
			}
			catch (nlohmann::detail::type_error)
			{
				lg.d("json type_error for shift_state in TeslaFi, car assumed to be in park, manually writing");
				tfiShift = "P";
			}
			lg.d("tfiShift: " + tfiShift);

			tfiLocation = jsonTfiData["location"];
			lg.d("tfiLocation: " + tfiLocation);
			tfiIntTemp = jsonTfiData["inside_temp"];
			lg.d("tfiIntTemp: " + tfiIntTemp);
			tfiOutTemp = jsonTfiData["outside_temp"];
			lg.d("tfiOutTemp: " + tfiOutTemp);
			tfiTempSetting = jsonTfiData["driver_temp_setting"];
			lg.d("tfiTempSetting: " + tfiTempSetting);
			tfiIsHvacOn = jsonTfiData["is_climate_on"];
			lg.d("tfiIsHvacOn: " + tfiIsHvacOn);
			tfiUsableBat = jsonTfiData["usable_battery_level"];
			lg.d("tfiUsableBat: " + tfiUsableBat);
			tfiBat = jsonTfiData["battery_level"];
			lg.d("tfiBat :" + tfiBat);

			carOnline = true;
		}
		else if ((tfiConnectionState == "online") and (jsonTfiData["display_name"].type() != json::value_t::string))
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
		lg.d("settingsMutex UNLOCKED after teslaFiGetData");
	}
	catch (string e)
	{
		lg.e("CURL TeslaFi exception: " + e);
		settings::settingsMutex.unlock();
		lg.d("settingsMutex UNLOCKED after teslaFiGetData EXCEPTION");
		throw string("Can't get car data from Tesla Fi. (CURL problem?)");
	}
	catch (nlohmann::detail::type_error e)
	{
		lg.e("Problem getting data from Tesla Fi. Car updating? API down? nlohmann::detail::type_error");
		settings::settingsMutex.unlock();
		lg.d("settingsMutex UNLOCKED after teslaFiGetData EXCEPTION");
		throw string("Can't get car data from Tesla Fi. nlohmann::detail::type_error");
	}

	carData_s["Success"] = "True";
	carData_s["Car awake"] = std::to_string(carOnline);
	carData_s["Tesla Fi Date"] = tfiDate;
	carData_s["Tesla Fi Name"] = tfiName;
	carData_s["Tesla Fi Car State"] = tficarState;
	carData_s["Tesla Fi Connection State"] = tfiConnectionState;
	carData_s["Tesla Fi Shift State"] = tfiShift;
	carData_s["Tesla Fi Location"] = tfiLocation;
	carData_s["Tesla Fi Inside temp"] = tfiIntTemp;
	carData_s["Tesla Fi Outside temp"] = tfiOutTemp;
	carData_s["Tesla Fi Driver Temp Setting"] = tfiTempSetting;
	carData_s["Tesla Fi is HVAC On"] = tfiIsHvacOn;
	carData_s["Tesla Fi Usable Battery"] = tfiUsableBat;
	carData_s["Tesla Fi Battery level"] = tfiBat;


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
	json wake_result = settings::tfiURL + "&command=wake_up";
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
	json seat_result = teslaPOST(settings::teslaVURL + "command/remote_seat_heater_request", json{ {"heater", 0}, {"level", requestedSeatHeat } });

	// If seats should be 0, turn off ALL heated seats in car
	if (requestedSeatHeat == 0) {
		for (int seatNumber = 1; seatNumber <= 4; seatNumber++) {
			teslaPOST(settings::teslaVURL + "command/remote_seat_heater_request", json{ {"heater", seatNumber}, {"level", requestedSeatHeat } });
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
		json jdefrost_result = teslaPOST(settings::teslaVURL + "command/set_preconditioning_max", json{ {"on", true } });
		max_defrost_on = jdefrost_result["result"];
	}
	lg.d("Tinside_temp: ", Tinside_temp, ", encourageDefrost: ", settings::u_encourageDefrost);
	lg.d("max_defrost_on: ", max_defrost_on);

	resultVector.push_back(std::to_string(max_defrost_on));
	lg.d("resultVector, seat: ", resultVector[0], "// defrost_on: ", resultVector[1]);
	return resultVector;
}

// Could be global util?
string authorizer::random_ANstring() {
	srand((unsigned int)time(NULL));
	auto randchar = []() -> char
	{
		const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
		const size_t max_index = (sizeof(charset) - 1);
		return charset[rand() % max_index];
	};
	string str(86, 0);
	std::generate_n(str.begin(), 86, randchar);
	return str;
}

// Could be global util?
size_t authorizer::stringCount(const std::string& referenceString,
	const std::string& subString) {

	const size_t step = subString.size();

	size_t count(0);
	size_t pos(0);

	while ((pos = referenceString.find(subString, pos)) != std::string::npos) {
		pos += step;
		++count;
	}

	return count;
}