#ifndef STATION_DATA
#define STATION_DATA

#include <cstring>
#include "HardwareSerial.h"

#define STATION_DATA_ELEMENTS 15
#define WEATHER_STATES 15

class StationData
{
  public:  
    
    char    *getDate();
    void    setDate(const char *str);
    
    char    *getForecast(uint16_t index);
    bool    updateForecast(uint16_t index, const char *str);

    float   getTemperature(uint16_t index);
    bool    updateTemperature(uint16_t index, float temp);

    float   getTemperatureLow(uint16_t index);
    bool    updateTemperatureLow(uint16_t index, float temp);

    float   getTemperatureIndoor();
    bool    updateTemperatureIndoor(float temp);

    float   getTemperatureOutdoor();
    bool    updateTemperatureOutdoor(float temp);

    float   getTemperatureOther();
    bool    updateTemperatureOther(float temp);    

    int     getBatteryPercent();
    void    setBatteryPercent(int bat);

    bool    isGarageOpen();
    void    setGarageOpen(const char *state);

    StationData();
  private:
    bool    garage_open=false;
    float   temperatures[STATION_DATA_ELEMENTS];
    float   temperatures_low[STATION_DATA_ELEMENTS];
    float   temperature_indoor=0;
    float   temperature_outdoor=0;
    float   temperature_other=0;
    char    date[20];
    char    forecasts[STATION_DATA_ELEMENTS][30];
    int     battery_percent;
    char    weather_values[WEATHER_STATES][30]={  "clear-night", "cloudy", "exceptional", "fog", "hail", "lightning", "lightning-rainy",
                                  "partlycloudy", "pouring", "rainy", "snowy", "snowy-rainy", "sunny", "windy", "windy-variant"                                
                               };
    bool isValidWeather(const char *str);
};



#endif