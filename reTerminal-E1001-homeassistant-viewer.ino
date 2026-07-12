#include <GxEPD2_BW.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <time.h>
#include <WebSocketsClient.h>

#include "reTerminal-E1001.h"
#include "data.h"
#include "config.h"
#include "Utils.h"

const char *VERSION = "1.0.0";

MyUtils utils(&Serial1);

// Button state variables
bool lastKeyGreen = HIGH;
bool lastKeyRight = HIGH;
bool lastKeyLeft  = HIGH;

ReTerminalE1001 reTerminal;

StationData stationData;

// Allocate the JSON document
// Use arduinojson.org/v6/assistant to compute the capacity.
const size_t capacity = 1024*4;
DynamicJsonDocument doc(capacity);

WebSocketsClient webSocket;
String message;

bool WSConnected = false;
uint32_t disconnectionTime=0;

const uint32_t refresh_ms=100;
const uint32_t max_disconnection_time= 1000*60*2/refresh_ms; // 2 minutes
const uint32_t max_uptime    = 1000*20/refresh_ms;           // 20 seconds up before deep sleep

uint32_t refresh_step=0;
bool redraw=false;
bool sdcard_available=false;

bool updateData1=false;
bool updateData2=false;
bool updateData3=false;
bool updateData4=false;
bool updateData5=false;
bool updateData6=false;

/**
 * @brief Prints the reason for waking up from deep sleep.
 */
void wakeUp() {
  esp_sleep_wakeup_cause_t wakeupReason = reTerminal.wakeUp();
  switch (wakeupReason) {
    case ESP_SLEEP_WAKEUP_EXT0:      
      break;
    case ESP_SLEEP_WAKEUP_TIMER:
      break;
    default:
      reTerminal.welcome(VERSION);
      break;
  }
}

void deepSleep1h() {  
  reTerminal.welcome(VERSION,false);
  reTerminal.deepSleep(1000000 * 60 * 60); // one hour
}

void deepSleep8h() {  
  reTerminal.welcome(VERSION,false);
  reTerminal.deepSleep(1000000 * 60 * 60 * 8); // eight hours
}

void deepSleep5m() {  
  reTerminal.deepSleep(1000000 * 60 * 5); // 5 min
}

void deepSleep10m() {  
  reTerminal.deepSleep(1000000 * 60 * 10); // 10 min
}

void deepSleep1s() {  
  reTerminal.deepSleep(1000000); // 5 min
}

void setup()
{
  Serial1.begin(115200, SERIAL_8N1, SERIAL_RX, SERIAL_TX);
  while (!Serial1) {
    delay(10);
  }

  Serial1.println("System starting...");

  reTerminal.init();

  wakeUp();  // Print the reason for waking up  

  sdcard_available=reTerminal.mountSDCard();
  
  // Waiting for Wifi
  if (WiFi.status() != WL_CONNECTED)
  {
    waitForWifi();
  }   

	// Opening Websocket on HomeAssistant
	webSocket.begin(ha_server, ha_port, ha_endpoint);

	// Configuring Event handler
	webSocket.onEvent(webSocketEvent);    
}

void updateE1001Data() {
  stationData.setBatteryPercent(reTerminal.getBatteryPercent(reTerminal.getBatteryVoltage()));
}

void activity_led(bool active)
{
  if (active)
  {
    digitalWrite(LED_PIN, LOW);
  }
  else
  {
    digitalWrite(LED_PIN, HIGH);
  }
}

void waitForWifi()
{

  if (!utils.wifi_start(false,wifi_ssid,wifi_password,activity_led,max_disconnection_time))
  {
    deepSleep8h();
  }
  activity_led(false);
}

float extractFloat(const char *temperatureString) {
  float temperature;
  JsonObject temperature_;
  if (doc["event"]["a"][temperatureString])
  {
    temperature_ = doc["event"]["a"][temperatureString];
  }
  if (doc["event"]["c"][temperatureString]["+"])
  {
    temperature_ = doc["event"]["c"][temperatureString]["+"];
  }
  temperature=atof(temperature_["s"]);
  return temperature;
}

const char *extractString(const char *temperatureString) {
  JsonObject data_;
  if (doc["event"]["a"][temperatureString])
  {
    data_ = doc["event"]["a"][temperatureString];
  }
  if (doc["event"]["c"][temperatureString]["+"])
  {
    data_ = doc["event"]["c"][temperatureString]["+"];
  }  
  return data_["s"];
}

void subscribeToEvent(uint8_t id, const char *event) {
  String message="{ \"id\": "+String(id)+", \"type\": \"subscribe_entities\", \"entity_ids\": [\"";
  message.concat(event);
  message.concat("\"]}");
  webSocket.sendTXT(message);   
}

void processHAMessages(uint8_t * payload)
{
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  const char* type = doc["type"];
  const int id=doc["id"];

  if (strcmp(type,"auth_required") == 0)
  {
    // send Authentication
    message="{ \"type\": \"auth\", \"access_token\": \"";
    message.concat(ha_token);
    message.concat("\" }");
    webSocket.sendTXT(message);   
  }
  else if (strcmp(type,"auth_ok") == 0)
  {
    // subsribe to weather events
    message="{ \"id\": 1, \"type\": \"weather/subscribe_forecast\", \"forecast_type\": \"daily\", \"entity_id\": \"";
    message.concat(ha_weather_entity);
    message.concat("\" }");
    webSocket.sendTXT(message); 

    subscribeToEvent(2, ha_temp_indoor);
    subscribeToEvent(3, ha_temp_outdoor);
    subscribeToEvent(4, ha_temp_other);
    subscribeToEvent(5, ha_sun);
    subscribeToEvent(6, ha_garage);
  }
  else if (strcmp(type,"result") == 0)
  {
    bool success = doc["success"];
    if (!success)
    {
      Serial1.println("ERROR");
    }
  }  
  else if (strcmp(type,"event") == 0)
  {
    if (id==1)
    {
      int i=0;
      for (JsonObject event_forecast_item : doc["event"]["forecast"].as<JsonArray>()) {
        const char*   event_forecast_item_datetime =      event_forecast_item["datetime"];
        const char*   event_forecast_item_condition =     event_forecast_item["condition"]; 
        float         event_forecast_item_temperature =   event_forecast_item["temperature"]; 
        float         event_forecast_item_templow =       event_forecast_item["templow"]; 
        float         event_forecast_item_precipitation = event_forecast_item["precipitation"]; 
        int           event_forecast_item_humidity =      event_forecast_item["humidity"]; 
        if (i==0)
        {
          stationData.setDate(event_forecast_item_datetime);          
        }
        stationData.updateForecast(i, event_forecast_item_condition);
        stationData.updateTemperature(i, event_forecast_item_temperature);
        stationData.updateTemperatureLow(i, event_forecast_item_templow);
        i++;
      }
      updateData1=true;
    }
    if(id==2)
    {
      stationData.updateTemperatureIndoor(extractFloat(ha_temp_indoor));
      updateData2=true;
    }
    if (id==3)
    {
      stationData.updateTemperatureOutdoor(extractFloat(ha_temp_outdoor));
      updateData3=true;
    }
    if (id==4)
    {
      stationData.updateTemperatureOther(extractFloat(ha_temp_other));
      updateData4=true;
    }
    if (id==6)
    {
      stationData.setGarageOpen(extractString(ha_garage));
      updateData6=true;
    }
  }  
  else
  {
    Serial1.println(type);
  } 
  redraw=updateData1 && updateData2 && updateData3 && updateData4 && updateData6;
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
	switch(type) {
		case WStype_DISCONNECTED:
      if (WSConnected)
      {
			  Serial1.printf("[WS] Disconnected!\n");
        disconnectionTime=0;
      }
      WSConnected=false;
			break;
		case WStype_CONNECTED:
			Serial1.printf("[WS] Connected to url: %s\n", payload);      
      WSConnected=true;
			break;
		case WStype_TEXT:
			Serial1.printf("[WS] get text: %s\n", payload);
      processHAMessages(payload);
			break;
		case WStype_BIN:
			Serial1.printf("[WS] get binary length: %u\n", length);
			break;
		case WStype_ERROR:			
		case WStype_FRAGMENT_TEXT_START:
		case WStype_FRAGMENT_BIN_START:
		case WStype_FRAGMENT:
		case WStype_FRAGMENT_FIN:
			break;
	}
}


void loop() {  

    // Read current button states
  bool keyGreenState = digitalRead(BUTTON_GREEN);
  bool keyRightState = digitalRead(BUTTON_RIGHT);
  bool keyLeftState  = digitalRead(BUTTON_LEFT);

  // Check Green Button
  if (keyGreenState != lastKeyGreen) {
    if (keyGreenState == LOW) {
      Serial1.println("GREEN pressed!");   
      refresh_step=0;
      updateE1001Data();
      reTerminal.mainMenu(stationData); 
    }
    lastKeyGreen = keyGreenState;
  }

  // Check Right Button
  if (keyRightState != lastKeyRight) {
    if (keyRightState == LOW) {
      Serial1.println("RIGHT pressed!"); 
      deepSleep1s();    
    }
    lastKeyRight = keyRightState;
  }

  // Check Left Button
  if (keyLeftState != lastKeyLeft) {
    if (keyLeftState == LOW) {
      Serial1.println("LEFT pressed!");     
    }
    lastKeyLeft = keyLeftState;
  }
  
  delay(refresh_ms);
  webSocket.loop();

  if (redraw)
  {    
    Serial1.println("Updating screen");
    updateE1001Data();
    reTerminal.mainMenu(stationData);
    redraw=false;
  }

  if (max_uptime < refresh_step)
  {    
    deepSleep5m();
  }
  
  refresh_step++;
}



