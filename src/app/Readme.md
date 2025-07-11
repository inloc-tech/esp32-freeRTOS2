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

Listening topics:
	:project/:uid/app/settings/reset/set"         
	:project/:uid/app/sniffer/reboot/set"         
	:project/:uid/app/sniffer/reset/set"          
	:project/:uid/app/sniffer/fota/update/set"    
	:project/:uid/app/sniffer/network/set"        
	:project/:uid/app/sniffer/network/get"        
	:project/:uid/app/sniffer/settings/log/set"   
	:project/:uid/app/sniffer/settings/log/get"   
	:project/:uid/app/sniffer/settings/set"       
	:project/:uid/app/sniffer/settings/get"       
	:project/:uid/app/sniffer/settings/serial/set"
	:project/:uid/app/sniffer/settings/serial/get"
Messages which includes "sniffer" in topic are dispatched through serial port

Messages received from serial port are parsed and stored or sent according to next topics:
	- "settings" - stored
	- "network" - stored
	- ":project/:uid/sniffer/fota" - sent
	- ":project/:uid/packet" - sent

