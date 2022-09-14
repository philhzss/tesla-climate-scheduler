import teslapy
import json

settings = None # Global use

with open('settings.json') as settings_file:
    settings = json.load(settings_file)

refreshToken = settings["Tesla account settings"]["teslaRefreshToken"]
teslaEmail = settings["Tesla account settings"]["teslaEmail"]

with teslapy.Tesla(teslaEmail) as tesla:
    if not tesla.authorized:
        tesla.refresh_token(refresh_token=refreshToken)
    vehicles = tesla.vehicle_list()
    #print(vehicles[0])

with open('cache.json') as cache_file:
    cache = json.load(cache_file)

accessToken = cache[teslaEmail]["sso"]["access_token"]
settings["Tesla account settings"]["teslaAccessToken"] = accessToken

with open("settings.json", "w") as jsonFile:
    json.dump(settings, jsonFile, indent=4)