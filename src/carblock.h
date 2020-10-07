#pragma once
#include "unordered_map"

typedef std::unordered_map<string, string> umap;

class car
{
public:
	// public vars
	bool carAwake = false;
	umap carOutput;

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
	// Private functions
	/*
	for each elem in list of properties
	if (type) ele = [correct type it should be in json]
	ele = json data value
	else
	break
	*/

public:
	// Public functions
	umap getData(string log = "");

	void wake();
};