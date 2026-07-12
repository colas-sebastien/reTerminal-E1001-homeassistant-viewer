// Wifi Information
const char* wifi_ssid     = "";                                                         // SSID
const char* wifi_password = "";                                                         // SSID PASSWORD

// Home Assistant information
const char      *ha_server           = "";                                              // HOME ASSISTANT HOSTNAME OR IP
const uint16_t   ha_port             = 8123;                                            // HOME ASSISTANT PORT
const char      *ha_endpoint         = "/api/websocket";                                // HOME ASSISTANT WEBSOCKET
const char      *ha_token            = "";                                              // HOME ASSISTANT TOKEN
const char      *ha_weather_entity   = "weather.vernon";                                // WEATHER ENTITY
const char      *ha_temp_outdoor     = "sensor.temperature_exterieure";                 // OUTDOOR SENSOR
const char      *ha_temp_indoor      = "sensor.temperature_salon_temperature";          // INDOOR  SENSOR
const char      *ha_temp_other       = "sensor.temperature_bureau_temperature";         // SECONDARY INDOOR SENSOR
const char      *ha_sun              = "sun.sun";                                       // INFORMATION
const char      *ha_garage           = "binary_sensor.porte_garage_opening";            // GARAGE SENSOR