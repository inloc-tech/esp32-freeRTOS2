#ifndef SNIFF_MESSAGE_H
#define SNIFF_MESSAGE_H


#include <Arduino.h>
#include <ArduinoJson.h>
#include <TimeLib.h>

#define DOC_SIZE         3000
#define RSSI_ARRAY_SIZE  50

// Define the function signature for the callback
typedef bool (*MqttCallback)(uint8_t clientID, String topic, String data, uint8_t qos, bool retain);

class Sniffer
{
  public:
    Sniffer();
    void core(String msg, MqttCallback callback);
  private:
};





#endif
