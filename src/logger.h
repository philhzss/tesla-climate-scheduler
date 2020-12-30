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
	template<typename ...Args>
	void e(Args&&... args)
	{
		if (fileLogLevel >= Error)
		{
			string message = Log::prepare(std::forward<Args>(args)...);
			write(message, source_file, "**ERROR**");
		}
	}
	template<typename ...Args>
	void en(Args&&... args)
	{
		if (fileLogLevel >= Error)
		{
			string message = Log::prepare(std::forward<Args>(args)...);
			write(message, source_file, "**ERROR**", true);
		}
	}

	template<typename ...Args>
	void i(Args&&... args)
	{
		if (fileLogLevel >= Info)
		{
			string message = Log::prepare(std::forward<Args>(args)...);
			write(message, source_file, "Info");
		}
	}
	template<typename ...Args>
	void in(Args&&... args)
	{
		if (fileLogLevel >= Info)
		{
			string message = Log::prepare(std::forward<Args>(args)...);
			write(message, source_file, "Info", true);
		}
	}

	template<typename ...Args>
	void d(Args&&... args)
	{
		if (fileLogLevel >= Debug)
		{
			string message = Log::prepare(std::forward<Args>(args)...);
			write(message, source_file, "Debug");
		}
	}
	template<typename ...Args>
	void dn(Args&&... args)
	{
		if (fileLogLevel >= Debug)
		{
			string message = Log::prepare(std::forward<Args>(args)...);
			write(message, source_file, "Debug", true);
		}
	}

	template<typename ...Args>
	void p(Args&&... args)
	{
		if (fileLogLevel >= Programming)
		{
			string message = Log::prepare(std::forward<Args>(args)...);
			write(message, source_file, "..Programming..");
		}
	}

	template<typename ...Args>
	void n(Args&&... args)
	{
		string message = Log::prepare(std::forward<Args>(args)...);
		b(notify(message));
	}

	template<typename ...Args>
	void b(Args&&... args)
	{
		string message = Log::prepare(std::forward<Args>(args)...);
		write(message, "", "");
	}

	template<typename ...Args>
	string prepareOnly(Args&&... args)
	{
		string message = Log::prepare(std::forward<Args>(args)...);
		return message;
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

	void write(string message, string source_file, string level, bool notification = false);


	// Logger preparator
	template<typename ...Args>
	string prepare(Args&&... args)
	{
		std::ostringstream out;
		out.precision(1);
		out << std::fixed << std::boolalpha;
		(out << ... << args);
		return out.str();
	}
};