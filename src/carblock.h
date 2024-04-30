#include "map"
#pragma once


class car
{
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



private:
	void wake();

	// True if contains "vehicle unavailable"
	bool timeoutButSleeping(string readBuffer);


	// Verify if at home or work based on lat/lon and tolerance
	string checkCarLocation();

public:
	// Pull data from car, waking the car if requested
	std::map<string, string> getData(bool wakeCar = false, bool manualWakeWait = false);

	// POST request to "https://owner-api.teslamotors.com/ + url"
	json teslaPOST(string specifiedUrlPage, json bodyPackage = json());

	// Returns confirmed heated seat setting, & if max condition is on
	std::vector<string> coldCheckSet();

	// GET request to "https://api.teslemetry.com/api/1/vehicles/ + url", with token & vehicle_tag built in
	json teslaGET(string specifiedUrlPage);
	int calcTempMod(int interior_temp);

	// Verify various car parameters and return reason if failure
	string triggerAllowed();
};