#include "logger.h"
#include "tesla_var.h"

using std::cout;
using std::endl;
using std::cin;
using std::string;



/* Constructor that'll take a string for parameter,
	will be inserted before each log message to make sure we know what file its from */
Log::Log(string sourceFile)
{
	source_file = sourceFile;
}


// Set level of logging ONCE for whole program, and print whatever the level is
void Log::SetLevel(int level)
{
	string tempLevelVar;
	m_LogLevel = level;
	if (level == 0)
		tempLevelVar = "[Errors only]";
	else if (level == 1)
		tempLevelVar = "[Normal - Info & errors]";
	else if (level == 2)
		tempLevelVar = "[Debug - all messages]";
	else if (level == 3)
		tempLevelVar = "[Programming - High debugging level messages]";
	cout << "\nLog level is: " << tempLevelVar << "\n\n";
}

int Log::ReadLevel()
{
	return m_LogLevel;
}



string Log::notify(string message)
{
	string res = curl_POST(settings::u_slackChannel, message);
	return res;
}


// Logging functions for e-Error, i-Info, d-Debug
void Log::e(string message, bool notification)
{
	if (m_LogLevel >= LevelError)
		cout << "**[" << source_file << " ERROR]** " << message << endl;
	if (notification)
		notify("*!!ERROR:* " + message);
}
void Log::i(string message, bool notification)
{
	if (m_LogLevel >= LevelInfo)
		cout << "[" << source_file << " info] " << message << endl;
	if (notification)
		notify(message);
}
void Log::d(string message, bool notification)
{
	if (m_LogLevel >= LevelDebug)
		cout << "[" << source_file << " Debug] " << message << endl;
	if (notification)
		notify(message);
}
void Log::p(string message)
{
	if (m_LogLevel >= LevelProgramming)
		cout << "[.." << source_file << " Programming..] " << message << endl;
}
void Log::n(string message)
{
	string res = notify(message);
	i("Notification sent to Slack channel, response code: " + res);
} // Line 69: nice