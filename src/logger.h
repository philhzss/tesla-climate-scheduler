#pragma once
#include <iostream>
#include <sstream>
#include <iomanip> 


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

	void e(string contents, bool notification = false);
	void i(string contents, bool notification = false);
	void d(string contents, bool notification = false);
	void p(string contents);
	void n(string message);


	// Blank bare message
	void b(string message = "");

	// Should be private eventually:
	// Logger preparator (define outside logger class)
	template<typename ...Args>
	string prepare(Args&&... args)
	{
		std::ostringstream out;
		out.precision(1);
		out << std::fixed << std::boolalpha;
		(out << ... << args);
		return out.str();
	}


private:
	// Stores the logging level for the file
	int fileLogLevel;
	string source_file;

	//Private functions
	string notify(string message);
	inline void toFile(string message);
	inline string getCurrentDateTime(string s);
	string curl_POST_slack(string url, string message);

	void write(string message, string source_file, string level, bool notification);

};