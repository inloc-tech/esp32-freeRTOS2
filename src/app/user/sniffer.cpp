
#include "sniffer.h"

SnifferS snifferS;

Sniffer::Sniffer(){

}

void Sniffer::core(String text, MqttCallback callback){

	DynamicJsonDocument doc(2048*4);  // 50kbytes
	
	#ifdef DEBUG_SNIFFER
		Serial.println(text);
	#endif
	
	if(text.length() ==  0)
		return;
	
    if (text.indexOf("PACKETS=") > -1) {
		text = text.substring(sizeof("PACKETS"));

		DeserializationError error = deserializeJson(doc, text);

		uint8_t clientId = 1;
		String topic = "/packet";

		if(snifferS.settings.sniffer_active){
			// Print the keys inside the JSON document
		    for (JsonPair kv : doc.as<JsonObject>()) {			
				callback(clientId,topic+"/"+String(kv.key().c_str()),kv.value().as<String>(),0,false);
				delay(10);
		    }
		}

	}else if (text.indexOf("NETWORK=") > -1) {
		text = text.substring(sizeof("NETWORK"));

		DeserializationError error = deserializeJson(doc, text);

		uint8_t clientId = 0;
		String topic = "/app/sniffer/network";

		for (JsonPair kv : doc.as<JsonObject>()) {
			String key = String(kv.key().c_str());
			String value = kv.value().as<String>();
			#ifdef DEBUG_SNIFFER
				Serial.println(key+":"+value);
			#endif
			if(key == "ssid"){
				if(value.length() <= 32){
					memset(snifferS.network.ssid,0,sizeof(snifferS.network.ssid));
					memcpy(snifferS.network.ssid,value.c_str(),value.length());
				}
				if(value != String(settings.wifi.ssid)){
					Serial1.println("ssid:"+String(settings.wifi.ssid));
				}
			}else if(key == "pwd"){
				if(value.length() < 32){
					memset(snifferS.network.pwd,0,sizeof(snifferS.network.pwd));
					memcpy(snifferS.network.pwd,value.c_str(),value.length());
				}
				if(value != String(settings.wifi.pwd))
					Serial1.println("password:"+String(settings.wifi.pwd));
			}else if(key == "channel"){
				// check if is number
				snifferS.network.channel = value.toInt();
			}else if(key == "nMessages"){
				// check if is number
				snifferS.network.nMessages = value.toInt();
			}
			//delay(10);
		}
		callback(clientId,topic,text,2,false);

	}else if (text.indexOf("SETTINGS=") > -1) {
		text = text.substring(sizeof("SETTINGS"));

		DeserializationError error = deserializeJson(doc, text);

		uint8_t clientId = 0;
		String topic = "/app/sniffer/settings";

		for (JsonPair kv : doc.as<JsonObject>()) {
			String key = String(kv.key().c_str());
			String value = kv.value().as<String>();
			#ifdef DEBUG_SNIFFER
				Serial.println(key+":"+value);
			#endif
			if(key == "version"){
				// check if is number
				if(value.length() <= 8){
					memset(snifferS.fw.version,0,sizeof(snifferS.fw.version));
					memcpy(snifferS.fw.version,value.c_str(),value.length());
				}
			}else if(key == "mac"){
				// check if is number
				if(value.length() <= 13){
					memset(snifferS.fw.uid,0,sizeof(snifferS.fw.uid));
					memcpy(snifferS.fw.uid,value.c_str(),value.length());
				}
			}else if(key == "sniffer_active"){
				// check if is number
				snifferS.settings.sniffer_active = value.toInt();
			}else if(key == "keepalive_period"){
				// check if is number
				snifferS.settings.keepalive_period = value.toInt();
			}else if(key == "sniffer_loop"){
				// check if is number
				snifferS.settings.packets_period = value.toInt();
			}else if(key == "log_level"){
				// check if is number
				snifferS.settings.log_level = value.toInt();
			}
			//delay(10);
		}
		callback(clientId,topic,text,2,false);

	}else if (text.indexOf("update=") > -1){
		uint8_t clientId = 0;
		text = text.substring(sizeof("update"));
		callback(clientId,"/app/sniffer/fota/update",text,2,false);
		delay(10);
	}

}