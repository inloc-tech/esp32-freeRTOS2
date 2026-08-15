# Readme App

## Description
Reads incoming messages from serial1 port and forwards through MQTT
If "ssid" or "password" mismatched current settings, sends a message through serial port to fix it
Reads incoming messages from MQTT and processes it.

## Serial
Available messages:
	PACKETS=<json>
	WIFI=<json>
	FIRMWARE=<json>
	NETWORK=<json>
	SETTINGS=<json>
	update=<string>

Serial messages are parsed as structured payloads and forwarded through MQTT.

Current serial schemas:
	- PACKETS: packet fields are forwarded one by one without changing the existing packet behavior
	- WIFI: {"ssid":"...","pwd":"...","channel":"..."}
	- FIRMWARE: {"version":"...","model":"...","variant":"..."}
	- NETWORK: {"bssid":"...","nMessages":"..."}
	- SETTINGS: {"keepalive_period":"...","sniffer_active":"...","sniffer_loop":"...","log_level":"..."}
	- update: update=<string>

The NETWORK bssid is normalized and used as the sniffer UID in MQTT topics.

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
	- /app/sniffer/:uid/settings/network - network identity data
	- /app/sniffer/:uid/settings/wifi - wifi credentials
	- /app/sniffer/:uid/settings/firmware - firmware identity
	- /app/sniffer/:uid/settings/log - keepalive/log settings
	- /app/sniffer/:uid/settings/sniffer - sniffer_active and packets_period
	- /app/sniffer/:uid/fota/update - firmware update requests
	- /packets/:channel - packet fields forwarded as separate MQTT topics

