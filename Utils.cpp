#include <cstdint>
#include <stdint.h>
#include "Utils.h"


MyUtils::MyUtils(HardwareSerial *output)
{
  log=output;
}

bool MyUtils::wifi_start(const bool AccessPoint,const char *ssid,const char *password,InputEvent event, uint32_t timeout)
{
  uint8_t delay_ms=10;
  bool active=true;
  event(active);
  if (AccessPoint)
  {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password); 
    log->print("Activating wifi Access Point: ");
    log->println(ssid);  
    log->print("IP address: "); 
    log->println(WiFi.softAPIP());  
    return true;
  }
  else
  {
    uint32_t count=0;
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);  
    log->print("Connecting to wifi: ");
    log->println(ssid);    
    while (!WiFi.isConnected())
    {      
        delay(delay_ms);
        count++;
        if(count%20==0)
        {
          active=!active;
          event(active);
        }
        if ((timeout!=0) && (count>timeout/delay_ms))
        {
          log->println("Can't connect: timeout"); 
          return false;
        }
    }
    log->print("IP address: "); 
    log->println(WiFi.localIP());  
    return true;
  }
}

void MyUtils::mdns_start(const char *hostname)
{
    esp_err_t err = mdns_init();
    if (err) {
      log->println("MDNS Init failed\n");
      return;
    }
    mdns_hostname_set(hostname);
    mdns_instance_name_set(hostname);
    String msg="MDNS Started with hostname: ";
    msg+=hostname;
    log->println(msg);
}


void MyUtils::SPIFFS_files()
{   const char *dirname="/";

    log->println("Listing SPIFFS / directory");

    File root = SPIFFS.open(dirname);
    if (!root) {
        log->println("Failed to open directory");
        return;
    }

    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            log->print("  DIR : ");
            log->println(file.name());
        } else {
            log->print("  FILE: ");
            log->println(file.name());
        }
        file = root.openNextFile();
    }
}

void MyUtils::SPIFFS_start()                  // Start the SPI Flash File System (SPIFFS)
{ 
  if (!SPIFFS.begin())               // Try to mount without formating
  {
    log->println("SPIFFS Mount Failed");
    log->println("Formating");
    if (!SPIFFS.begin(true))         // Format if not able to mount
    {
      log->println("Error during format");
      return;
    }    
  }                   
  log->println("SPIFFS started.");
}

String MyUtils::SPIFFS_readFile(char *path) {
  String buffer = "";
  log->printf("Reading file: %s\r\n", path);

  File file = SPIFFS.open(path);
  if (!file || file.isDirectory()) {
    log->println("- failed to open file for reading");
    return "";
  }

  while (file.available()) {
    buffer += (char)file.read();
  }
  file.close();
  return buffer;
}

bool MyUtils::SPIFFS_exist(char *path) {
  return (SPIFFS.exists(path));
}

void MyUtils::SPIFFS_writeFileJSON(char *filename,JsonDocument doc)
{
  File file = SPIFFS.open(filename, FILE_WRITE);
  serializeJson(doc, file);
  file.close();  
}

// upload a new file to the SPIFFS
void MyUtils::HTTP_handleFileUpload(WebServer *httpServer) 
{         
    HTTPUpload& upload = httpServer->upload();
    if(upload.status == UPLOAD_FILE_START)
    {
        String filename = upload.filename;
        if(!filename.startsWith("/")) filename = "/"+filename;
        log->print("handleFileUpload Name: "); log->println(filename);
        _fsUploadFile = SPIFFS.open(filename, "w"); // Open the file for writing in FS (create if it doesn't exist)                
    }
    else if(upload.status == UPLOAD_FILE_WRITE)
    {
        if(_fsUploadFile) _fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
        log->print("=");
    }
    else if(upload.status == UPLOAD_FILE_END)
    {
        if(_fsUploadFile)
        {
          log->println("> OK");
        }
        else
        {
          log->println("> KO");
        }        

        if(_fsUploadFile) // If the file was successfully created
        {
            _fsUploadFile.close();
            log->print("handleFileUpload Size: "); log->println(upload.totalSize);
        }
        else
        {
            httpServer->send(500, "text/plain", "500: couldn't create file");
        }
    }
}

// convert the file extension to the MIME type
String MyUtils::getContentType(String filename) 
{ 
    if (filename.endsWith(".html"))     return "text/html";
    else if (filename.endsWith(".htm")) return "text/html";
    else if (filename.endsWith(".css")) return "text/css";
    else if (filename.endsWith(".js"))  return "application/javascript";
    else if (filename.endsWith(".ico")) return "image/x-icon";
    else if (filename.endsWith(".png")) return"image/png";
    else if (filename.endsWith(".gif")) return "image/gif";
    else if (filename.endsWith(".jpg")) return "image/jpeg";
    else if (filename.endsWith(".xml")) return "text/xml";
    else if (filename.endsWith(".pdf")) return "application/x-pdf";
    else if (filename.endsWith(".zip")) return "application/x-zip";
    else if (filename.endsWith(".gz"))  return "application/x-gzip";
    else if (filename.endsWith(".json"))return "application/json";
    return "text/plain";
}

// send the right file to the client (if it exists)
bool MyUtils::HTTP_handleFileRead(WebServer *httpServer,String path)
{    
    log->println("handleFileRead: " + path);
    
    if (path.endsWith("/")) path += "index.html"; // If a folder is requested, send the index file
    String contentType = getContentType(path); // Get the MIME type

    if (SPIFFS.exists(path)) // If the file exists
    { 
        File file = SPIFFS.open(path, "r"); // Open it
        size_t sent = httpServer->streamFile(file, contentType); // And send it to the client
        file.close(); // Then close the file again
        return true;
    }
    log->println("\tFile Not Found");
    return false; // If the file doesn't exist, return false
}




