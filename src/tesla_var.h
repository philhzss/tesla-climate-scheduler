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
#include <thread>


using json = nlohmann::json;
using std::cout;
using std::endl;
using std::cin;
using std::string;



// Program info
const string tcs_buildInfo = "\nBuild Date : 2023.10.30 - curl_timeout fix";
const string tcs_version = "3.6.0";
const string tcs_versionInfo = "\nVersion: _*" + tcs_version + "*_" + tcs_buildInfo;
const string tcs_userAgent = "Tesla Climate Scheduler/" + tcs_version;

// Declarations
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
size_t header_callback(char* buffer, size_t size,
	size_t nitems, void* userdata);
string curl_GET(string url);
extern time_t nowTime_secs;
const string string_time_and_date(tm tstruct);
const string date_time_str_from_time_t(const char * format = "%Y-%m-%d %X", time_t * time_t_secs = nullptr);

// Wait for X number of time, checking the API for a request every second. Silent (no log)
void sleepWithAPIcheck(int totalSleepTime);