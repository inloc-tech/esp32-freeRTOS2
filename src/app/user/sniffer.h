#ifndef SNIFF_MESSAGE_H
#define SNIFF_MESSAGE_H


#include <Arduino.h>
#include <map>
#include <ArduinoJson.h>

#include <TimeLib.h>
#include "./src/settings/settings.h"
#include "./app_package.h"
#include "./../../../core.h"

#define DOC_SIZE         3000
#define RSSI_ARRAY_SIZE  50

// Define the function signature for the callback
typedef bool (*MqttCallback)(uint8_t clientID, String topic, String data, uint8_t qos, bool retain);

struct SnifferS {

	struct fw {
		char         version[8];
		char         md5[16];
		char         uid[13];
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
		uint16_t     log_level;
	} settings;
};

extern SnifferS snifferS;


enum snifferTopics_ {
  sniffer_reboot_,
  sniffer_reset_,
  sniffer_version_get_,
  sniffer_status_get_,
  sniffer_fota_update_,
  sniffer_settings_,
  sniffer_settings_get_,
  sniffer_settings_update_,
  sniffer_settings_wifi_,
  sniffer_settings_wifi_get_,
  sniffer_settings_log_,
  sniffer_settings_log_get_,
  sniffer_settings_packets_,
  sniffer_settings_packets_get_,
  sniffer_serial_,
  sniffer_serial_get_,
  sniffer_not_found
};


static const std::map<long, snifferTopics_> snifferTopics {
  { (long)std::hash<std::string>{}("/reboot/set"),                       sniffer_reboot_ },
  { (long)std::hash<std::string>{}("/reset/set"),                        sniffer_reset_ },
  { (long)std::hash<std::string>{}("/version/get"),                      sniffer_version_get_ },
  { (long)std::hash<std::string>{}("/status/get"),                       sniffer_status_get_ },
  { (long)std::hash<std::string>{}("/fota/update/set"),                  sniffer_fota_update_ },
  { (long)std::hash<std::string>{}("/settings/set"),                     sniffer_settings_ },
  { (long)std::hash<std::string>{}("/settings/get"),                     sniffer_settings_get_ },
  { (long)std::hash<std::string>{}("/settings/update"),                  sniffer_settings_update_ },
  { (long)std::hash<std::string>{}("/settings/wifi/set"),                sniffer_settings_wifi_ },
  { (long)std::hash<std::string>{}("/settings/wifi/get"),                sniffer_settings_wifi_get_ },
  { (long)std::hash<std::string>{}("/settings/log/set"),                 sniffer_settings_log_ },
  { (long)std::hash<std::string>{}("/settings/log/get"),                 sniffer_settings_log_get_ },
  { (long)std::hash<std::string>{}("/settings/packets/set"),             sniffer_settings_packets_ },
  { (long)std::hash<std::string>{}("/settings/packets/get"),             sniffer_settings_packets_get_ },
  { (long)std::hash<std::string>{}("/serial/set"),                       sniffer_serial_ },
  { (long)std::hash<std::string>{}("/serial/get"),                       sniffer_serial_get_ },
};


class Sniffer
{
  public:
  	bool fUpdateVersion = true;

    Sniffer();
    void core(String msg, MqttCallback callback);
	void parse_mqtt_messages(uint8_t clientID, String topic, String payload, MqttCallback callback);

  private:

    snifferTopics_ resolveOption(std::map<long, snifferTopics_> map, String topic);
};

#endif
