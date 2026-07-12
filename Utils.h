#ifndef Utils_h
#define Utils_h

#include <WiFi.h>
#include <ESPmDNS.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "HardwareSerial.h"

class MyUtils
{
  using InputEvent = void (*)(bool);

  public:
    MyUtils(HardwareSerial *output);
    bool   wifi_start(const bool AccessPoint,const char *ssid,const char *password,InputEvent event, uint32_t timeout=0);
    void   mdns_start(const char *hostname);
    void   SPIFFS_start();
    void   SPIFFS_files();
    String SPIFFS_readFile(char *path);
    bool   SPIFFS_exist(char *path);
    void   SPIFFS_writeFileJSON(char *filename,JsonDocument doc);    
    void   HTTP_handleFileUpload(WebServer *httpServer);
    bool   HTTP_handleFileRead(WebServer *httpServer,String path);
  private:
    String getContentType(String filename);
    File _fsUploadFile;
    HardwareSerial *log;
};

#endif