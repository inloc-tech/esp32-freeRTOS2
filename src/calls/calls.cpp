
#include "calls.h"

/*
* !! All calls to sysfile must be done here
*/

extern MODEMfreeRTOS mRTOS;
extern SYSFILE sysfile;
extern SemaphoreHandle_t spiffsMutex;

HTTP_HEADER_MSG* msg_header;
HTTP_BODY_MSG* msg_body;

FOTA fota;

bool CALLS::fw_fota(String url){

  String protocol = "";
  if(url.startsWith("HTTPS://") || url.startsWith("https://")){
    protocol = "HTTPS";
    url = url.substring(8);
  }else if(url.startsWith("HTTP://") || url.startsWith("http://")){
    protocol = "HTTP";
    url = url.substring(7);
  }else{
    Serial.println("Invalid URL");
    return false;
  }

  int16_t index = url.indexOf("/");
  String host = url.substring(0,index);
  String path = url.substring(index);
  String method = "GET";
  String header_key = "";
  String header_value = "";
  String body = "";
  bool json = false;

  do_fota(protocol,host,path,method,header_key,header_value,body,json);
}

/*
* !! check if md5 is being compared
*/
bool CALLS::fw_settings_update(String url, String filename){

  String protocol = "";
  if(url.startsWith("HTTPS://") || url.startsWith("https://")){
    protocol = "HTTPS";
    url = url.substring(8);
  }else if(url.startsWith("HTTP://") || url.startsWith("http://")){
    protocol = "HTTP";
    url = url.substring(7);
  }else{
    Serial.println("Invalid URL");
    return false;
  }

  int16_t index = url.indexOf("/");
  String host = url.substring(0,index);
  String path = url.substring(index);
  String method = "GET";
  String header_key = "";
  String header_value = "";
  String body = "";
  bool json = false;
  String json_str = do_request(protocol,host,path,method,header_key,header_value,body,json);

  Serial.println(json_str);

  if(!xSemaphoreTake( spiffsMutex, 5000)){
    //xSemaphoreGive(spiffsMutex);
    return false;
  }

  sysfile.write_file(filename.c_str(),json_str.c_str(),json_str.length());

  xSemaphoreGive(spiffsMutex);
}

/*
* load settings
*/
bool CALLS::fw_settings_load(String filename, String version){

  if(!xSemaphoreTake( spiffsMutex, 5000)){
    //xSemaphoreGive(spiffsMutex);
    return false;
  }

  // read settings
  uint16_t len = sizeof(settings);
  char* data = (char*)malloc(len);
  if(data != nullptr){
    sysfile.read_file(filename.c_str(),data,&len);
    memcpy(settings.fw.version,data,sizeof(settings.fw.version));
    if(String(settings.fw.version) == version)
      memcpy(settings.fw.version,data,sizeof(settings));
    else{
      memset(settings.fw.version,0,sizeof(settings.fw.version));
      memcpy(settings.fw.version,version.c_str(),version.length());
      sysfile.write_file(filename.c_str(),settings.fw.version,sizeof(settings));
    }
    free(data);
  }

  xSemaphoreGive(spiffsMutex);

  return true;
}

/*
* read file
*/
bool CALLS::read_file(String filename, char* data, uint16_t* len){

  if(!xSemaphoreTake( spiffsMutex, 100)){
     //xSemaphoreGive(spiffsMutex);
     return false;
  }

  bool res = sysfile.read_file(filename.c_str(),data,len);

  xSemaphoreGive(spiffsMutex);

  return res;
}

/*
* clean records
*/
bool CALLS::delete_file(String filename){

  if(!xSemaphoreTake( spiffsMutex, 2000)){
     //xSemaphoreGive(spiffsMutex);
     return false;
  }

  bool res = sysfile.delete_file(filename.c_str());

  xSemaphoreGive(spiffsMutex);

  return res;
}

/*
* init directories
*/
bool CALLS::init_filesystem(String directory[], uint8_t len){

  if(!xSemaphoreTake( spiffsMutex, 2000)){
     xSemaphoreGive(spiffsMutex);
     return false;
  }

  // check filesystem
  uint8_t i = 0;
  Serial.printf("dir size: %d \n",len);
  for(uint8_t i=0; i < len; i++){
    if(!sysfile.create_dir(directory[i].c_str()))
      DBGLOG(Error,"-- create dir: "+ directory[i] +" has FAILED --");
  }

  // list filesystem
  sysfile.list_filesystem(5);

  xSemaphoreGive(spiffsMutex);

  return true;
}

/*
* init directories
*/
bool CALLS::create_dir(String directory){

  if(!xSemaphoreTake( spiffsMutex, 5000)){
     xSemaphoreGive(spiffsMutex);
     return false;
  }

  bool res = sysfile.create_dir(directory.c_str());

  xSemaphoreGive(spiffsMutex);

  if(!res)
    DBGLOG(Error,"-- create dir: "+ directory +" has FAILED --");

  return res;
}

/*
* Iterate over files, call callback after each iteration
*/
bool CALLS::check_filesystem_records(String dir, uint32_t timeout, bool (*callback)(String)){


  if(!xSemaphoreTake( spiffsMutex, 100)){
    //xSemaphoreGive(spiffsMutex);
    Serial.println("couldn't get semaphore");
    return false;
  }

  sysfile.iterateDir(dir.c_str(),timeout,callback);

  xSemaphoreGive( spiffsMutex );

  return true;
}

bool CALLS::store_record(String filename, const char* data, uint16_t len){

  if(!xSemaphoreTake( spiffsMutex, 5000)){
    xSemaphoreGive(spiffsMutex);
    return false;
  }

  sysfile.write_file(filename.c_str(),data,len);

  xSemaphoreGive( spiffsMutex );

  return true;

}

bool CALLS::remove_dir(String dir){

  if(!xSemaphoreTake( spiffsMutex, 100)){
    //xSemaphoreGive(spiffsMutex);
    return false;
  }

  bool res = sysfile.delete_dir(dir.c_str(),2000);

  xSemaphoreGive( spiffsMutex );

  return res;
}

void CALLS::clean_dir(String dir){

  if(!xSemaphoreTake( spiffsMutex, 100)){
    //xSemaphoreGive(spiffsMutex);
    return;
  }
  sysfile.deleteEmptySubDirectories(LITTLEFS,dir.c_str(),2000);

  xSemaphoreGive( spiffsMutex );
}

bool CALLS::fw_reboot(){
  ESP.restart();
}

bool CALLS::fw_reset(){

  if(!xSemaphoreTake( spiffsMutex, 5000)){
    xSemaphoreGive(spiffsMutex);
    return false;
  }

  sysfile.format();

  xSemaphoreGive( spiffsMutex );

  return true;
}

void CALLS::check_alarms(){

}

void CALLS::check_sensors(){

}

String CALLS::do_request(String protocol, String host, String path, String method, String header_key, String header_value, String body, bool json){

  if(protocol == "HTTPS")
    mRTOS.https_pushMessage(CONTEXTID,CLIENTID,SSLCLIENTID,host,path,method,header_key,header_value,body,json);
  else if(protocol == "HTTP")
    mRTOS.http_pushMessage(CONTEXTID,CLIENTID,host,path,method);
  else return "";

  uint32_t timeout = millis() + 60000; // 30 seconds timeout
  while(timeout > millis()){
    msg_header = mRTOS.http_header_getNextMessage(msg_header);
    if(msg_header != NULL){
      Serial.printf("client [%d] %s \n",msg_header->clientID,msg_header->http_response.c_str());
      if(msg_header->http_response.indexOf("200") > 0){
        Serial.printf("http body len %d \n",msg_header->body_len);
        uint32_t len = 0;
        char* data = (char*)malloc(msg_header->body_len);
        while(msg_header->body_len != len  && timeout > millis()){
          msg_body = mRTOS.http_body_getNextMessage(msg_body);

          if(msg_body != NULL){

            if(data != nullptr){
              for(uint16_t i=0;i<msg_body->data_len;i++){
                data[len+i] = msg_body->data[i];
              }
            }

            len += msg_body->data_len;
            Serial.printf("http total bytes read of body data: %d \n",len);

          }
          delay(100); // use delay to moderate concurrency access to queues
        }
        Serial.println("http all data was read");

        String body = "";
        for(uint16_t i=0;i<len;i++){
          //Serial.print(data[i]);
          body += data[i];
        }

        //String body = String(data);
        free(data);
        return body;
      }
    }
    delay(100); // use delay to moderate concurrency access to queues
  }
  return "";
}

bool CALLS::do_fota(String protocol, String host, String path, String method, String header_key, String header_value, String body, bool json){

  if(protocol == "HTTPS")
    mRTOS.https_pushMessage(CONTEXTID,CLIENTID,SSLCLIENTID,host,path,method,header_key,header_value,body,json);
  else if(protocol == "HTTP")
    mRTOS.http_pushMessage(CONTEXTID,CLIENTID,host,path,method);
  else return "";

  uint32_t timeout = millis() + 60000; // 30 seconds timeout
  uint32_t fota_size;
  while(timeout > millis()){
    msg_header = mRTOS.http_header_getNextMessage(msg_header);
    if(msg_header != NULL){
      Serial.printf("client [%d] %s \n",msg_header->clientID,msg_header->http_response.c_str());
      if(msg_header->http_response.indexOf("200") > 0){
        Serial.printf("http body len %d \n",msg_header->body_len);
        fota.start(msg_header->body_len);
        fota_size = msg_header->body_len;
        uint32_t len = 0;

        while(fota_size != len && timeout > millis()){
          msg_body = mRTOS.http_body_getNextMessage(msg_body);

          if(msg_body != NULL){
            timeout = millis()+10000;
            //md5_.add((uint8_t*)body_rx->data,body_rx->len);
            int8_t tries = 3;
            while(!fota.write_block((uint8_t*)msg_body->data,msg_body->data_len)){
              delay(300);
              if(tries == 0)
                return false;
              tries--;
            }
            Serial.println("heap free: " + String(ESP.getFreeHeap() / 1024) + " KiB");
            len += msg_body->data_len;
            Serial.printf("http total bytes read of body data: %d \n",len);
          }
          delay(100); // use delay to moderate concurrency access to queues
        }
        Serial.println("http all data was read");
        /*
        md5_.calculate();
        String md5_calculated = md5_.toString();
        log("md5 calculated: "+md5_calculated);
        log("md5 header: "+md5_http);
        */
        if(fota.has_finished()){
          Serial.println("new fw uploaded");
          Serial.println("rebooting");
          fw_reboot();
        }

        return true;
      }
    }
    delay(100); // use delay to moderate concurrency access to queues
  }
  return false;
}
