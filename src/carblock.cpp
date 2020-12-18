#include <unistd.h>
#include "tesla_var.h"
#include "logger.h"
#include "carblock.h"



using std::cout;
using std::endl;
using std::cin;
using std::string;




// Log file name for console messages
static Log lg("Carblock", Log::LogLevel::Debug);


void car::teslaAuth()
{
	if (lg.ReadLevel() >= Log::Debug) {
		lg.b();
		lg.b();
		lg.d("teslaAuth function start");
	}
	json authjson;
	authjson = teslaPOST("oauth/token", settings::authReqPackage, true);
	//cout << authjson << endl;
	string teslaToken = authjson["access_token"];
	settings::teslaAuthString = "Authorization: Bearer " + teslaToken;
	lg.d("Tesla Token header is set to: " + settings::teslaAuthString);
	if (lg.ReadLevel() >= Log::Debug) {
		lg.d("teslaAuth function end");
		lg.b();
		lg.b();
	}
	return;
}


std::map<string, string> car::getData(bool wakeCar)
{
	// Get token from Tesla servers and store it in settings cpp
	// This must be run before everything else
	car::teslaAuth();

	// Get Tesla vehicleS state and vID
	try
	{
		json teslaGetData = car::teslaGET("api/1/vehicles");
		Tconnection_state = teslaGetData["state"];
		carOnline = (Tconnection_state == "online") ? true : false;
		// Store VID and VURL in settings
		settings::teslaVID = teslaGetData["id_s"];
		settings::teslaVURL = "api/1/vehicles/" + settings::teslaVID + "/";
	}
	catch (string e)
	{
		lg.e("CURL Tesla API exception: ", e);
		throw string("Can't get car data from Tesla API. (CURL problem?)");
	}
	catch (nlohmann::detail::type_error e)
	{
		lg.e("Problem getting data from Tesla API. Car updating? Tesla API down? nlohmann::detail::type_error");
	}

	// State and vehicle ID always obtained, even if wakeCar false
	carData_s["vehicle_id"] = settings::teslaVID;
	carData_s["state"] = Tconnection_state;
	carData_s["Car awake"] = std::to_string(carOnline);

	if (wakeCar)
	{
		do {
			// Run wake function until carOnline is true
			car::wake();
		} while (!carOnline);

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

	return carData_s;
}


json car::teslaPOST(string url, json package, bool noBearerToken)
{
	string fullUrl = settings::teslaURL + url;
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
		/* Now specify the POST data */
		struct curl_slist* headers = nullptr;
		headers = curl_slist_append(headers, "Content-Type: application/json");
		// This should only be specified false on teslaAuth as we dont have token yet
		if (!noBearerToken) {
			const char* token_c = settings::teslaAuthString.c_str();
			lg.p("Sending auth header: " + settings::teslaAuthString);
			headers = curl_slist_append(headers, token_c);
		}


		// Serialize the package json to string
		string data = package.dump();
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
			lg.i(std::to_string(response_code) + "=response code for " + fullUrl);
			lg.p("readBuffer (before jsonify): " + readBuffer);
		}

		/* always cleanup */
		curl_easy_cleanup(curl);
	}
	curl_global_cleanup();

	json jsonReadBuffer = json::parse(readBuffer);
	json responseObject;
	// If noBearerToken then it doesnt return "response", returns "access token" so cant do this
	if (noBearerToken) {
		responseObject = jsonReadBuffer;
	}
	else {
		// Inside "response" is an array, the first item is what contains the response:
		responseObject = jsonReadBuffer["response"];
	}
	return responseObject;
}

json car::teslaGET(string url)
{
	string fullUrl = settings::teslaURL + url;
	const char* const url_to_use = fullUrl.c_str();
	CURL* curl;
	CURLcode res;
	// Buffer to store result temporarily:
	string readBuffer;
	long response_code;

	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();
	if (curl) {


		// Header for auth
		const char* authHeaderC = settings::teslaAuthString.c_str();
		struct curl_slist* headers = nullptr;
		headers = curl_slist_append(headers, authHeaderC);
		headers = curl_slist_append(headers, "Content-Type: application/json");

		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_URL, url_to_use);

		/* use a GET to fetch this */
		//curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

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
			lg.i(std::to_string(response_code) + "=response code for " + fullUrl);
			lg.p("readBuffer (before jsonify): " + readBuffer);
		}

		/* always cleanup */
		curl_easy_cleanup(curl);
	}
	curl_global_cleanup();

	json jsonReadBuffer = json::parse(readBuffer);
	json responseObject;
	if (jsonReadBuffer["response"].is_array()) {
		/* Inside "response" is an array, one item PER VEHICLE. Currently only supports 1 vehicle
		* But I will ahve to change this code to check for multiple vehicles eventually*/
		responseObject = (jsonReadBuffer["response"])[0];
	}
	else {
		responseObject = (jsonReadBuffer["response"]);
	}
	return responseObject;
}


void car::wake()
{
	lg.b();
	json wake_result = teslaPOST(settings::teslaVURL + "wake_up");
	string state_after_wake = wake_result["state"];
	lg.d("Wake command sent while state was: " + state_after_wake);
	carOnline = (state_after_wake == "online") ? true : false;
	if (carOnline) {
		return;
	}
	sleep(5);
	return;
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