#include "map"
#pragma once



class car
{
public:
	// public vars
	bool carOnline = false;
	std::map<string, string> carData_s;
	
	string Tdisplay_name = "";
	string Tshift_state = "";
	float Tinside_temp;
	float Toutside_temp;
	float Tdriver_temp_setting;
	bool Tis_climate_on;
	float Tusable_battery_level;
	float Tbattery_level;

	string _state = "";


private:
	void wake();
	// Write token to settings::teslaAuthString for Tesla official API usage
	void teslaAuth();


public:
	// Pull data from car, waking the car if requested
	std::map<string, string> getData(bool wakeCar=false);
	
	// POST request to "https://owner-api.teslamotors.com/ + url"
	json teslaPOST(string url, json package=json(), bool noBearerToken=false);

	// RE WRITE TeslaPOST as teslaAUTH, and create another with oAuth built in like GET??


	// GET request to "https://owner-api.teslamotors.com/ + url", oAuth token built-in
	json teslaGET(string url);
	int calcTempMod(int interior_temp);
	
};