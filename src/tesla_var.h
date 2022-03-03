#pragma once
#include <iostream>
#include <stdio.h>
#include <vector>
#include <nlohmann/json.hpp>
#include <curl/curl.h>
#include "settings.h"
#include "logger.h"
#include "carblock.h"
#include <fstream>


using json = nlohmann::json;
using std::cout;
using std::endl;
using std::cin;
using std::string;



// Program info
const string tcs_buildInfo = "\nBuild Date : 2022.03.03 - Basic HTTP API";
const string tcs_version = "3.2.0";
const string tcs_versionInfo = "\nVersion: _*" + tcs_version + "*_" + tcs_buildInfo;
const string tcs_userAgent = "Tesla Climate Scheduler/" + tcs_version;

// Declarations
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
size_t header_callback(char* buffer, size_t size,
	size_t nitems, void* userdata);
string curl_GET(string url);
extern time_t nowTime_secs;
const string string_time_and_date(tm tstruct);
const string return_current_time_and_date();

