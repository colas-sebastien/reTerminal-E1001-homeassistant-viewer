#ifndef E1001_LIB
#define E1001_LIB

#include <SD.h>
#include <SPI.h>
#include <cmath>
#include <SensirionI2cSht4x.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeSerifBold24pt7b.h>

#include "weather_images.h"
#include "data.h"
#include "language.h"

#define SERIAL_RX 44
#define SERIAL_TX 43
#define LED_PIN 6               // GPIO6 - Onboard LED (inverted logic)

// Battery monitoring pins
#define BATTERY_ADC_PIN 1       // GPIO1 - Battery voltage ADC
#define BATTERY_ENABLE_PIN 21   // GPIO21 - Battery monitoring enable
#define BATTERY_HIGH 4.10
#define BATTERY_LOW  3

// I2C pins for reTerminal E Series
#define I2C_SDA 19
#define I2C_SCL 20

// SD Card
#define SD_EN_PIN   16
#define SD_DET_PIN  15
#define SD_CS_PIN   14
#define SD_MISO_PIN 8

// Define ePaper SPI pins
#define EPD_SCK_PIN  7
#define EPD_MOSI_PIN 9
#define EPD_CS_PIN   10
#define EPD_DC_PIN   11
#define EPD_RES_PIN  12
#define EPD_BUSY_PIN 13

// 0: reTerminal E1001 (7.5'' B&W)

#define GxEPD2_DISPLAY_CLASS GxEPD2_BW
#define GxEPD2_DRIVER_CLASS GxEPD2_750_GDEY075T7 // 7.5'' B&W driver

#define MAX_DISPLAY_BUFFER_SIZE 16000

#define MAX_HEIGHT(EPD)                                        \
    (EPD::HEIGHT <= MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8) \
         ? EPD::HEIGHT                                         \
         : MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8))

// Define button pins according to schematic
const int BUTTON_GREEN = 3;    // KEY0 - GPIO3
const int BUTTON_RIGHT = 4;    // KEY1 - GPIO4
const int BUTTON_LEFT  = 5;    // KEY2 - GPIO5




class ReTerminalE1001
{
  public:
    void init();
    float getBatteryVoltage();
    int   getBatteryPercent(float batteryVoltage);
    void  deepSleep(uint64_t time);
    esp_sleep_wakeup_cause_t wakeUp();
    void getTemperatureAndHumidity(float& aTemperature, float& aHumidity);
    bool mountSDCard();

    // icons:
    void homeAssistant(uint16_t x, uint16_t y);
    void noWifi(uint16_t x, uint16_t y);
    void cloud(uint16_t x, uint16_t y);
    void cloudAndWind(uint16_t x, uint16_t y);
    void sun(uint16_t x, uint16_t y);
    void moon(uint16_t x, uint16_t y);
    void wind(uint16_t x, uint16_t y);
    void fog(uint16_t x, uint16_t y);
    void snow(uint16_t x, uint16_t y);
    void rain(uint16_t x, uint16_t y);
    void rain(uint16_t x, uint16_t y,uint8_t intensity);
    void lightning(uint16_t x, uint16_t y);
    void lightningRainy(uint16_t x, uint16_t y);
    void lightningRainySunny(uint16_t x, uint16_t y);
    void littleCloud(uint16_t x, uint16_t y);
    void thermometer(uint16_t x, uint16_t y);
    void garageOpen(uint16_t x, uint16_t y);
    // GUI Elements
    void grid();
    void icons();    
    void hibernate();
    void message(const char *msg);
    void weather(char* weather,uint16_t x,uint16_t y);
    bool drawBmp(String filename, int16_t x, int16_t y);
    // GUI Pages
    void mainMenu(StationData stationData);
    void welcome(const char *version,bool wifi=true);


  private:
    const char *img_sleep ="/800_480/sleep.bmp";
    const char *img_battery_low ="/512_400/battery.bmp";
    // Create sensor object
    SensirionI2cSht4x sht4x;  

    void   forecast(uint16_t x, uint16_t y, char state[], float temp, float temp_low);
    String formatDateFR(String dateStr);
    String formatDateEN(String dateStr);
    int    dayOfTheWeek(int y, int m, int d);
    String dayNameFR(String dateStr,int shift);   

    uint16_t read16(File &f);
    uint32_t read32(File &f);     
};


#endif