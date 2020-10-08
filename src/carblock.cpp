#include "tesla_var.h"
#include "logger.h"
#include "carblock.h"



using std::cout;
using std::endl;
using std::cin;
using std::string;




// Log file name for console messages
static Log lg("Carblock");




std::map<string, string> car::getData(string log)
{
	try
	{
		string readBufferData = curl_GET(settings::tfiURL);

		// Parse and log the data from tfi, after making sure you're authorized
		lg.b();
		lg.d("Raw data from TeslaFi: " + readBufferData);
		if (readBufferData.find("unauthorized") != string::npos)
		{
			lg.e("TeslaFi access denied.");
			throw string("TeslaFi token incorrect or expired.");
		}
		json jsonTfiData = json::parse(readBufferData);

		// Display all the data and store in variables
		tfiDate = jsonTfiData["Date"];
		lg.d("tfiDate: " + tfiDate);
		if (jsonTfiData["display_name"].type() == json::value_t::string) {
			// Car is not sleeping, no need to wake
			lg.d("Car display_name is a string (good), car is awake. Getting more data.");
			tfiName = jsonTfiData["display_name"];
			lg.d("tfiName: " + tfiName);
			tfiIntTemp = jsonTfiData["inside_temp"];
			lg.d("tfiIntTemp: " + tfiIntTemp);
			tfiOutTemp = jsonTfiData["outside_temp"];
			lg.d("tfiOutTemp: " + tfiOutTemp);
			tfiTempSetting = jsonTfiData["driver_temp_setting"];
			lg.d("tfiTempSetting: " + tfiTempSetting);
			tfiIsHvacOn = jsonTfiData["is_climate_on"];
			lg.d("tfiIsHvacOn: " + tfiIsHvacOn);
			tfiUsableBat = jsonTfiData["usable_battery_level"];
			lg.d("tfiUsableBat: " + tfiUsableBat);
			tfiBat = jsonTfiData["battery_level"];
			lg.d("tfiBat :" + tfiBat);
			carAwake = true;
		}
		else {
			// Car is sleeping or trying to sleep, will have to wake
			lg.d("Car display_name is null, not a string, car is most likely sleeping OR trying to sleep.");
			carAwake = false;
		}
		tficarState = jsonTfiData["carState"];
		lg.d("tficarState: " + tficarState);
		tfiState = jsonTfiData["state"];
		lg.d("tfiState (connection state): " + tfiState);
		// If the connection state is reported as online, but no other data can be pulled, car is in Tesla Fi sleep mode attempt. Detect this and assume car is asleep if so:
		if ((tfiState == "online") and (jsonTfiData["display_name"].type() != json::value_t::string))
		{
			lg.i("The car is trying to sleep, sleep mode assumed, will attempt to wake.");
			carAwake = false;

		}
		try {
			string tfiShift = jsonTfiData["shift_state"];
		}
		catch (nlohmann::detail::type_error)
		{
			lg.d("json type_error for shift_state in TeslaFi, car assumed to be in park, manually writing");
			tfiShift = "P";
		}
		lg.d("tfiShift: " + tfiShift);
		tfiLocation = jsonTfiData["location"];
		lg.d("tfiLocation: " + tfiLocation);
	}
	catch (string e)
	{
		lg.e("CURL TeslaFi exception: " + e);
		throw string("Can't get car data from Tesla Fi. (CURL problem?)");
	}
	// maybe change to an ordered map, the console log order is annoying
	carData["Success"] = "True";
	carData["Car awake"] = std::to_string(carAwake);
	carData["Tesla Fi Date"] = tfiDate;
	carData["Tesla Fi Name"] = tfiName;
	carData["Tesla Fi Car State"] = tficarState;
	carData["Tesla Fi Connection State"] = tfiState;
	carData["Tesla Fi Shift State"] = tfiShift;
	carData["Tesla Fi Location"] = tfiLocation;
	carData["Tesla Fi Inside temp"] = tfiIntTemp;
	carData["Tesla Fi Outside temp"] = tfiOutTemp;
	carData["Tesla Fi Driver Temp Setting"] = tfiTempSetting;
	carData["Tesla Fi is HVAC On"] = tfiIsHvacOn;
	carData["Tesla Fi Usable Battery"] = tfiUsableBat;
	carData["Tesla Fi Battery level"] = tfiBat;
	lg.b();

	if (!log.empty())
	{
		lg.i("getData result:");
	
		for (std::pair<string, string> element : carData)
		{
			lg.i(element.first + ": " + element.second);
		}
	}
	return carData;
}

void car::wake()
{

}

//void testFunc()
//{
//	car Tesla;
//	umap carOutput = Tesla.getData();
//	lg.i("getData result (unordered map):");
//	
//	cout << "Car output test" << endl;
//	cout << carOutput["Tesla Fi Outside temp"] << endl;
//	cout << "tadaa";
//}