
#include "sniffer.h"

// limit to the maximum bytes that can be transmitted over wifi
uint8_t index_arr = 0;

Sniffer::Sniffer(){

}

void Sniffer::core(String text, MqttCallback callback){

	DynamicJsonDocument doc(2048*4);  // 50kbytes
	
	#ifdef DEBUG_SNIFFER
		Serial.println(text);
	#endif
	
	if(text.length() ==  0)
		return;
	
	int8_t index_i = text.indexOf("PACKETS=");

    if (text.indexOf("PACKETS=") > -1) {
		text = text.substring(sizeof("PACKETS"));

		DeserializationError error = deserializeJson(doc, text);

		uint8_t clientId = 1;
		String topic = "/packet";

		//if(settings.system.sniffing){
			// Print the keys inside the JSON document
		    for (JsonPair kv : doc.as<JsonObject>()) {			
				callback(clientId,topic+"/"+String(kv.key().c_str()),kv.value().as<String>(),0,false);
				delay(10);
		    }
		//}

	}
	else if (text.indexOf("NETWORK=") > -1) {
		text = text.substring(sizeof("NETWORK"));

		DeserializationError error = deserializeJson(doc, text);

		uint8_t clientId = 0;
		String topic = "/app/sniffer/network";

		for (JsonPair kv : doc.as<JsonObject>()) {
			String key = String(kv.key().c_str());
			String value = kv.value().as<String>();
			Serial.println(key+":"+value);
			if(key == "ssid" && value != String(settings.wifi.ssid)){
				Serial1.println("ssid:"+String(settings.wifi.ssid));
			}else if(key == "pwd" && value != String(settings.wifi.pwd)){
				Serial1.println("password:"+String(settings.wifi.pwd));
			}
			callback(clientId,topic+"/"+key,value,2,false);
			delay(10);
		}

	}
	else if (text.indexOf("SETTINGS=") > -1) {
		text = text.substring(sizeof("SETTINGS"));

		DeserializationError error = deserializeJson(doc, text);

		uint8_t clientId = 0;
		String topic = "/app/sniffer/settings";

		//if(settings.system.sniffing){
			// Print the keys inside the JSON document
		    for (JsonPair kv : doc.as<JsonObject>()) {
		    	Serial.println(String(kv.key().c_str())+":"+kv.value().as<String>());
				callback(clientId,topic+"/"+String(kv.key().c_str()),kv.value().as<String>(),2,false);
				delay(10);
		    }
		//}

	}else if (text.indexOf("update=") > -1){
		uint8_t clientId = 0;
		text = text.substring(sizeof("update"));
		callback(clientId,"/app/sniffer/fota/update",text,2,false);
		delay(10);
	}

}