
#include "app.h"
#include <ArduinoJson.h>
#include "sniffer.h"

extern CALLS call;
extern SENSORS sensors;
extern SYSFILE sysfile;

/* user calls below */
Sniffer sniffer;

app_settings app_s = {
  .fw = {
    /* version */   APP_VERSION,
    /* md5 */       ""
  },
  // user settings
  .sniffer = {
    /* enabled */   false,
    /* channel */   0,
    /* loop */      2000,
  }
};
/*
bool mqttSend(uint8_t clientID, String topic, String data, uint8_t qos, bool retain){
  return call.mqtt_send(clientID,topic,data,qos,retain);
}
*/
void APP::init(){

  Serial.println("Init app" + String(FW_MODEL) +" module");

  load_settings();
  log_settings();

  /* user code below */
}

void APP::loop(){

  if(timeoutInfo < millis()){
    Serial.println("app is running");
    timeoutInfo += 5000;
  }

  /* user code below */
  if(Serial1.available()){
    msg += Serial1.readStringUntil('\n');
    #ifdef DEBUG_SNIFFER
    Serial.println(msg+'\n');
    #endif
    sniffer.core(msg, core_send_mqtt_message);
    msg = "";
  }
/*
  if(timeoutSniffer <= millis()){
    timeoutSniffer = millis() + app_s.sniffer.loop;
    sniffer.core(msg, core_send_mqtt_message);
    msg = "";
  }
*/
}

/*
* Function called from core.cpp on new message arrived
* if topic ends with '/get', it should be removed from topic avoiding loops
* if topic ends with '/set', an unpublish should be sent
*/
void APP::parse_mqtt_messages(uint8_t clientID, String topic, String payload){

  Serial.println("app topic: "+topic);
  Serial.println("payload: "+payload);
  String subtopic = "";
  bool set = false;
  bool get = false;
  bool store = false;

  if(topic.endsWith("/set")){
    set = true;
    if(payload == "")
      return;
  }

  if(topic.endsWith("/get")){
    get = true;
    uint16_t index = topic.lastIndexOf("/");
    subtopic = topic.substring(0,index); // filter get
  }

  switch(resolveOption(appTopics,topic)){
    case settings_reset_:
      reset_settings();
      break;
    case sniffer_reboot_:
      Serial1.println("reboot:1");
      break;
    case sniffer_reset_:
      Serial.println("Not implemented !!");
      break;
    case sniffer_fota_update_:
      {  
        DeserializationError error = deserializeJson(doc, payload);
        if(error){
          Serial.println("Not Json");
          return;
        }

        if(doc.containsKey("url")){
          String url = doc["url"];
          Serial1.println("update:"+url);
        }
      }
      break;
    case sniffer_network_:
      {
        DeserializationError error = deserializeJson(doc, payload);
        if(error){
          Serial.println("Not Json");
          return;
        }

        if(doc.containsKey("ssid")){
          #ifndef UNITTEST
            String ssid = doc["ssid"];
          #else
            String ssid = "";
            if(doc["ssid"].is_string())
              ssid = doc["ssid"];
          #endif
          Serial1.println("ssid:"+ssid);
          delay(100);
        }

        if(doc.containsKey("pwd")){
          #ifndef UNITTEST
            String pwd = doc["pwd"];
          #else
            String pwd = "";
            if(doc["pwd"].is_string())
              pwd = doc["pwd"];
          #endif
          Serial1.println("password:"+pwd);
          delay(100);
        }

        if(doc.containsKey("channel")){
          #ifndef UNITTEST
            String channel = doc["channel"];
          #else
            String channel = "";
            if(doc["channel"].is_string())
              channel = doc["channel"];
          #endif
          Serial1.println("channel:"+channel);
          delay(100);
        }
      }
      break;
    case sniffer_network_get_:
      {
        String ssid = String(snifferS.network.ssid);
        String pwd = String(snifferS.network.pwd);
        String channel = String(snifferS.network.channel);
        String nMessages = String(snifferS.network.nMessages);
        String payload = "{\"ssid\":\""+ssid+"\",\"pwd\":\""+pwd+"\",\"channel\":\""+String(channel)+"\",\"nMessages\":\""+String(nMessages)+"\"}";
        core_send_mqtt_message(clientID,subtopic,payload,2,false);
      }
    case sniffer_log_:
      Serial.println("Not implemented !!");
      break;
    case sniffer_settings_:
      {
        DeserializationError error = deserializeJson(doc, payload);
        if(error){
          Serial.println("Not Json");
          return;
        }

        if(doc.containsKey("sniffer_active")){
          #ifndef UNITTEST
            String sniffer_active = doc["sniffer_active"];
          #else
            String sniffer_active = "";
            if(doc["sniffer_active"].is_string())
              sniffer_active = doc["sniffer_active"];
          #endif
          Serial1.println("active:"+sniffer_active);
          delay(100);
        }

        if(doc.containsKey("keepalive_period")){
          #ifndef UNITTEST
            String keepalive_period = doc["keepalive_period"];
          #else
            String keepalive_period = "";
            if(doc["keepalive_period"].is_string())
              keepalive_period = doc["keepalive_period"];
          #endif
          Serial1.println("keepalive_period:"+keepalive_period);
          delay(100);
        }

        if(doc.containsKey("packets_period")){
          #ifndef UNITTEST
            String packets_period = doc["packets_period"];
          #else
            String packets_period = "";
            if(doc["packets_period"].is_string())
              packets_period = doc["packets_period"];
          #endif
          Serial1.println("packets_period:"+packets_period);
          delay(100);
        }
      }
      break;
    case sniffer_settings_get_:
      {
        String version = String(snifferS.fw.version);
        String md5 = String(snifferS.fw.md5);
        String sniffer_active = String(snifferS.settings.sniffer_active);
        String keepalive_period = String(snifferS.settings.keepalive_period);
        String packets_period = String(snifferS.settings.packets_period);
        String payload = "{\"version\":\""+version+"\",\"version\":\""+md5+"\",\"sniffer_active\":\""+String(sniffer_active)+"\",\"keepalive_period\":\""+String(keepalive_period)+"\",\"packets_period\":\""+String(packets_period)+"\"}";
        core_send_mqtt_message(clientID,subtopic,payload,2,false);
      }
    case sniffer_serial_:
      Serial.println("Not implemented !!");
      break;
  }

  if(store){
    store_settings();
    log_settings();
  }

  if(set)
    core_send_mqtt_message(clientID,topic,"",2,true); // unpublish

}


appTopics_ APP::resolveOption(std::map<long, appTopics_> map, String topic) {

  std::string topic_ = std::string(topic.c_str());
  long str_hash = (long)std::hash<std::string>{}(topic_);
  std::map<long,appTopics_>::iterator it;

  it = map.find(str_hash);
  if(it != map.end())
    return it->second;

  return app_not_found;
}

bool APP::load_settings(){
  uint16_t len = sizeof(app_s);
  Serial.printf("len:%d\n",len);
  Serial.printf("filename %s \n",APP_SETTINGS_FILENAME);
  char data[1000];
  call.read_file(APP_SETTINGS_FILENAME,data,&len);
  memcpy(app_s.fw.version,data,sizeof(app_s.fw.version));
  String oldVersion = String(app_s.fw.version);
  String currentVersion = String(APP_VERSION);
  Serial.println("prev app.version: "+oldVersion);
  Serial.println("current app.version: "+currentVersion);
  if(currentVersion.startsWith("0.") || currentVersion.startsWith("1.") || currentVersion.startsWith("2.")){
    memcpy(app_s.fw.version,data,sizeof(app_s));
    memset(app_s.fw.version,0,sizeof(app_s.fw.version));
    memcpy(app_s.fw.version,currentVersion.c_str(),currentVersion.length());
  }else{
    Serial.println("resetting app settings..");
    memset(app_s.fw.version,0,sizeof(app_s.fw.version));
    memcpy(app_s.fw.version,currentVersion.c_str(),currentVersion.length());
  }
  store_settings();
  return true;
}


bool APP::store_settings(){

  Serial.printf("filename %s \n",APP_SETTINGS_FILENAME);
  if(!call.write_file(APP_SETTINGS_FILENAME,app_s.fw.version,sizeof(app_s))){
    Serial.println("failing writing file: "+String(APP_SETTINGS_FILENAME));
    return false;
  }
  return true;
}

bool APP::reset_settings(){
  memset(app_s.fw.version,0,sizeof(app_s.fw.version));
  if(call.write_file(APP_SETTINGS_FILENAME,app_s.fw.version,sizeof(app_s)))
    call.fw_reboot();
}

void APP::log_settings(){

  Serial.println("app.md5: "+String(app_s.fw.md5));

}
