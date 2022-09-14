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


void car::teslaAuth()
{
	/* This should be set to false if you want TCS to use TeslaPy Authorization
	* (built-in). If you want to provied your own access-token, set to true
	*/
	bool usingExternalAuthToken = false;

	if (usingExternalAuthToken) {
		return;
	}
	else
	{
		system("python3 auth.py"); // Use the refresh token to get an access token
		settings::readSettings("silent"); // Read the new access token from settings.json
		return;
	}
}


std::map<string, string> car::getData(bool wakeCar)
{
	json teslaGetData;
	// Get token from Tesla servers and store it in settings cpp
	// This must be run before everything else
	car::teslaAuth();

	// Get Tesla vehicleS state and vID
	try
	{
		teslaGetData = car::teslaGET("api/1/vehicles");
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
		throw "Mutex timeout in main thread (after succesful teslaGET in getData)";
	}
	lg.d("!!!MAIN: MUTEX LOCKED (before first teslaGET in getData)!!!");

	Tconnection_state = teslaGetData["state"];
	carOnline = (Tconnection_state == "online") ? true : false;
	// Store VID and VURL in settings
	settings::teslaVID = teslaGetData["id_s"];
	settings::teslaVURL = "api/1/vehicles/" + settings::teslaVID + "/";

	// State and vehicle ID always obtained, even if wakeCar false
	carData_s["vehicle_id"] = settings::teslaVID;
	carData_s["state"] = Tconnection_state;
	carData_s["Car awake"] = std::to_string(carOnline);

	settings::settingsMutex.unlock();
	lg.d("Mutex UNLOCKED before waking car");

	if (wakeCar)
	{
		bool initialWakeState = carOnline; // if it was sleeping, must delay Tinside_temp
		do {
			// Run wake function until carOnline is true
			car::wake();
		} while (!carOnline);

		if (!initialWakeState)
		{
			sleep(10);
			lg.d("Car was not awake, waiting 8 seconds after wake before getting data.");
		}

		// Get the mutex before getting more data
		if (!settings::settingsMutexLockSuccess("before teslaGET CAR AWOKEN in getData")) {
			throw "Mutex timeout in main thread (before teslaGET CAR AWOKEN in getData)";
		}
		lg.d("!!!MAIN: MUTEX LOCKED (before teslaGET CAR AWOKEN in getData)!!!");


		// Now that the car is online, we can get more data
		json response = teslaGET(settings::teslaVURL + "vehicle_data");
		Tdisplay_name = response["display_name"];

		json climate_state = response["climate_state"];
		Tinside_temp = climate_state["inside_temp"];
		Toutside_temp = climate_state["outside_temp"];
		Tdriver_temp_setting = climate_state["driver_temp_setting"];
		Tis_climate_on = climate_state["is_climate_on"];

		json charge_state = response["charge_state"];
		Tusable_battery_level = charge_state["usable_battery_level"];
		Tbattery_level = charge_state["battery_level"];

		json drive_state = response["drive_state"];
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
		teslaDataUpdateTime = return_current_time_and_date() + " LOCAL";

		// remove trailing 0s?
		carData_s["display_name"] = Tdisplay_name;
		carData_s["shift_state"] = Tshift_state;
		// not good
		carData_s["inside_temp"] = std::to_string(Tinside_temp);
		carData_s["outside_temp"] = std::to_string(Toutside_temp);
		carData_s["driver_temp_setting"] = std::to_string(Tdriver_temp_setting);
		carData_s["is_climate_on"] = std::to_string(Tis_climate_on);
		carData_s["usable_battery_level"] = std::to_string(Tusable_battery_level);
		carData_s["battery_level"] = std::to_string(Tbattery_level);
		carData_s["location_home_or_work"] = location;

		settings::settingsMutex.unlock();
		lg.d("Mutex UNLOCKED after waking car");

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
	json responseObject;
	bool response_code_ok;
	do
	{
		string fullUrl = settings::teslaOwnerURL + specifiedUrlPage;
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

			curl_easy_setopt(curl, CURLOPT_USERAGENT, "TCS");
			/* Now specify the POST data */
			struct curl_slist* headers = nullptr;
			// This should only be specified false on teslaAuth as we dont have token yet
			headers = curl_slist_append(headers, "Content-Type: application/json");
			const char* token_c = settings::teslaAuthString.c_str();
			lg.p("Sending auth header: " + settings::teslaAuthString);
			headers = curl_slist_append(headers, token_c);


			// Serialize the body/package json to string
			string data = bodyPackage.dump();
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
				lg.i(response_code, "=response code for ", fullUrl);
				lg.p("readBuffer (before jsonify): " + readBuffer);

				if (response_code != 200)
				{
					lg.en("Abnormal server response (", response_code, ") for ", fullUrl);
					lg.d("readBuffer for incorrect: " + readBuffer);
					response_code_ok = false;
					lg.i("Waiting 30 secs and retrying");
					sleep(30); // wait a little before redoing the curl request
					continue;
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
		json jsonReadBuffer = json::parse(readBuffer);
		// Inside "response" is an array, the first item is what contains the response:
		responseObject = jsonReadBuffer["response"];
	} while (!response_code_ok);
	return responseObject;
}

string car::teslaAuthPOST(string URLparams, json bodyPackage, string& headerResponse)
{
	// Buffer to store result temporarily:
	string readBuffer;
	json responseObject;
	bool response_code_ok;
	do
	{
		string fullUrl = settings::teslaAuthURL + URLparams;
		const char* const url_to_use = fullUrl.c_str();
		// lg.d("teslaPOSTing to this URL: " + fullUrl); // disabled for clutter

		CURL* curl;
		CURLcode res;
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
			// curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace); //

			curl_easy_setopt(curl, CURLOPT_USERAGENT, tcs_userAgent.c_str());
			/* Now specify the POST data */
			struct curl_slist* headers = nullptr;
			// This should only be specified false on teslaAuth as we dont have token yet
			string contentType = "Content-Type: application/x-www-form-urlencoded";
			headers = curl_slist_append(headers, contentType.c_str());


			// Serialize the body/package json to string
			string data = bodyPackage.dump();
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
			curl_easy_setopt(curl, CURLOPT_COOKIE, car::auth.step1_cookie.c_str());
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
			curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.length());
			lg.d("Postfields sent = ", data);
			lg.d("Headers sent = ", contentType);
			lg.d("Cookie sent = ", car::auth.step1_cookie);
			curl_easy_setopt(curl, CURLOPT_POST, 1);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0);

			/* we want the headers be written to this file handle */
			curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
			curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headerResponse);

			/* Perform the request, res will get the return code */
			res = curl_easy_perform(curl);
			/* Check for errors */
			if (res != CURLE_OK)
				fprintf(stderr, "curl_easy_perform() failed: %s\n",
					curl_easy_strerror(res));
			if (res == CURLE_OK) {
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
				lg.i(response_code, "=response code for ", fullUrl);
				lg.p("readBuffer (before jsonify): " + readBuffer);

				// Code should be 302 for step 2
				if (response_code != 302)
				{
					lg.en("Abnormal server response (", response_code, ") for ", fullUrl);
					lg.d("readBuffer for incorrect: " + readBuffer);
					response_code_ok = false;
					lg.i("Waiting 30 secs and retrying");
					sleep(30); // wait a little before redoing the curl request
					continue;
				}
				else {
					response_code_ok = true;
				}
			}

			/* always cleanup */
			curl_easy_cleanup(curl);
		}
		curl_global_cleanup();
	} while (!response_code_ok);
	return readBuffer;
}


json car::teslaGET(string specifiedUrlPage)
{
	json responseObject;
	bool response_code_ok;
	do
	{
		string fullUrl = settings::teslaOwnerURL + specifiedUrlPage;
		const char* const url_to_use = fullUrl.c_str();
		CURL* curl;
		CURLcode res;
		// Buffer to store result temporarily:
		string readBuffer;
		long response_code;

		curl_global_init(CURL_GLOBAL_DEFAULT);
		curl = curl_easy_init();
		if (curl) {


			// Header for already authorized request
			const char* authHeaderC = settings::teslaAuthString.c_str();
			struct curl_slist* headers = nullptr;
			headers = curl_slist_append(headers, authHeaderC);
			headers = curl_slist_append(headers, "Content-Type: application/json");
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);


			curl_easy_setopt(curl, CURLOPT_URL, url_to_use);
			curl_easy_setopt(curl, CURLOPT_USERAGENT, "Tesla Climate Scheduler/3.0.5"); // add variable v number

			/* use a GET to fetch this */
			curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

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
			if (res == CURLE_OK) {
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
				lg.i(response_code, "=response code for GET ", fullUrl);
				lg.p("readBuffer (before jsonify): ", readBuffer);

				if (response_code != 200)
				{
					lg.in("IS TOKEN EXPIRED???");
					lg.en("Abnormal server response (", response_code, ") for GET ", fullUrl);
					lg.d("readBuffer for incorrect: " + readBuffer);
					response_code_ok = false;
					lg.i("Waiting 30 secs and retrying");
					sleep(30); // wait a little before redoing the curl request
					settings::readSettings("silent"); // to allow updating the token without restarting app
					continue;
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

string car::teslaAuthGET(string URLparams, string& headerResponse)
{
	// Buffer to store result temporarily:
	string readBuffer;
	bool response_code_ok;
	do
	{
		string fullUrl = settings::teslaAuthURL + URLparams;
		const char* const url_to_use = fullUrl.c_str();
		CURL* curl;
		CURLcode res;

		long response_code;

		curl_global_init(CURL_GLOBAL_DEFAULT);
		curl = curl_easy_init();
		if (curl) {

			curl_easy_setopt(curl, CURLOPT_URL, url_to_use);
			curl_easy_setopt(curl, CURLOPT_USERAGENT, tcs_userAgent.c_str());

			/* use a GET to fetch this */
			curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

			/* we want the headers be written to this file handle */
			curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headerResponse);

			/* Perform the request, res will get the return code */
			res = curl_easy_perform(curl);
			/* Check for errors */
			if (res != CURLE_OK)
			{
				fprintf(stderr, "curl_easy_perform() teslaAuthGET failed: %s\n",
					curl_easy_strerror(res));
				throw "curl_easy_perform() failed: " + std::to_string(res);
			}
			if (res == CURLE_OK) {
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
				lg.i(response_code, "=response code for GET ", fullUrl);

				if (response_code != 200)
				{
					lg.en("Abnormal server response (", response_code, ") for GET ", fullUrl);
					lg.d("readBuffer for incorrect: " + readBuffer);
					response_code_ok = false;
					lg.i("Waiting 30 secs and retrying");
					sleep(30); // wait a little before redoing the curl request
					continue;
				}
				else {
					response_code_ok = true;
				}
			}

			/* always cleanup */
			curl_easy_cleanup(curl);
		}
		curl_global_cleanup();

	} while (!response_code_ok);
	return readBuffer;
}


void car::wake()
{
	lg.b();
	json wake_result = teslaPOST(settings::teslaVURL + "wake_up");
	string state_after_wake = wake_result["state"];
	lg.d("Wake command sent while state was: " + state_after_wake);
	carOnline = (state_after_wake == "online") ? true : false;
	if (carOnline) {
		// add a delay here of X seconds to make sure Tinside_temp is accurate? v3.0.4.1 debug
		return;
	}
	sleep(5);
	return;
}



static
void dump(const char* text,
	FILE* stream, unsigned char* ptr, size_t size)
{
	size_t i;
	size_t c;
	unsigned int width = 0x10;

	fprintf(stream, "%s, %10.10ld bytes (0x%8.8lx)\n",
		text, (long)size, (long)size);

	for (i = 0; i < size; i += width) {
		fprintf(stream, "%4.4lx: ", (long)i);

		/* show hex to the left */
		for (c = 0; c < width; c++) {
			if (i + c < size)
				fprintf(stream, "%02x ", ptr[i + c]);
			else
				fputs("   ", stream);
		}

		/* show data on the right */
		for (c = 0; (c < width) && (i + c < size); c++) {
			char x = (ptr[i + c] >= 0x20 && ptr[i + c] < 0x80) ? ptr[i + c] : '.';
			fputc(x, stream);
		}

		fputc('\n', stream); /* newline */
	}
}

static
int my_trace(CURL* handle, curl_infotype type,
	char* data, size_t size,
	void* userp)
{
	const char* text;
	(void)handle; /* prevent compiler warning */
	(void)userp;

	switch (type) {
	case CURLINFO_TEXT:
		fprintf(stderr, "== Info: %s", data);
	default: /* in case a new one is introduced to shock us */
		return 0;

	case CURLINFO_HEADER_OUT:
		text = "=> Send header";
		break;
	case CURLINFO_DATA_OUT:
		text = "=> Send data";
		break;
	case CURLINFO_SSL_DATA_OUT:
		text = "=> Send SSL data";
		break;
	case CURLINFO_HEADER_IN:
		text = "<= Recv header";
		break;
	case CURLINFO_DATA_IN:
		text = "<= Recv data";
		break;
	case CURLINFO_SSL_DATA_IN:
		text = "<= Recv SSL data";
		break;
	}

	dump(text, stderr, (unsigned char*)data, size);
	return 0;
}



// Parabola equation to figure out tempModifier from car's interior temp
int car::calcTempMod(int interior_temp)
{
	double rawTempTimeModifier;
	int finalTempTimeModifier;
	int defaultMinTime = settings::intdefault20CMinTime;

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
		lg.d("rawTempTimeModifier was calculated as ", rawTempTimeModifier, " mins, probably because default20CMinTime is set to "
			+ settings::u_default20CMinTime + " mins.");
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
	tempGood = ((Tinside_temp > 17) && (Tinside_temp < 23)) ? false : true;
	carOnlineGood = carOnline;
	shiftStateGood = (Tshift_state == "P") ? true : false;

	if (batteryGood && tempGood && carOnlineGood && shiftStateGood)
	{
		return "continue";
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
	if (Tinside_temp <= -10)
	{
		requestedSeatHeat = 3;
	}
	else if (Tinside_temp <= 5)
	{
		requestedSeatHeat = 2;
	}
	else if (Tinside_temp < 14)
	{
		requestedSeatHeat = 1;
	}
	else
	{
		requestedSeatHeat = 0;
	}

	// Send the heat-seat request, turning the heated seat off if it's hot enough
	json seat_result = teslaPOST(settings::teslaVURL + "command/remote_seat_heater_request", json{ {"heater", 0}, {"level", requestedSeatHeat } });
	if (seat_result["result"]) {
		resultVector.push_back(std::to_string(requestedSeatHeat));
		lg.d("seat_result: ", seat_result["result"]);
	}
	else {
		resultVector.push_back("heated seats error");
	}

	if (Tinside_temp <= -1)
	{
		json jdefrost_result = teslaPOST(settings::teslaVURL + "command/set_preconditioning_max", json{ {"on", true } });
		max_defrost_on = jdefrost_result["result"];
	}
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