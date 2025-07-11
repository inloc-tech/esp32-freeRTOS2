#ifndef SNIFF_MESSAGE_H
#define SNIFF_MESSAGE_H


#include <Arduino.h>
#include <ArduinoJson.h>
#include <TimeLib.h>
#include "./src/settings/settings.h"
#include "./app_package.h"

#define DOC_SIZE         3000
#define RSSI_ARRAY_SIZE  50

// Define the function signature for the callback
typedef bool (*MqttCallback)(uint8_t clientID, String topic, String data, uint8_t qos, bool retain);

struct SnifferS {

	struct fw {
		char         version[8];
		char         md5[16];
	}fw;

	struct network { // on settings load
		char         ssid[32];
		char         pwd[32];
		uint8_t 	 channel;
		uint32_t	 nMessages;
	} network;

	struct settings { // on settings load
		bool         sniffer_active;
		uint16_t     keepalive_period;
		uint16_t     packets_period;
	} settings;
};

extern SnifferS snifferS;

class Sniffer
{
  public:
    Sniffer();
    void core(String msg, MqttCallback callback);

  private:
};

#endif
