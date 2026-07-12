#include "data.h"

StationData::StationData(){
  for (int i=0; i<STATION_DATA_ELEMENTS;i++) {
    temperatures[i]=0.0;
    temperatures_low[i]=0.0;
    strcpy(forecasts[i],"N/A");
    strcpy(date,"00/00/0000");
  }  
}

void StationData::setDate(const char *str){
  strcpy(date,str);
}

char *StationData::getDate(){
  return date;
}

char *StationData::getForecast(uint16_t index) {
  return forecasts[index];
}

bool StationData::updateForecast(uint16_t index, const char *str)
{
  if ((strcmp(forecasts[index],str)!=0) && isValidWeather(str))
  {
    strcpy(forecasts[index],str);
    return true;
  }
  else
  {
    return false;
  }
}

float StationData::getTemperature(uint16_t index) {
  return temperatures[index];
}

bool  StationData::updateTemperature(uint16_t index, float temp) {
  if (temperatures[index]!=temp)
  {    
    temperatures[index]=temp;
    return true;
  }
  else
  {
    return false;
  }
}

float StationData::getTemperatureLow(uint16_t index) {
  return temperatures_low[index];
}

bool  StationData::updateTemperatureLow(uint16_t index, float temp) {
  if (temperatures_low[index]!=temp)
  {
    temperatures_low[index]=temp;
    return true;
  }
  else
  {
    return false;
  }
}

float StationData::getTemperatureIndoor() {
  return temperature_indoor;
}

bool StationData::updateTemperatureIndoor(float temp) {
  if (temperature_indoor!=temp)
  {
    temperature_indoor=temp;
    return true;
  }
  else
  {
    return false;
  }  
}

float StationData::getTemperatureOutdoor() {
  return temperature_outdoor;
}

bool StationData::updateTemperatureOutdoor(float temp) {
  if (temperature_outdoor!=temp)
  {
    temperature_outdoor=temp;
    return true;
  }
  else
  {
    return false;
  }  
}

float StationData::getTemperatureOther() {
  return temperature_other;
}

bool StationData::updateTemperatureOther(float temp) {
  if (temperature_other!=temp)
  {
    temperature_other=temp;
    return true;
  }
  else
  {
    return false;
  }  
}



int StationData::getBatteryPercent() {
  return battery_percent;
}

void StationData::setBatteryPercent(int bat) {
  battery_percent=bat;
}

bool StationData::isValidWeather(const char *str) {
  for (int i=0;i<WEATHER_STATES;i++)
  {
    if (strcmp(str,weather_values[i])==0)
    {
      return true;
    }
  }
  Serial.print("Unkown weather: ");
  Serial.println(str);
  return false;
}

bool StationData::isGarageOpen() {
  return garage_open;
}

void StationData::setGarageOpen(const char *state) {
  garage_open=strcmp(state,"on")==0;
}
