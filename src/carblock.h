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
	// Write token to settings::teslaAuthString for Tesla official API usage
	void teslaAuth();



	// Verify if at home or work based on lat/lon and tolerance
	string checkCarLocation();

public:
	// Pull data from car, waking the car if requested
	std::map<string, string> getData(bool wakeCar = false, bool manualWakeWait = false);

	// POST request to "https://owner-api.teslamotors.com/ + url"
	json teslaPOST(string specifiedUrlPage, json bodyPackage = json());

	// Returns confirmed heated seat setting, & if max condition is on
	std::vector<string> coldCheckSet();

	// GET request to "https://owner-api.teslamotors.com/ + url", oAuth token built-in
	json teslaGET(string specifiedUrlPage);
	int calcTempMod(int interior_temp);

	// Verify various car parameters and return reason if failure
	string triggerAllowed();
};