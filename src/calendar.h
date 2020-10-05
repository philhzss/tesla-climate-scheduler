#pragma once
#include <iostream>
#include "tesla_var.h"
#include <vector>


using std::string;

//temp
string get_str_between_two_str(const string& s,
	const std::string& start_delim,
	const std::string& stop_delim);
//temp



void initiateCal();


class calEvent
{
private:
	string DTSTART, DTEND, DESCRIPTION;

public:
	// Function to convert string DTSTARTs into tm objects
	void setEventParams(calEvent &event);

	// Static vars
	static std::vector<calEvent> myCalEvents;
	static std::vector<calEvent> myValidEvents;

	// Static methods
	static string eventTimeCheck(int wakeTimer, int triggerTimer);

	// public start and end times as timeobjects
	tm start{0};
	tm end{0};

	// Difference (in minutes) between program runtime and event start/end time
	int startTimer, endTimer;

	// Custom constructor
	calEvent(string singleEvent_str);
};