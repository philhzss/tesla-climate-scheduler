#include "map"
#pragma once

// New auth stuff
class authorizer {
public:
	string random_ANstring();
	string code_verifier;

	const string client_id = "ownerapi";
	string code_challenge;
	const string code_challenge_method = "S256";
	const string redirect_uri = "https://auth.tesla.com/void/callback";
	const string response_type = "code";
	const string scope = "openid%20email%20offline_access";
	const string state = "anyRandomString";


	size_t stringCount(const std::string& referenceString,
		const std::string& subString);
	std::unordered_map<string, string> step1_forms;
	string step1_cookie;


};

class car
{
	friend class authorizer;
public:
	// public vars
	bool carOnline = false;

	bool carAwake = false; // Replace with carOnline?? for Tfi

	std::map<string, string> carData_s;

	string teslaDataUpdateTime = "";
	string Tvehicle_name = "";
	string Tshift_state = "";
	string Tconnection_state = "";
	float Tinside_temp;
	float Toutside_temp;
	float Tdriver_temp_setting;
	bool Tis_climate_on;
	float Tusable_battery_level;
	float Tbattery_level;
	float Tlat;
	float Tlong;
	string location;
	string datapack;
	authorizer auth;



private:
	void wake();
	json tfiRawQuery();

	// Verify if at home or work based on lat/lon and tolerance
	string checkCarLocation();


	// TeslaFi data
	string tfiDate = "";
	string tfiName = "";
	string tficarState = "";
	string tfiConnectionState = "";
	string tfiShift = "";
	string tfiLocation = "";
	string tfiIntTemp = "";
	string tfiOutTemp = "";
	string tfiTempSetting = "";
	string tfiIsHvacOn = "";
	string tfiUsableBat = "";
	string tfiBat = "";

	// Function that checks TFi data, checks if polling disabled, etc
	// WARNING, DOES NOT UPDATE CARDATA MAP!!!!!!!! --> ?? To check, not sure what this means (2024)
	std::map<string, string> teslaFiGetData(bool wakeCar, bool manualWakeWait);


public:
	// Pull data from car, waking the car if requested
	std::map<string, string> getData(bool wakeCar = false, bool manualWakeWait = false);

	// Returns confirmed heated seat setting, & if max condition is on
	std::vector<string> coldCheckSet();

	int calcTempMod(int interior_temp);

	// Verify various car parameters and return reason if failure
	string triggerAllowed();
};