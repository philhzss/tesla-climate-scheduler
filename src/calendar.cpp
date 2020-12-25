#include "calendar.h"
#include "tesla_var.h"
#include "logger.h"

using std::string;

static Log lg("Calendar", Log::LogLevel::Debug);


std::vector<calEvent> calEvent::myCalEvents;
std::vector<calEvent> calEvent::myValidEvents;
calEvent* calEvent::lastTriggeredEvent;


// Get long string of raw calendar data from URL
string GetCalRawData() {
	try
	{
		string rawCalContent = curl_GET(settings::u_calendarURL);
		return rawCalContent;
	}
	catch (string e)
	{
		throw "initiateCal error: " + e + "\nCheck calendar URL??? Internet connection?";
	};
}

// Take the entire ICS file string and convert it into vector of strings with events
std::vector<string> calStringSep(string entire_ical_str)
{
	std::vector<string> vectorOfEvents;

	// We are stopping when we reach this string:
	string delimiter = "END:VEVENT";

	// Initial, start at position 0
	size_t last = 0;
	size_t next = 0;
	string token;

	while ((next = entire_ical_str.find(delimiter, last)) != string::npos)
	{
		string a = entire_ical_str.substr(last, next - last);
		/* cout << "Next event here" << endl;
		cout << a << endl; */
		vectorOfEvents.push_back(a);
		// + 11 chars to cutoff the END:VEVENT and make the strings start with BEGIN
		last = next + 11;
	}
	return vectorOfEvents;
}

// From StackOverflow website
// 1-String, 2-Start delim, 3-End delim
string get_str_between_two_str(const string& s,
	const std::string& start_delim,
	const std::string& stop_delim)
{
	unsigned first_delim_pos = s.find(start_delim);
	unsigned end_pos_of_first_delim = first_delim_pos + start_delim.length();
	// (Phil mod); make it start at end_post_of_first_delim instead of at pos 0, otherwise doesnt do shit right
	unsigned last_delim_pos = s.find(stop_delim, end_pos_of_first_delim);

	// Substr that start at the end of first delim, and ends at
	return s.substr(end_pos_of_first_delim,
		last_delim_pos - end_pos_of_first_delim);
}

// Constructor for calEvent object type
calEvent::calEvent(string singleEvent_str)
{
	// Delimiters for finding values
	string startString_delim = "DTSTART:";
	string startString_delimEND = "Z";
	string endString_delim = "DTEND:";
	string endString_delimEND = "Z";
	string description_delim = "(Eastern Standard Time)";
	string description_delimEND = "refres";

	// Get and store the raw strings for these private vars
	DTEND = get_str_between_two_str(singleEvent_str, endString_delim, endString_delimEND);
	DTSTART = get_str_between_two_str(singleEvent_str, startString_delim, startString_delimEND);
	DESCRIPTION = get_str_between_two_str(singleEvent_str, description_delim, description_delimEND);

	// Debug stuff;
	/*cout << "A calEvent Object DTSTART:" << endl;
	cout << DTSTART << endl;
	cout << "A calEvent Object DTEND:" << endl;
	cout << DTEND << endl;
	cout << "A calEvent Object DESCRIPTION" << endl;
	cout << DESCRIPTION << endl;*/
}


// Function to be called from main.cpp, creates myCalEvents vector containing all calEvent objects from scratch
void initiateCal()
{
	string calRawData = GetCalRawData();

	// Cut out the "header" data from the raw cal string
	int	calHeadEndPos = calRawData.find("END:VTIMEZONE");
	calRawData = calRawData.substr(calHeadEndPos);

	// Create custom calEvents and stick them in a vector (code by Carl!!!!)
	std::vector<string> calEventsVector = calStringSep(calRawData);


	for (string s : calEventsVector)
	{
		bool noIgnoredWordsInEvent = true;
		if (settings::ignoredWordsExist()) {
			// Check for every word in the words to ignore vector, for every event.
			for (string ignoredWord : settings::u_wordsToIgnore)
			{
				lg.p("Checking word ", ignoredWord);
				if (s.find(ignoredWord) != string::npos)
				{
					// Tell the user what is the date of the ignored event for debugging
					string ignoredWordDTSTART = "DTSTART";
					int datePos = s.find(ignoredWordDTSTART);
					// sFromDTSTART is DTSTART: the date and some extra (40 chars total)
					string sFromDTSTART = s.substr(datePos, 40);
					// Start at where the DTSTART starts, add its length, +1 for the : or ; which changes
					// -10 is because I must subtract the length of the word DTSTART + 1, AND the first char of LOC (L). I think
					string dateOfignoredWord = s.substr(datePos + ignoredWordDTSTART.length() + 1, sFromDTSTART.find("LOC") - 10);
					// Remove the first 11 chars (they are VALUE=DATE:) if its an all day event
					dateOfignoredWord = (dateOfignoredWord.find("VALUE=") != string::npos) ? dateOfignoredWord.erase(0, 11) : dateOfignoredWord;
					lg.p("Ignored word \"", ignoredWord, "\" found in event on ", dateOfignoredWord, " skipping.");
					noIgnoredWordsInEvent = false;
					break;
				}
				else {
					noIgnoredWordsInEvent = true;
				}
			}
		}
		else {
			noIgnoredWordsInEvent = true;
		}
		if (noIgnoredWordsInEvent)
		{
			calEvent::myCalEvents.push_back(calEvent(s));
		}

	}

	// The custom myCalEvents vector is initialized
	lg.i("There are " + std::to_string(calEvent::myCalEvents.size()) + " events in the database, filtered from " + std::to_string(calEventsVector.size()) + " events. This includes past events, will be filtered later.");

	// Parse myCalEvents items to get their event start-end times set as datetime objects
	for (calEvent& event : calEvent::myCalEvents)
	{
		event.setEventParams(event);
	}

	// Calculate start & end timers for each event and store them in the event instance
	calEvent::initEventTimers();

	// Get rid of all events in the past
	calEvent::removePastEvents();
}

// Convert raw DTSTART/DTEND strings into datetime objects
void calEvent::setEventParams(calEvent& event)
{
	// Convert str to tm struct taking (raw string & reference to timeStruct to store time in)
	auto convertToTm = [](string rawstr, tm& timeStruct)
	{
		int year, month, day, hour, min, sec;
		const char* rawCstr = rawstr.c_str();
		// Example string: DTSTART:20200827T103000Z
		sscanf(rawCstr, "%4d%2d%2d%*1c%2d%2d%2d", &year, &month, &day, &hour, &min, &sec);

		// Set struct values, (Year-1900 and month counts from 0)
		timeStruct.tm_year = year - 1900;
		timeStruct.tm_mon = month - 1;
		timeStruct.tm_mday = day;
		timeStruct.tm_hour = hour;
		timeStruct.tm_min = min;
		timeStruct.tm_sec = sec;

		/*cout << "Year is " << timeStruct.tm_year << endl;
		cout << "Month is " << timeStruct.tm_mon << endl;
		cout << "Day is " << timeStruct.tm_mday << endl;
		cout << "Hour is " << timeStruct.tm_hour << endl;
		cout << "Minute is " << timeStruct.tm_min << endl;*/
		return timeStruct;
	};

	// lg.p("Converting DTSTART, raw string is: " + DTSTART);
	event.start = convertToTm(event.DTSTART, event.start);
	event.end = convertToTm(event.DTEND, event.end);
	event.homeDone = false;
	event.workDone = false;
}

void calEvent::initEventTimers()
{
	for (calEvent& event : calEvent::myCalEvents)
	{
		/*lg.p
		(
			"::BEFORE modifications by eventTimeCheck::"
			"\nYear=" + (std::to_string(event.end.tm_year)) +
			"\nMonth=" + (std::to_string(event.end.tm_mon)) +
			"\nDay=" + (std::to_string(event.end.tm_mday)) +
			"\nTime=" + (std::to_string(event.end.tm_hour)) + ":" + (std::to_string(event.end.tm_min)) + "\n"
		);*/
		// Make a temporary tm struct to not let the mktime function overwrite my event struct
		tm tempEventStart = event.start;
		tm tempEventEnd = event.end;
		// Using the temp tm structs, convert tm to time_t epoch seconds
		time_t startTime_secs = mktime(&tempEventStart) - timezone;
		time_t endTime_secs = mktime(&tempEventEnd) - timezone;
		lg.p("Now time in secs: " + std::to_string(nowTime_secs));
		lg.p("Start time in secs: " + std::to_string(startTime_secs));
		lg.p("End time in secs: " + std::to_string(endTime_secs));
		// Calculate diff between now and event start/stop times, store value in minutes in object timers
		event.startTimer = (difftime(startTime_secs, nowTime_secs)) / 60;
		event.endTimer = (difftime(endTime_secs, nowTime_secs)) / 60;
		lg.p
		(
			"::AFTER modifications by initEventTimers, based on shift END TIME::"
			"\nYear=" + (std::to_string(event.end.tm_year)) +
			"\nMonth=" + (std::to_string(event.end.tm_mon)) +
			"\nDay=" + (std::to_string(event.end.tm_mday)) +
			"\nTime=" + (std::to_string(event.end.tm_hour)) + ":" + (std::to_string(event.end.tm_min)) +
			"\nStartTimer=" + (std::to_string(event.startTimer)) +
			"\nEndTimer=" + (std::to_string(event.endTimer)) + "\n"
		);
	}
}

void calEvent::updateValidEventTimers()
{
	for (calEvent& event : calEvent::myValidEvents) // applies to valid (non-past) events only
	{
		// Make a temporary tm struct to not let the mktime function overwrite my event struct
		tm tempEventStart = event.start;
		tm tempEventEnd = event.end;

		// Using the temp tm structs, convert tm to time_t epoch seconds
		time_t startTime_secs = mktime(&tempEventStart) - timezone;
		time_t endTime_secs = mktime(&tempEventEnd) - timezone;

		// Calculate diff between now and event start/stop times, store value in minutes in object timers
		event.startTimer = (difftime(startTime_secs, nowTime_secs)) / 60;
		event.endTimer = (difftime(endTime_secs, nowTime_secs)) / 60;
	}
}

void calEvent::removePastEvents()
{
	int origSize = myCalEvents.size();
	for (calEvent& event : calEvent::myCalEvents)
	{

		if ((event.startTimer > 0) || (event.endTimer > 0))
		{
			calEvent::myValidEvents.push_back(event);
		}

	}
	int newSize = myValidEvents.size();
	lg.i("Past events filtered, there are now " + std::to_string(newSize) + " events in the database, filtered from " + std::to_string(origSize) + " events.");

	// Only print this if level is higher than programming as it's a lot of lines
	if (lg.ReadLevel() >= Log::Programming) {
		lg.d("All events INCLUDING past:");
		for (calEvent event : calEvent::myCalEvents)
		{
			lg.d("Start");
			lg.d(event.DTSTART);
			lg.d("End");
			lg.d(event.DTEND);
			lg.b();
		}
		lg.b("\n");
		lg.d("All events EXCLUDINTG past (valid only):");
		for (calEvent event : calEvent::myValidEvents)
		{
			lg.d("Start");
			lg.d(event.DTSTART);
			lg.d("End");
			lg.d(event.DTEND);
			lg.b();
		}
	}
}

// Check if any start-end event is valid, and return appropriate string based on timers
string calEvent::eventTimeCheck(int intwakeTimer, int inttriggerTimer)
{
	// Verify if any event timer is coming up soon (within the defined timer parameter) and return location-validity
	for (calEvent& event : calEvent::myValidEvents)
	{
		// If event start time is less (sooner) than event trigger time
		// startTimer - (commute + start bias)
		if (event.startTimer > 0 && (event.startTimer - settings::intcommuteTime + settings::intshiftStartBias
			<= inttriggerTimer))
		{
			lg.d("Triggered by a timer value of: " + std::to_string(event.startTimer));
			lg.p
			(
				"::Trigger debug (event start)::"
				"\nYear=" + (std::to_string(event.end.tm_year)) +
				"\nMonth=" + (std::to_string(event.end.tm_mon)) +
				"\nDay=" + (std::to_string(event.end.tm_mday)) +
				"\nTime=" + (std::to_string(event.end.tm_hour)) + ":" + (std::to_string(event.end.tm_min)) +
				"\nStartTimer=" + (std::to_string(event.startTimer)) +
				"\nEndTimer=" + (std::to_string(event.endTimer)) + "\n"
			);
			lg.i("Shift starting at " + string_time_and_date(event.start) + " is valid from home.");
			lg.i("Shift was determined valid and triggered at: " + return_current_time_and_date() + " LOCAL");

			if (!event.homeDone) // make sure this event hasn't be triggered before for home
			{
				// Return home since this shift is triggered and its "going to work"
				event.updateLastTriggeredEvent();
				return "home";
			}
			else
			{
				lg.i("EVENT HAD ALREADY TRIGGERED FOR HOME, ignoring.");
				return "duplicate";
			}
		}
		else if (event.endTimer > 0 && (event.endTimer + settings::intshiftEndBias <= inttriggerTimer))
		{
			lg.d("Triggered by a timer value of: " + std::to_string(event.endTimer));
			lg.p
			(
				"::Trigger debug (event end)::"
				"\nYear=" + (std::to_string(event.end.tm_year)) +
				"\nMonth=" + (std::to_string(event.end.tm_mon)) +
				"\nDay=" + (std::to_string(event.end.tm_mday)) +
				"\nTime=" + (std::to_string(event.end.tm_hour)) + ":" + (std::to_string(event.end.tm_min)) +
				"\nStartTimer=" + (std::to_string(event.startTimer)) +
				"\nEndTimer=" + (std::to_string(event.endTimer)) + "\n"
			);
			lg.i("Shift starting at " + string_time_and_date(event.end) + " is valid from work.");
			lg.i("Shift was determined valid and triggered at: " + return_current_time_and_date() + " LOCAL\n");

			if (!event.workDone) // make sure this event hasn't be triggered before for work
			{
				// Return work since this shift is triggered and its "coming back from to work"
				event.updateLastTriggeredEvent();
				return "work";
			}
			else
			{
				lg.i("EVENT HAD ALREADY TRIGGERED FOR WORK, ignoring.");
				return "duplicate";
			}
		}
		// Make sure to ignore a negative startTimer, as it will be negative during an entire work shift
		else if (event.startTimer > 0 && event.startTimer <= intwakeTimer || event.endTimer <= intwakeTimer && event.endTimer > 0)
		{
			lg.d("Triggered by a timer value of: " + std::to_string(event.startTimer) + " for start, or: " + std::to_string(event.endTimer) + " for end.");
			lg.p
			(
				"::Trigger debug (wakeTimer)::"
				"\nYear=" + (std::to_string(event.end.tm_year)) +
				"\nMonth=" + (std::to_string(event.end.tm_mon)) +
				"\nDay=" + (std::to_string(event.end.tm_mday)) +
				"\nTime=" + (std::to_string(event.end.tm_hour)) + ":" + (std::to_string(event.end.tm_min)) +
				"\nStartTimer=" + (std::to_string(event.startTimer)) +
				"\nEndTimer=" + (std::to_string(event.endTimer)) + "\n"
			);
			// Notification for wake action now in main.cpp
			lg.i("Wake event triggered at: " + return_current_time_and_date() + " LOCAL\n");
			// Return wake since this shift is upcoming but not close enough to start the car yet
			return "wake";
		}
		else
		{
			lg.p("No match for start: " + std::to_string(event.startTimer) + " NOR for end: " + std::to_string(event.endTimer) + ".");
		}
	}
	// If we're here, no event matched any parameter
	lg.d("No matching event for wakeTimer: ", intwakeTimer, "mins, triggerTimer: ", inttriggerTimer, "mins.");
	return "";
}

void calEvent::updateLastTriggeredEvent()
{
	lg.d("lastTriggeredEvent has been updated");
	lastTriggeredEvent = this;
}

void calEvent::cleanup()
{
	myCalEvents.clear();
	myValidEvents.clear();
}