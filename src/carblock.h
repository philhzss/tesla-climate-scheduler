#include "map"
#pragma once


class car
{
public:
	bool carOnline = false;

	std::map<string, string> carData_s;

	string Tvehicle_name = "";
	string Tshift_state = "";
	string Tconnection_state = "";
	float Tinside_temp;
	float Toutside_temp;
	float Tdriver_temp_setting;
	bool Tis_climate_on;
	float Tusable_battery_level;
	float Tbattery_level;
	string location = "";
	string datapack;


	// TeslaFi data
	string tfiDate = "";



private:
	void wake();

	// Pull data & update map
	void tfiQueryCar(); 

	// Verify if at home or work based on lat/lon and tolerance
	string checkCarLocation();


	// TeslaFi data
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

public:
	// URL request with Tesla Fi, URL is anything after tFi URL
	json tfiInternetOperation(string url = "");
	
	// Pull data from car, waking the car if requested, updating datapack
	std::map<string, string> teslaFiGetData(bool wakeCar, bool manualWakeWait);

	// Returns confirmed heated seat setting, & if max condition is on
	std::vector<string> coldCheckSet();

	int calcTempMod(int interior_temp);

	// Verify various car parameters and return reason if failure
	string triggerAllowed();
};