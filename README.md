# Tesla Climate Scheduler (tcs)
Control your Tesla's climate & heated seats based on events from a .ics file

#### Currently incomplete, no release available at this time


### Important notes
+ **This is my first program** and was written primarily for use by myself. Will probably not work out of the box for your use case, but feel free to use/tweak my code if it can be useful to you!
+ Tested on ARM only. Specifically, I run this program continuously on a Raspberry Pi 4.
+ This program is specifically targeting people with a "non standard" schedule (ie shifts), and people who have an ICS calendar containing **only** shifts/work events. This program is (currently) designed to trigger on every event it finds in the given ICS file. You can set it to ignore leave / vacation days, more details to come.
+ Tested with Tesla Model 3. I am unsure if the API calls are the same for other Tesla models and cannot test right now.


## Features
+ Automatically control your Tesla's HVAC & heated seats based on events in an online ICS file. If the online ICS file is auto-filled (by your employer for example), this program is 100% autonomous and your car should always be preheated (or cooled down) before you need to drive to and from work.
+ Car's HVAC system will turn on sooner if it's very cold outside, to give it more time to preheat, trigger time is dynamically adjusted based on car's external temperature sensor.
+ Slightly customisable, and highly customisable if you decide to tweak the source code based on your personal needs.


## Requirements (for usage)
Once again this is my first program and is probably useless for most people. You can most likely do what you want to do using TeslaFi/Home Assistant. This program requires:
+ An online calendar containing **only** events you want to trigger the car (shifts for work for example);
	+ There is a way to ignore events containig certain keywords (I use it to ignore sick leave days) but it's not super polished. I wouldn't advise feeding your entire calendar into this program unless you just use it to store events to which you must drive to.
+ A Tesla (model 3 tested, other models might or might not work);
+ A constant internet connection;
+ (Required) A Teslemetry account;
+ (Optional) A Slack account to receive push notifications to your phone;
+ Some patience and understanding to set up.



## Usage
1. Get a dynamic DNS setup (ie No-Ip)
2. Install Caddy
3. Setup Teslemtry account with webhooks into your Caddy port
4. Assuming you're running this on Linux like I am, download the binary to your Linux machine.
5. Download/open settings.example.json, and fill it up with your desired settings.
	+ I will add wiki pages with more details eventually, in the meantime look at the file *tesla_var.h* for details on settings.
	+ Rename the file to settings.json and place in the same folder as tcs binary file. The binary file needs to have a settings.json file in the same folder in order to run.
6. Download/open Caddyfile.example, fill in your domain name and ports.
	+ Caddy needs to be setup as a reverse proxy, put the same ports in Caddyfile as in settings.json (you can keep the default)
	+ Remove ".example" and leave Caddyfile wherever you want, as long as your Caddy server is using it as the config file.
7. Launch the binary, verify your settings have been applied and let it run 24/7 for it to control your vehicle.


## Building requirements
+ Requires [nlohmann's JSON for Modern C++](https://github.com/nlohmann/json)
+ Requires [libcurl](https://curl.haxx.se/libcurl/)