#include "tesla_var.h"



using std::cout;
using std::endl;
using std::cin;
using std::string;

static Log lg("Logger", Log::LogLevel::Debug);

/* Constructor that'll take a string for parameter,
	will be inserted before each log message to make sure we know what file its from
	Also requires logging level per file now*/
Log::Log(string sourceFile, int level)
{
	source_file = sourceFile;

	string tempLevelVar;
	fileLogLevel = level;
	if (level == 0)
		tempLevelVar = "[Errors only]";
	else if (level == 1)
		tempLevelVar = "[Normal - Info & errors]";
	else if (level == 2)
		tempLevelVar = "[Debug - all messages]";
	else if (level == 3)
		tempLevelVar = "[Programming - High debugging level messages]";
	cout << "Log level for " + source_file + " is: " << tempLevelVar << endl;
}


int Log::ReadLevel()
{
	return fileLogLevel;
}


string Log::curl_POST_slack(string url, string message)
{
	const char* const url_to_use = url.c_str();
	CURL* curl;
	CURLcode res;
	// Buffer to store result temporarily:
	string readBuffer;
	long response_code;

	/* In windows, this will init the winsock stuff */
	curl_global_init(CURL_GLOBAL_ALL);

	/* get a curl handle */
	curl = curl_easy_init();

	if (curl) {
		/* First set the URL that is about to receive our POST. This URL can
		   just as well be a https:// URL if that is what should receive the
		   data. */
		curl_easy_setopt(curl, CURLOPT_URL, url_to_use);
		/* Now specify the POST data */
		struct curl_slist* headers = nullptr;
		headers = curl_slist_append(headers, "Content-Type: application/json");

		string data = "{\"text\":\"" + message + "\"}";
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.length());
		curl_easy_setopt(curl, CURLOPT_POST, 1);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);


		/* Perform the request, res will get the return code */
		res = curl_easy_perform(curl);
		/* Check for errors */
		if (res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
				curl_easy_strerror(res));
		if (res == CURLE_OK) {
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
		}

		/* always cleanup */
		curl_easy_cleanup(curl);
	}
	curl_global_cleanup();
	return std::to_string(response_code);
}



string Log::notify(string message)
{
	if (settings::slackEnabled)
	{
		string res;
		try {
			res = Log::curl_POST_slack(settings::u_slackChannel, message);
		}
		catch (int i) {
			lg.d("Caught error ", i, ", retrying curl_POST_slack() once");
			res = Log::curl_POST_slack(settings::u_slackChannel, message);;
			return res;
		}
		return "[Notif info] Notification sent: " + res;
	}
	else
	{
		return "[Notif info] Notifications disabled, not pushing to Slack";
	}
}


void Log::write(string message, string sourceFileWrite, string levelPrint, bool notification)
{
	string toWrite;
	if ((sourceFileWrite == "") && (levelPrint == ""))
	{
		toWrite = message;
	}
	else {
		toWrite = "[" + sourceFileWrite + " " + levelPrint + "] " + message;
	}
	cout << toWrite << endl;
	if (notification)
		b(notify(message)); // Notify without levelPrint
	if (settings::u_logToFile)
		toFile(toWrite);
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

