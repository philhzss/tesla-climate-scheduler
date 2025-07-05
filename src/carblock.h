#include "map"
#pragma once


class car
{
public:
	// public vars
	bool carOnline = false;

	std::map<string, string> carData_s;

	// TeslaFi data
	string tfiVehicle_name = "";
	string tfiShift_state = "";
	string tfiConnection_state = "";
	string tfiCar_state_activity = "";
	string tfiLocation = "";
	float tfiInside_temp;
	float tfiOutside_temp;
	float tfiDriver_temp_setting;
	bool tfiIs_climate_on;
	float tfiUsable_battery_level;
	float tfiBattery_level;
	string tfiDate = "";

	// Non-TeslaFi Car Data
	string location = "";
	string datapack;

	

private:
	void wake();

	// Pull data & update map
	void tfiQueryCar();

	// Verify if at home or work based on lat/lon and tolerance
	string checkCarLocation();

public:
	// URL request with Tesla Fi, URL is anything after tFi URL
	json tfiInternetOperation(string url = "");

	// Pull data from car, waking the car if requested, updating datapack
	std::map<string, string> tfiGetData(bool wakeCar = false, bool manualWakeWait = false);

	// Returns confirmed heated seat setting, & if max condition is on
	std::vector<string> coldCheckSet();

	int calcTempMod(int interior_temp);

	// Verify various car parameters and return reason if failure
	string triggerAllowed();
};