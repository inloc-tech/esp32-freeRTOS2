
#include "sniffer.h"

SnifferS snifferS;

Sniffer::Sniffer(){

}

void Sniffer::core(String text, MqttCallback callback){

	DynamicJsonDocument doc(2048*4);  // 50kbytes
	
	#ifdef DEBUG_SNIFFER
		LOG_DEBUG("%s\n", text.c_str());
	#endif
	
	if(text.length() ==  0)
		return;
	
	if (text.indexOf("PACKETS=") > -1) {
		text = text.substring(sizeof("PACKETS"));

		DeserializationError error = deserializeJson(doc, text);
		if(error)
			return;

		uint8_t clientId = 0;
		if(settings.mqtt2.active == 1)
			clientId = 1;

		String channel = String(snifferS.network.channel);
		if(channel == "")
			return;

		String topic = "/packets/"+channel;

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
		if(error)
			return;

		for (JsonPair kv : doc.as<JsonObject>()) {
			String key = String(kv.key().c_str());
			String value = kv.value().as<String>();
			#ifdef DEBUG_SNIFFER
				LOG_DEBUG("%s:%s\n", key.c_str(), value.c_str());
			#endif
			if(key == "bssid"){
				value.replace(":", "");
				value.toLowerCase();
				if(value.length() <= 12){
					memset(snifferS.network.bssid,0,sizeof(snifferS.network.bssid));
					memcpy(snifferS.network.bssid,value.c_str(),value.length());
				}
			}else if(key == "nMessages"){
				// check if is number
				snifferS.network.nMessages = value.toInt();
			}
			//delay(10);
		}
		
		uint8_t clientId = 0;
		String uid = String(snifferS.fw.uid);
		if(uid == "")
			return;

		String nMessages = String(snifferS.network.nMessages);
		String bssid = String(snifferS.network.bssid);

		String topic = "/app/sniffer/"+uid+"/network";
		String payload = "{\"bssid\":\""+bssid+"\",\"nMessages\":\""+nMessages+"\"}";
		callback(clientId,topic,payload,2,false);

	}else if (text.indexOf("WIFI=") > -1) {
		text = text.substring(sizeof("WIFI"));

		DeserializationError error = deserializeJson(doc, text);
		if(error)
			return;

		for (JsonPair kv : doc.as<JsonObject>()) {
			String key = String(kv.key().c_str());
			String value = kv.value().as<String>();
			#ifdef DEBUG_SNIFFER
				LOG_DEBUG("%s:%s\n", key.c_str(), value.c_str());
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
			}
		}

		uint8_t clientId = 0;
		String uid = String(snifferS.fw.uid);
		if(uid == "")
			return;

		String ssid = String(snifferS.network.ssid);
		String pwd = String(snifferS.network.pwd);

		String topic = "/app/sniffer/"+uid+"/settings/wifi";
		String payload = "{\"ssid\":\""+ssid+"\",\"pwd\":\""+pwd+"\"}";
		callback(clientId,topic,payload,2,false);

	}else if (text.indexOf("FIRMWARE=") > -1) {
		text = text.substring(sizeof("FIRMWARE"));

		bool fSendVersion = false;
		bool fSendModel = false;
		bool fSendVariant = false;

		DeserializationError error = deserializeJson(doc, text);
		if(error)
			return;

		for (JsonPair kv : doc.as<JsonObject>()) {
			String key = String(kv.key().c_str());
			String value = kv.value().as<String>();
			#ifdef DEBUG_SNIFFER
				LOG_DEBUG("%s:%s\n", key.c_str(), value.c_str());
			#endif
			if(key == "version"){
				// check if is number
				if(value.length() <= sizeof(snifferS.fw.version)){
					memset(snifferS.fw.version,0,sizeof(snifferS.fw.version));
					memcpy(snifferS.fw.version,value.c_str(),value.length());
					fSendVersion = true;
				}
			}
			else if(key == "model"){
				// check if is number
				if(value.length() <= sizeof(snifferS.fw.model)){
					memset(snifferS.fw.model,0,sizeof(snifferS.fw.model));
					memcpy(snifferS.fw.model,value.c_str(),value.length());
					fSendModel = true;
				}
			}
			else if(key == "variant"){
				// check if is number
				if(value.length() <= sizeof(snifferS.fw.variant)){
					memset(snifferS.fw.variant,0,sizeof(snifferS.fw.variant));
					memcpy(snifferS.fw.variant,value.c_str(),value.length());
					fSendVariant = true;
				}
			}else if(key == "mac"){
				LOG_INFO("save mac:%s\n", value.c_str());
				// check if is number
				if(value.length() <= sizeof(snifferS.fw.uid)){
					memset(snifferS.fw.uid,0,sizeof(snifferS.fw.uid));
					memcpy(snifferS.fw.uid,value.c_str(),value.length());
				}
			}
		}

		uint8_t clientId = 0;
		String uid = String(snifferS.fw.uid);
		if(uid == "")
			return;

		if(fSendVersion){
			String version = String(snifferS.fw.version);
			String topic = "/app/sniffer/"+uid+"/version";
			String payload = version;
			callback(clientId,topic,payload,2,true);
		}
		if(fSendModel){
			String model = String(snifferS.fw.model);
			topic = "/app/sniffer/"+uid+"/model";
			payload = model;
			callback(clientId,topic,payload,2,true);
		}
		if(fSendVariant){
			String variant = String(snifferS.fw.variant);
			topic = "/app/sniffer/"+uid+"/variant";
			payload = variant;
			callback(clientId,topic,payload,2,true);
		}

	}else if (text.indexOf("SETTINGS=") > -1) {
		text = text.substring(sizeof("SETTINGS"));

		DeserializationError error = deserializeJson(doc, text);
		if(error)
			return;

		uint8_t clientId = 0;

		for (JsonPair kv : doc.as<JsonObject>()) {
			String key = String(kv.key().c_str());
			String value = kv.value().as<String>();
			#ifdef DEBUG_SNIFFER
				LOG_DEBUG("%s:%s\n", key.c_str(), value.c_str());
			#endif
			if(key == "sniffer_active"){
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
			}else if(key == "channel"){
				// check if is number
				snifferS.network.channel = value.toInt();
			}
			//delay(10);
		}

		String uid = String(snifferS.fw.uid);
		if(uid == "")
			return;

		String topic = "/app/sniffer/"+uid+"/settings/log";
		String keepalive_period = String(snifferS.settings.keepalive_period);
		String log_level = String(snifferS.settings.log_level);
		String payload = "{\"keepalive_period\":\""+keepalive_period+"\",\"log_level\":\""+log_level+"\"}";
		callback(clientId,topic,payload,2,false);

		String sniffer_active = String(snifferS.settings.sniffer_active);
		String packets_period = String(snifferS.settings.packets_period);
		String channel = String(snifferS.network.channel);
		
		topic = "/app/sniffer/"+uid+"/settings/packets";
		payload = "{\"sniffer_active\":\""+sniffer_active+"\",\"packets_period\":\""+packets_period+"\",\"channel\":\""+channel+"\"}";
		callback(clientId,topic,payload,2,false);

	}else if (text.indexOf("update=") > -1){
		uint8_t clientId = 0;
		text = text.substring(sizeof("update"));
		String uid = String(snifferS.fw.uid);
		if(uid == "")
			return;
		String topic = "/app/sniffer/"+uid+"/fota/update/status";
		callback(clientId,topic,text,2,false);
		delay(10);
	}

}

void Sniffer::parse_mqtt_messages(uint8_t clientID, String topic, String payload, MqttCallback callback){

	uint8_t index = topic.indexOf("/");
	String snifferTopic = topic.substring(index+1); // filter first "/" keyword
	index = snifferTopic.indexOf("/");
	snifferTopic = snifferTopic.substring(index+1); // filter "app" keyword
	index = snifferTopic.indexOf("/");
	snifferTopic = snifferTopic.substring(index+1); // filter model
	index = snifferTopic.indexOf("/");
	snifferTopic = snifferTopic.substring(index); // filter uid

	LOG_DEBUG("sniffer topic: %s\n", snifferTopic.c_str());

	String subtopic = "";
	bool get = false;
	bool store = false;

	if(topic.endsWith("/get")){
		get = true;
		uint16_t index = topic.lastIndexOf("/");
		subtopic = topic.substring(0,index); // filter get
	}

	switch(resolveOption(snifferTopics,snifferTopic)){
		case sniffer_reboot_:
			Serial1.println("reboot:1");
			break;
		case sniffer_reset_:
			LOG_WARN("Not implemented !!\n");
			break;
		case sniffer_version_get_:
			{
				String version = String(snifferS.fw.version);
				callback(clientID,subtopic,version,2,true);
			}
			break;
		case sniffer_model_get_:
			{
				String model = String(snifferS.fw.model);
				callback(clientID,subtopic,model,2,true);
			}
			break;
		case sniffer_variant_get_:
			{
				String variant = String(snifferS.fw.variant);
				callback(clientID,subtopic,variant,2,true);
			}
			break;
		case sniffer_status_get_:
			{
				callback(clientID,subtopic,"online",2,true);
			}
			break;
		case sniffer_fota_update_:
			{  
				DeserializationError error = deserializeJson(doc, payload);
				if(error){
					LOG_ERROR("Not Json\n");
					return;
				}

				if(doc.containsKey("url")){
					String url = doc["url"];
					Serial1.println("update:"+url);
				}
			}
			break;
		case sniffer_settings_get_:
			{
				LOG_DEBUG("settings get\n");
				String ssid = String(snifferS.network.ssid);
				String pwd = String(snifferS.network.pwd);
				String payload = "{\"ssid\":\""+ssid+"\",\"pwd\":\""+pwd+"\"}";
				String newTopic = subtopic + "/wifi";
				callback(clientID,newTopic,payload,2,false);

				String keepalive_period = String(snifferS.settings.keepalive_period);
				String log_level = String(snifferS.settings.log_level);
				payload = "{\"keepalive_period\":\""+keepalive_period+"\",\"log_level\":\""+log_level+"\"}";
				newTopic = subtopic + "/log";
				callback(clientID,newTopic,payload,2,false);

				String sniffer_active = String(snifferS.settings.sniffer_active);
				String packets_period = String(snifferS.settings.packets_period);
				String channel = String(snifferS.network.channel);

				payload = "{\"sniffer_active\":\""+sniffer_active+"\",\"packets_period\":\""+packets_period+"\",\"channel\":\""+channel+"\"}";
				newTopic = subtopic + "/packets";
				callback(clientID,newTopic,payload,2,false);
			}
			break;
		case sniffer_settings_wifi_:
			{
				DeserializationError error = deserializeJson(doc, payload);
				if(error){
					LOG_ERROR("Not Json\n");
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
			}
			break;
		case sniffer_settings_wifi_get_:
			{
				String ssid = String(snifferS.network.ssid);
				String pwd = String(snifferS.network.pwd);
				String payload = "{\"ssid\":\""+ssid+"\",\"pwd\":\""+pwd+"\"}";
				callback(clientID,subtopic,payload,2,false);
			}
			break;
		case sniffer_settings_log_:
			{
				DeserializationError error = deserializeJson(doc, payload);
				if(error){
					LOG_ERROR("Not Json\n");
					return;
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

				if(doc.containsKey("log_level")){
					#ifndef UNITTEST
						String log_level = doc["log_level"];
					#else
						String log_level = "";
						if(doc["log_level"].is_string())
							log_level = doc["log_level"];
					#endif
					Serial1.println("log_level:"+log_level);
					delay(100);
				}
			}
			break;
		case sniffer_settings_log_get_:
			{
				String keepalive_period = String(snifferS.settings.keepalive_period);
				String log_level = String(snifferS.settings.log_level);
				String payload = "{\"keepalive_period\":\""+keepalive_period+"\",\"log_level\":\""+log_level+"\"}";
				callback(clientID,subtopic,payload,2,false);
			}
			break;
		case sniffer_settings_packets_:
			{
				DeserializationError error = deserializeJson(doc, payload);
				if(error){
					LOG_ERROR("Not Json\n");
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
					Serial1.println("sniffer_active:"+sniffer_active);
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
		case sniffer_settings_packets_get_:
			{
				String sniffer_active = String(snifferS.settings.sniffer_active);
				String packets_period = String(snifferS.settings.packets_period);
				String channel = String(snifferS.network.channel);
				String nMessages = String(snifferS.network.nMessages);
				String payload = "{\"sniffer_active\":\""+sniffer_active+"\",\"packets_period\":\""+packets_period+"\",\"channel\":\""+channel+"\"}";
				callback(clientID,subtopic,payload,2,false);
			}
			break;
		case sniffer_serial_:
			LOG_WARN("Not implemented !!\n");
			break;
		case sniffer_not_found:
			LOG_WARN("sniffer topic not found\n");
			break;
	}

	/*
	if(store){
		store_settings();
		log_settings();
	}
	*/

}

snifferTopics_ Sniffer::resolveOption(std::map<long, snifferTopics_> map, String topic) {

	std::string topic_ = std::string(topic.c_str());
	long str_hash = (long)std::hash<std::string>{}(topic_);
	std::map<long,snifferTopics_>::iterator it;

	it = map.find(str_hash);
	if(it != map.end())
		return it->second;

	return sniffer_not_found;
}