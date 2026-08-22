# Changelog

## v1.1.0 - Released
	fix: recover uid, splits firmware data, fixes nMessages and logs_level, adds variant and model
	perf: remove fUpdateVersion logic
	ci: set board_fqbn for esp32 board

## v1.0.10 - pre-release (bugs found)
	refactor sniffer serial messages
	refactor sniffer mqtt messages
	compatible with v2.0.4 sniffer version

## v1.0.9 - Released
	wifi ssid change to Inloc

## v1.0.8 - Released
	fix: packets: if mqtt2 is not active sends to mqtt1 (default connection)
	
## v1.0.7 - Trial
	packet topic change:
 		- send packets to topic: "/packets/:channel"

## v1.0.6 - Released
	app: mqtt: (uid)
		- Adds sniffer uid after "app/sniffer/" topic
		- Adds sniffer route

## v1.0.5 - Released
	Refactoring mqtt topics for app
		- stores all new configurations in a struct
	Storing info from sniffer
		- Sending packet coming from sniffer as a JSON

## v1.0.4 - Released
	Adds "app" to topic 
	
## v1.0.3 - Released
	sends sniffed packets for 2nd mqtt connection

## v1.0.2 - Released
	app user dynamic memory:
		- Increases DynamicJsonDocument for sniffer
		- Increases MQTT buffer size
	Disables Debug Sniffer
	changes packet qos level to 0 - test it..

## v1.0.1 - Released
	First functionable app.
	Supports: 
		- settings and network configuration forward.
		- info and fota request forward
		- packets, settings and network parser
		- Sync ssid and pwd from attached uController to configs on current uController

## v1.0.0
	Default version - does nothing