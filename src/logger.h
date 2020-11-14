#pragma once
#include <iostream>

using std::cout;
using std::endl;
using std::cin;
using std::string;

class Log
{
public:
	// Public variables
	enum LogLevel
	{
		Error = 0, Info, Debug, Programming
	};


	// Public functions
	/* Constructor that'll take a string for parameter,
	will be inserted before each log message to make sure we know what file its from */
	Log(string sourceFile, int level);


	// Read log level for block of text outputs
	int ReadLevel();

	// Logging functions for e-Error, i-Info, d-Debug, n-Notification only
	void e(string message, bool notification = false);
	void i(string message, bool notification = false);
	void d(string message, bool notification = false);
	void p(string message);
	void n(string message);

	// Blank bare message
	void b(string message="");
	

private:
	// Stores the logging level for the file
	int fileLogLevel;
	string source_file;

	//Private functions
	string notify(string message);
	inline void toFile(string message);
	inline string getCurrentDateTime(string s);
	string curl_POST_slack(string url, string message);
};