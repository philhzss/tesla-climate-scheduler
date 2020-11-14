#include "tesla_var.h"
#include "logger.h"
#include "carblock.h"



using std::cout;
using std::endl;
using std::cin;
using std::string;




// Log file name for console messages
static Log lg("Carblock", Log::LogLevel::Programming);


void car::teslaAuth()
{
	lg.d("teslaAuth function start");
	json authjson;
	authjson = teslaPOST("oauth/token", settings::authReqPackage, true);
	//cout << authjson << endl;
	string teslaToken = authjson["access_token"];
	settings::teslaAuthString = "Authorization: Bearer " + teslaToken;
	lg.d("Tesla Token header is set to: " + settings::teslaAuthString);
	lg.d("teslaAuth function end");
	return;
}


std::map<string, string> car::getData(bool wakeCar, string log)
{
	// Get token from Tesla servers and store it in settings cpp
	// This must be run at least once before getData
	car::teslaAuth();

	// Get Tesla Fi data
	try
	{
		string readBufferData = curl_GET(settings::tfiURL);

		// Parse and log the data from tfi, after making sure you're authorized
		lg.b();
		lg.d("Raw data from TeslaFi: " + readBufferData);
		if (readBufferData.find("unauthorized") != string::npos)
		{
			lg.e("TeslaFi access denied.");
			throw string("TeslaFi token incorrect or expired.");
		}
		json jsonTfiData = json::parse(readBufferData);

		// Display all the data and store in variables
		tfiDate = jsonTfiData["Date"];
		lg.d("tfiDate: " + tfiDate);
		if (jsonTfiData["display_name"].type() == json::value_t::string) {
			// Car is not sleeping, no need to wake
			lg.d("Car display_name is a string (good), car is awake. Getting more data.");
			tfiName = jsonTfiData["display_name"];
			lg.d("tfiName: " + tfiName);
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
			carAwake = true;
		}
		else {
			// Car is sleeping or trying to sleep, will have to wake
			lg.d("Car display_name is null, not a string, car is most likely sleeping OR trying to sleep.");
			carAwake = false;
		}
		tficarState = jsonTfiData["carState"];
		lg.d("tficarState: " + tficarState);
		tfiState = jsonTfiData["state"];
		lg.d("tfiState (connection state): " + tfiState);
		// If the connection state is reported as online, but no other data can be pulled, car is in Tesla Fi sleep mode attempt. Detect this and assume car is asleep if so:
		if ((tfiState == "online") and (jsonTfiData["display_name"].type() != json::value_t::string))
		{
			lg.i("The car is trying to sleep, sleep mode assumed, will attempt to wake.");
			carAwake = false;
		}
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
	}
	catch (string e)
	{
		lg.e("CURL TeslaFi exception: " + e);
		throw string("Can't get car data from Tesla Fi. (CURL problem?)");
	}
	catch (nlohmann::detail::type_error e)
	{
		lg.e("Problem getting data from Tesla Fi. Car updating? Tesla API down? nlohmann::detail::type_error");
	}

	// Get Tesla official server vehicleS data
	try
	{
		json teslaGetData = car::teslaGET("api/1/vehicles");
		tstate = teslaGetData["state"];
		// Store VID and VURL in settings
		settings::teslaVID = teslaGetData["id_s"];
		settings::teslaVURL = "api/1/vehicles/" + settings::teslaVID + "/";
	}
	catch (string e)
	{
		lg.e("CURL Tesla API exception: " + e);
		throw string("Can't get car data from Tesla Official API. (CURL problem?)");
	}
	catch (nlohmann::detail::type_error e)
	{
		lg.e("Problem getting data from Tesla API. Car updating? Tesla API down? nlohmann::detail::type_error");
	}

	// Store the data in the map, then check wake status and wake if required
	carData["Success"] = "True";
	carData["Car awake"] = std::to_string(carAwake);
	carData["Tesla Fi Date"] = tfiDate;
	carData["Tesla Fi Name"] = tfiName;
	carData["Tesla Fi Car State"] = tficarState;
	carData["Tesla Fi Connection State"] = tfiState;
	carData["Tesla API Online State"] = tstate;
	carData["Tesla Fi Shift State"] = tfiShift;
	carData["Tesla Fi Location"] = tfiLocation;
	carData["Tesla Fi Inside temp"] = tfiIntTemp;
	carData["Tesla Fi Outside temp"] = tfiOutTemp;
	carData["Tesla Fi Driver Temp Setting"] = tfiTempSetting;
	carData["Tesla Fi is HVAC On"] = tfiIsHvacOn;
	carData["Tesla Fi Usable Battery"] = tfiUsableBat;
	carData["Tesla Fi Battery level"] = tfiBat;
	carData["Tesla API vehicle ID"] = settings::teslaVID;
	lg.b();

	if (!log.empty())
	{
		lg.i("getData logging enabled for this call");
		lg.i("getData result:");
		for (std::pair<string, string> element : carData)
		{
			lg.i(element.first + ": " + element.second);
		}
	}


	if (wakeCar)
	{
		do { car::wake(); } while (!carAwake);
	}

	return carData;
	// Implement somthing that repeats the getData function if the fields arent filled enough??
	// Or just call the wake function?? (when car updating, missing fields).
}


json car::teslaPOST(string url, json package, bool noBearerToken)
{
	string fullUrl = settings::teslaURL + url;
	const char* const url_to_use = fullUrl.c_str();
	lg.d("teslaPOSTing to this URL: " + fullUrl);

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
			lg.p("Sending this header" + settings::teslaAuthString);
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
			lg.i("Response code: " + std::to_string(response_code));
		}

		/* always cleanup */
		curl_easy_cleanup(curl);
	}
	curl_global_cleanup();

	json jsonReadBuffer = json::parse(readBuffer);
	return jsonReadBuffer;
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
		cout << "cteslaauth header is:";
		cout << authHeaderC << endl;

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
			lg.i("Response code: " + std::to_string(response_code));
		}

		/* always cleanup */
		curl_easy_cleanup(curl);
	}
	curl_global_cleanup();
	lg.p(readBuffer);
	json jsonReadBuffer = json::parse(readBuffer);
	// Inside "response" is an array, the first item is what contains the response:
	json responseObject = (jsonReadBuffer["response"])[0];
	return responseObject;
}


bool car::wake()
{
	// Test TeslaPOST with climate on cmd
	json j;
	teslaPOST(settings::teslaVURL + "command/auto_conditioning_start", j);
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
		lg.p("Interior temp is " + std::to_string(interior_temp) + "C, using summer curve.");
		rawTempTimeModifier = pow(interior_temp, 2) / 40 - interior_temp + (10 + defaultMinTime);
		if (rawTempTimeModifier >= 16)
		{
			// Capped at 16 mins when super hot in car
			lg.p("Calculated temp time is " + std::to_string(rawTempTimeModifier) + " mins, capping at 16 mins.");
			rawTempTimeModifier = 16;
		}
	}
	else
	{
		lg.p("Interior temp is " + std::to_string(interior_temp) + "C, using winter curve.");
		rawTempTimeModifier = pow(interior_temp, 2) / 300 - interior_temp / 2 + (9 + defaultMinTime);
		if (rawTempTimeModifier >= 32)
		{
			// Capped at 32 mins when super cold in car
			lg.p("Calculated temp time is " + std::to_string(rawTempTimeModifier) + " mins, capping at 32 mins.");
			rawTempTimeModifier = 32;
		}
	}

	// Round up
	finalTempTimeModifier = static_cast<int>(rawTempTimeModifier + 0.5);
	if (finalTempTimeModifier <= 2)
	{
		lg.d("rawTempTimeModifier was calculated as " + std::to_string(rawTempTimeModifier) + " mins, probably because default20CMinTime is set to "
			+ settings::u_default20CMinTime + " mins.");
		finalTempTimeModifier = 2;
		lg.d("Calculated TempTimeModifier too low, bottoming out at 2 minutes");
	}
	lg.d("Interior car temp is " + std::to_string(interior_temp) + "C, tempTimeModifier is: " + std::to_string(finalTempTimeModifier) + " minutes");

	return finalTempTimeModifier;
}