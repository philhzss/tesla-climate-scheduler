#pragma once
#include "tesla_var.h"


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

	// Event start & end time in secs
	time_t startTime_secs;
	time_t endTime_secs;

public:
	// Function to convert string DTSTARTs into tm objects
	void setEventParams(calEvent &event);
	static void removePastEvents();
	static void initEventTimers();
	static void updateValidEventTimers();

	// Whys isnt initiateCal part of calendar.h?

	// Cleanup at end of program
	static void cleanup();

	// Static vars
	static std::vector<calEvent> myCalEvents;
	static std::vector<calEvent> myValidEvents;
	static calEvent* lastTriggeredEvent;
	static calEvent* lastWakeEvent;

	// Static methods
	static string eventTimeCheck(int wakeTimer, int triggerTimer);

	// public start and end times as timeobjects
	tm start{0};
	tm end{0};

	// Difference (in minutes) between program runtime and event start/end time
	int startTimer, endTimer;

	// Keep track of if action has been done for start-end on a given shift
	bool homeDone;
	bool workDone;
	void updateLastTriggeredEvent();
	void updateLastWakeEvent();
	static int getNextWakeTimer(calEvent * event);

	// Custom constructor
	calEvent(string singleEvent_str);
};