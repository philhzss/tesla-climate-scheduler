#pragma once
#include <iostream>
#include <stdio.h>
#include <vector>
#include <nlohmann/json.hpp>
#include <curl/curl.h>
#include "settings.h"
#include "logger.h"


using json = nlohmann::json;
using std::cout;
using std::endl;
using std::cin;
using std::string;


// Program info
const string tcs_versionInfo = "\nVersion: _*3.0 pre-release*_\nBuild Date: 2020.10.05 - First C++ version";


// Declarations
void testFunc();
string curl_GET(string url);
string curl_POST(string url, string message);
extern time_t nowTime_secs;
const string string_time_and_date(tm tstruct);



// How long to wait before the program loops entirely, in SECONDS
const int repeatDelay = 7;




// EVERYTHING BELOW THIS IS CURRENTLY NOT IN USE
// COMES FROM THE OLD PYTHON VERSION OF THIS PROGRAM (2.5)
// MIGHT BE DELETED

/*Set time & temp parameters here, in DEGREES C and MINS:
	coldX_temp is the temp limit(will trigger if VALUE or colder), in DEGREES C
	cX_deltaTime is how long the car should start before YOU LEAVE, in MINS
	Higher cX_deltaTimes mean the car will start HVAC* earlier*/
const int cold3_temp = -20;
const int c3_deltaTime = 35;

const int cold2_temp = -8;
const int c2_deltaTime = 26;

const int cold1_temp = 2;
const int c1_deltaTime = 17;

// "Else" timer, if temp is above cold1Temp, preheats this amount of time before you LEAVE:
const int else_deltaTime = 13;

// comfy_deltaTime : If outside temp is between 17 and 24C, car will preheat this about of mins before you LEAVE to save power :
const int comfy_deltaTime = 7;