
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

  LOG_INFO("Init app %s module\n", FW_MODEL);

  load_settings();
  log_settings();

  /* user code below */
}

void APP::loop(){

  if(timeoutInfo < millis()){
    LOG_DEBUG("app is running\n");
    timeoutInfo += 5000;
  }

  /* user code below */
  if(Serial1.available()){
    msg += Serial1.readStringUntil('\n');
    #ifdef DEBUG_SNIFFER
    LOG_DEBUG("%s\n", msg.c_str());
    #endif
    sniffer.core(msg, core_send_mqtt_message);
    msg = "";
  }

}

/*
* Function called from core.cpp on new message arrived
* if topic ends with '/get', it should be removed from topic avoiding loops
* if topic ends with '/set', an unpublish should be sent
*/
void APP::parse_mqtt_messages(uint8_t clientID, String topic, String payload){

  LOG_DEBUG("app topic: %s\n", topic.c_str());
  LOG_DEBUG("payload: %s\n", payload.c_str());
  String subtopic = "";
  bool set = false;
  bool get = false;
  bool store = false;

  /*
  * filter app/:model/:sniffer_uid - on this system there's only sniffer connected.
  * No needed to check and choose route based on uid
  */

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
    case sniffer_route_:
      {
        sniffer.parse_mqtt_messages(clientID,topic,payload,core_send_mqtt_message);
        break;
      }
    case app_not_found:
      LOG_WARN("app topic not found\n");
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

  if( topic.indexOf("/app/sniffer") > -1)
    return sniffer_route_;

  return app_not_found;
}

bool APP::load_settings(){
  uint16_t len = sizeof(app_s);
  LOG_DEBUG("len:%d\n",len);
  LOG_DEBUG("filename %s \n",APP_SETTINGS_FILENAME);
  char data[1000];
  call.read_file(APP_SETTINGS_FILENAME,data,&len);
  memcpy(app_s.fw.version,data,sizeof(app_s.fw.version));
  String oldVersion = String(app_s.fw.version);
  String currentVersion = String(APP_VERSION);
  LOG_INFO("prev app.version: %s\n", oldVersion.c_str());
  LOG_INFO("current app.version: %s\n", currentVersion.c_str());
  if(currentVersion.startsWith("0.") || currentVersion.startsWith("1.") || currentVersion.startsWith("2.")){
    memcpy(app_s.fw.version,data,sizeof(app_s));
    memset(app_s.fw.version,0,sizeof(app_s.fw.version));
    memcpy(app_s.fw.version,currentVersion.c_str(),currentVersion.length());
  }else{
    LOG_INFO("resetting app settings..\n");
    memset(app_s.fw.version,0,sizeof(app_s.fw.version));
    memcpy(app_s.fw.version,currentVersion.c_str(),currentVersion.length());
  }
  store_settings();
  return true;
}

bool APP::store_settings(){

  LOG_DEBUG("filename %s \n",APP_SETTINGS_FILENAME);
  if(!call.write_file(APP_SETTINGS_FILENAME,app_s.fw.version,sizeof(app_s))){
    LOG_ERROR("failing writing file: %s\n", APP_SETTINGS_FILENAME);
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

  LOG_DEBUG("app.md5: %s\n", app_s.fw.md5);

}
