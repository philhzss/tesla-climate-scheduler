#include "calendar.h"
#include "tesla_var.h"
#include "logger.h"

using std::string;

static Log lg("Calendar");


std::vector<calEvent> calEvent::myCalEvents;
std::vector<calEvent> calEvent::myValidEvents;


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
		// If the word "Leave" is found in the individual event strings, don't create custom calEvents
		if (s.find(settings::u_ignoredWord1) != string::npos)
		{
			// Find date of Leave shift for console debug purposes
			string vacayShiftDetector = "DTSTART;VALUE=DATE:";
			int datePos = s.find(vacayShiftDetector);
			string dateOfVacay = s.substr(datePos + vacayShiftDetector.length(), 8);
			lg.i("The event on " + dateOfVacay + " is a " + settings::u_ignoredWord1 + " shift, skipping.");

			continue;
		}
		else if (s.find(settings::u_ignoredWord2) != string::npos)
		{
			// Find date of spare shift for console debug purposes
			string spareShiftDetector = "DTSTART:";
			int datePos = s.find(spareShiftDetector);
			string dateOfSpare = s.substr(datePos + spareShiftDetector.length(), 13);
			lg.d("The event starting at " + dateOfSpare + " is a " + settings::u_ignoredWord2 + " shift, skipping.");
			continue;
		}
		else
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

		lg.p
		(
			"::AFTER modifications WITHIN forLoop::"
			"\nYear=" + (std::to_string(event.start.tm_year)) +
			"\nMonth=" + (std::to_string(event.start.tm_mon)) +
			"\nDay=" + (std::to_string(event.start.tm_mday)) +
			"\nTime=" + (std::to_string(event.start.tm_hour)) + ":" + (std::to_string(event.start.tm_min)) + "\n"
		);
	}
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
	/*lg.p
	(
		"::AFTER modifications::"
		"\nYear=" + (std::to_string(event.start.tm_year)) +
		"\nMonth=" + (std::to_string(event.start.tm_mon)) +
		"\nDay=" + (std::to_string(event.start.tm_mday)) +
		"\nTime=" + (std::to_string(event.start.tm_hour)) + ":" + (std::to_string(event.start.tm_min)) + "\n"
	);*/

	// lg.p("Converting DTEND, raw string is: " + DTEND);
	event.end = convertToTm(event.DTEND, event.end);
	/*lg.p
	(
		"::AFTER modifications::"
		"\nYear=" + (std::to_string(event.end.tm_year)) +
		"\nMonth=" + (std::to_string(event.end.tm_mon)) +
		"\nDay=" + (std::to_string(event.end.tm_mday)) +
		"\nTime=" + (std::to_string(event.end.tm_hour)) + ":" + (std::to_string(event.end.tm_min)) + "\n"
	);*/
}

// Check if any start-end event is valid, and return appropriate string based on timers
string calEvent::eventTimeCheck(int intwakeTimer, int inttriggerTimer)
{
	// Calculate start & end timers for each event and store them in the event instance
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
			"::AFTER modifications by eventTimeCheck::"
			"\nYear=" + (std::to_string(event.end.tm_year)) +
			"\nMonth=" + (std::to_string(event.end.tm_mon)) +
			"\nDay=" + (std::to_string(event.end.tm_mday)) +
			"\nTime=" + (std::to_string(event.end.tm_hour)) + ":" + (std::to_string(event.end.tm_min)) +
			"\nStartTimer=" + (std::to_string(event.startTimer)) +
			"\nEndTimer=" + (std::to_string(event.endTimer)) + "\n"
		);
	}

	// Get rid of all events in the past
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
		lg.d("Past events filtered, there are now " + std::to_string(newSize) + " events in the database, filtered from " + std::to_string(origSize) + " events.");
	}

	// Verify if any event timer is coming up soon (within the defined timer parameter)
	for (calEvent& event : calEvent::myValidEvents)
	{
		// If event start time is less (sooner) than event trigger time
		// startTimer - (commute + start bias)
		if ((event.startTimer - settings::intcommuteTime + settings::intshiftStartBias
			<= inttriggerTimer) && (event.startTimer - settings::intcommuteTime + settings::intshiftStartBias
			> 0))
		{
			lg.p("Triggered by a timer value of: " + std::to_string(event.startTimer));
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
			lg.i("Shift starting at " + string_time_and_date(event.start) + " is valid from home.", true);
			lg.i("Shift was determined valid and triggered at: " + return_current_time_and_date() + " LOCAL\n");
			return "home";
		}
		else if ((event.endTimer - settings::intshiftEndBias) <= inttriggerTimer)
		{
			lg.p("Triggered by a timer value of: " + std::to_string(event.endTimer));
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
			lg.i("Shift starting at " + string_time_and_date(event.end) + " is valid from work.", true);
			lg.i("Shift was determined valid and triggered at: " + return_current_time_and_date() + " LOCAL\n");
			return "work";
		}
		// Make sure to ignore a negative startTimer, as it will be negative during an entire work shift
		else if (((event.startTimer > 0) && (event.startTimer <= intwakeTimer))  || (event.endTimer <= intwakeTimer))
		{
			lg.p("Triggered by a timer value of: " + std::to_string(event.startTimer) + " for start, or: " + std::to_string(event.endTimer) + " for end.");
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
			lg.i("Upcoming shift start/end, waking car to check temp.", true);
			lg.i("Wake event triggered at: " + return_current_time_and_date() + " LOCAL\n");
			return "wake";
		}
		else return "";
	}
}