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


private:
	

public:
	// Pull data about car without sending any commands
	std::map<string, string> getData(string log = "");
	// Send POST request to "https://owner-api.teslamotors.com/ + url"
	json teslaPOST(string url, json package);
	json teslaGET(string url);
	void wake();
	int calcTempMod(int interior_temp);
	
};