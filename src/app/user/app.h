#ifndef APP_H
#define APP_H

#include "Arduino.h"
#include <map>

#include "../../../package.h"
#include "sysfile.hpp"
#include "app_package.h"
#include "credentials.h"
#include "./../../../core.h"

struct app_settings {

  struct fw {
    char         version[8];
    char         md5[16];
  }fw;

  // user settings below
  struct sniffer {
    bool          enabled;
    uint8_t       channel;
    uint16_t      loop;
  }sniffer;

};

// user can edit it
enum appTopics_ {
  settings_reset_,
  sniffer_reboot_,
  sniffer_reset_,
  sniffer_fota_update_,
  sniffer_network_,
  sniffer_network_get_,
  sniffer_log_,
  sniffer_log_get_,
  sniffer_settings_,
  sniffer_settings_get_,
  sniffer_serial_,
  sniffer_serial_get_,
  app_not_found
};


// user can edit it
static const std::map<long, appTopics_> appTopics {
  { (long)std::hash<std::string>{}("/app/settings/reset/set"),                       settings_reset_},
  { (long)std::hash<std::string>{}("/app/sniffer/reboot/set"),                       sniffer_reboot_ },
  { (long)std::hash<std::string>{}("/app/sniffer/reset/set"),                        sniffer_reset_ },
  { (long)std::hash<std::string>{}("/app/sniffer/fota/update/set"),                  sniffer_fota_update_ },
  { (long)std::hash<std::string>{}("/app/sniffer/network/set"),                      sniffer_network_ },
  { (long)std::hash<std::string>{}("/app/sniffer/network/get"),                      sniffer_network_get_ },
  { (long)std::hash<std::string>{}("/app/sniffer/settings/log/set"),                 sniffer_log_ },
  { (long)std::hash<std::string>{}("/app/sniffer/settings/log/get"),                 sniffer_log_get_ },
  { (long)std::hash<std::string>{}("/app/sniffer/settings/set"),                     sniffer_settings_ },
  { (long)std::hash<std::string>{}("/app/sniffer/settings/get"),                     sniffer_settings_get_ },
  { (long)std::hash<std::string>{}("/app/sniffer/settings/serial/set"),              sniffer_serial_ },
  { (long)std::hash<std::string>{}("/app/sniffer/settings/serial/get"),              sniffer_serial_get_ },
};

class APP{

  public:
    APP(){};

    /* do not delete it the following functions */
    void init();
    void loop();
    void parse_mqtt_messages(uint8_t clientID, String topic, String payload);
    bool getValue(JsonObject& obj, String ref){return false;};

    // user public funcs
    
    // user public vars

  private:
    // do not delete the following function
    appTopics_ resolveOption(std::map<long, appTopics_> map, String topic);
    bool load_settings();
    bool store_settings();
    bool reset_settings();
    void log_settings();

    uint32_t timeoutInfo;

    // user private funcs

    // user private vars
    uint32_t timeoutSniffer;
    String msg;
};

#endif
