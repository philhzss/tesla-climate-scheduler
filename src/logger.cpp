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
	{
		cout << "**[" << source_file << " ERROR]** " << message << endl;
		if (notification)
			notify("*!!ERROR:* " + message);
		if (settings::u_logToFile == "true")
			toFile("**[" + source_file + " ERROR]** " + message);
	}
}
void Log::i(string message, bool notification)
{
	if (m_LogLevel >= LevelInfo)
	{
		cout << "[" << source_file << " info] " << message << endl;
		if (settings::u_logToFile == "true")
			toFile("[" + source_file + " info] " + message);
		if (notification)
			notify(message);
	}
}
void Log::d(string message, bool notification)
{
	if (m_LogLevel >= LevelDebug)
	{
		cout << "[" << source_file << " Debug] " << message << endl;
		if (settings::u_logToFile == "true")
			toFile("[" + source_file + " Debug] " + message);
		if (notification)
			notify(message);
	}
}
void Log::p(string message)
{
	if (m_LogLevel >= LevelProgramming)
	{
		cout << "[.." << source_file << " Programming..] " << message << endl;
		if (settings::u_logToFile == "true")
			toFile("[" + source_file + " Programming] " + message);
	}
}
void Log::n(string message)
{
	string res = notify(message);
	i("Notification sent to Slack channel, response code: " + res);
}
void Log::b(string message)
{
	cout << message << endl;
}


// Logging to file functions
inline string Log::getCurrentDateTime(string s) {
	time_t now = time(0);
	struct tm  tstruct;
	char  buf[80];
	tstruct = *localtime(&now);
	if (s == "now")
		strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);
	else if (s == "date")
		strftime(buf, sizeof(buf), "%Y-%m-%d", &tstruct);
	return string(buf);
};
inline void Log::toFile(string message) {

	string filePath = "log_" + Log::getCurrentDateTime("date") + ".txt";
	string now = Log::getCurrentDateTime("now");
	std::ofstream ofs(filePath.c_str(), std::ios_base::out | std::ios_base::app);
	ofs << now << '\t' << message << '\n';
	ofs.close();
}