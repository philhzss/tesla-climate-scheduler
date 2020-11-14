#include "map"
#pragma once



class car
{
public:
	// public vars
	bool carAwake = false;
	std::map<string, string> carData;

private:
	// private vars
	string tfiDate = "";
	string tfiName = "";
	string tficarState = "";
	string tfiState = "";
	string tfiShift = "";
	string tfiLocation = "";
	string tfiIntTemp = "";
	string tfiOutTemp = "";
	string tfiTempSetting = "";
	string tfiIsHvacOn = "";
	string tfiUsableBat = "";
	string tfiBat = "";

	string tstate = "";


private:
	bool wake();
	// Write token to settings::teslaAuthString for Tesla official API usage
	void teslaAuth();

public:
	// Pull data from car, waking the car if requested
	std::map<string, string> getData(bool wakeCar, string log = "");
	
	// POST request to "https://owner-api.teslamotors.com/ + url"
	json teslaPOST(string url, json package, bool noBearerToken=false);

	// RE WRITE TeslaPOST as teslaAUTH, and create another with oAuth built in like GET??


	// GET request to "https://owner-api.teslamotors.com/ + url", oAuth token built-in
	json teslaGET(string url);
	int calcTempMod(int interior_temp);
	
};