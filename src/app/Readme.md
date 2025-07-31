# Readme App

## Description
Reads incoming messages from serial1 port and forwards through MQTT
If "ssid" or "password" mismatched current settings, sends a message through serial port to fix it
Reads incoming messages from MQTT and processes it.

## Serial
Available messages:
	PACKETS=<json>
	NETWORK=<json>
	SETTINGS=<json>
	update=<string>

Messages in json format are parse and forward through mqtt. Each key generates a new topic.

## MQTT

Listening app topics:
	:project/:uid/app/settings/reset/set"
	:project/:uid/app/sniffer/sniffer_uid/.."

Listening sniffer topics: 
(topics with route: :project/:uid/app/sniffer/sniffer_uid)
	/reboot/set"         
	/reset/set"
	/version/get
	/status/get
	/fota/update/set"
	/settings/set
	/settings/get    
	/settings/update    
	/settings/wifi/set"        
	/settings/wifi/get"        
	/settings/log/set"   
	/settings/log/get"   
	/settings/packets/set"       
	/settings/packets/get"       
	/serial/set"
	/serial/get"
Messages which includes "sniffer" in topic are dispatched through serial port

Messages received from serial port are parsed and stored or sent according to next topics:
	- "settings" - stored
	- "network" - stored
	- ":project/:uid/sniffer/fota" - sent
	- ":project/:uid/packet" - sent

