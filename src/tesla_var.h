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
const string tcs_buildInfo = "\nBuild Date : 2021.01.09 - debug version for wakeTempWait";
const string tcs_version = "3.0.4.1s";
const string tcs_versionInfo = "\nVersion: _*" + tcs_version + "*_" + tcs_buildInfo;


// Declarations
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
string curl_GET(string url);
extern time_t nowTime_secs;
const string string_time_and_date(tm tstruct);
const string return_current_time_and_date();

