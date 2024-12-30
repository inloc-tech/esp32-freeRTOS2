
#include "sniffer.h"

// limit to the maximum bytes that can be transmitted over wifi
uint8_t index_arr = 0;

Sniffer::Sniffer(){

}

void Sniffer::core(String text, MqttCallback callback){

	DynamicJsonDocument doc(2048);  // 50kbytes
	
	#ifdef DEBUG_SNIFFER
		Serial.println(text);
	#endif
	
	if(text.length() ==  0)
		return;
	
	int8_t index_i = text.indexOf("PACKETS=");

    if (index_i > -1) {
		text = text.substring(index_i+8);

		DeserializationError error = deserializeJson(doc, text);

		uint8_t clientId = 0;
		String topic = "/packet";

		//if(settings.system.sniffing){
			// Print the keys inside the JSON document
		    for (JsonPair kv : doc.as<JsonObject>()) {			
				callback(clientId,topic+"/"+String(kv.key().c_str()),kv.value().as<String>(),2,false);
				delay(10);
		    }
		//}

	}

}