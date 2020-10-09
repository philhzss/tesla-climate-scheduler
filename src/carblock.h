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
	string auth();

public:
	// Public functions
	std::map<string, string> getData(string log = "");
	void wake();
	int calcTempMod(int interior_temp);
};