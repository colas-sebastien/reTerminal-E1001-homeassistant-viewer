#include "WString.h"
#include "reTerminal-E1001.h"

SPIClass hspi(HSPI);    

// Initialize display object
GxEPD2_DISPLAY_CLASS<GxEPD2_DRIVER_CLASS, MAX_HEIGHT(GxEPD2_DRIVER_CLASS)>
display(GxEPD2_DRIVER_CLASS(/*CS=*/EPD_CS_PIN, /*DC=*/EPD_DC_PIN,
                            /*RST=*/EPD_RES_PIN, /*BUSY=*/EPD_BUSY_PIN));

Language lang;
/**
 * @brief Read actual battery voltage from ADC.
 * @return Battery voltage in Volts.
 */
float ReTerminalE1001::getBatteryVoltage() {
  digitalWrite(BATTERY_ENABLE_PIN, HIGH);
  delay(5);
  int mv = analogReadMilliVolts(BATTERY_ADC_PIN);
  digitalWrite(BATTERY_ENABLE_PIN, LOW);
  return ((mv / 1000.0) * 2);  // Correction for voltage divider
}

/**
 * @brief Calculates the battery charge percentage based on the measured voltage.
 * @param batteryVoltage Measured battery voltage in volts.
 * @return Estimated battery charge percentage (0 to 100%).
 */
int ReTerminalE1001::getBatteryPercent(float batteryVoltage) {
  int percent=(batteryVoltage - BATTERY_LOW)*100/(BATTERY_HIGH-BATTERY_LOW);
  if (percent<0) 
  {
    percent=0;
  }
  if (percent>100)
  {
    percent=100;
  }
  return percent;
}

/**
 * @brief Ask the ESP32S3 to go in deep sleep
 */
void ReTerminalE1001::deepSleep(uint64_t time) {  
  Serial1.printf("Going to deep sleep\n");
  esp_sleep_enable_timer_wakeup(time);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_GREEN, LOW);
  esp_deep_sleep_start();    
}

/**
 * @brief Prints the reason for waking up from deep sleep.
 * @return The wakeup cause
 */
esp_sleep_wakeup_cause_t ReTerminalE1001::wakeUp() {
  esp_sleep_wakeup_cause_t wakeupReason = esp_sleep_get_wakeup_cause();
  switch (wakeupReason) {
    case ESP_SLEEP_WAKEUP_EXT0:
      Serial1.println("Wakeup caused by external signal (EXT0 - button press)");
      break;
    case ESP_SLEEP_WAKEUP_EXT1:
      Serial1.println("Wakeup caused by external signal (EXT1)");
      break;
    case ESP_SLEEP_WAKEUP_TIMER:
      Serial1.println("Wakeup caused by timer");
      break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
      Serial1.println("Wakeup caused by touchpad");
      break;
    default:
      Serial1.printf("Wakeup not caused by deep sleep: %d\n", wakeupReason);
      break;
  }
  return wakeupReason;
}


void ReTerminalE1001::init() {
  // Configure LED pin
  pinMode(LED_PIN, OUTPUT);

  // Configure button pins as inputs
  // Hardware already has pull-up resistors, so use INPUT mode
  pinMode(BUTTON_RIGHT, INPUT);
  pinMode(BUTTON_LEFT,  INPUT);

  // Configure the green button as an input with pull-up resistor
  pinMode(BUTTON_GREEN, INPUT_PULLUP);

  // Initialize I2C with custom pins
  Wire.begin(I2C_SDA, I2C_SCL);
  
  uint16_t error;
  char errorMessage[256];

  // Initialize the sensor
  sht4x.begin(Wire, 0x44);

  // Configure battery monitoring enable pin
  pinMode(BATTERY_ENABLE_PIN, OUTPUT);
  digitalWrite(BATTERY_ENABLE_PIN, HIGH);  // Enable battery monitoring
  
  // Configure ADC
  analogReadResolution(12);  // 12-bit resolution
  analogSetPinAttenuation(BATTERY_ADC_PIN, ADC_11db);

  // Initialize SD Card
  pinMode(SD_EN_PIN, OUTPUT);
  digitalWrite(SD_EN_PIN, HIGH);
  pinMode(SD_DET_PIN, INPUT_PULLUP);  

  // Initialize SPI
  //hspi.begin(EPD_SCK_PIN, -1, EPD_MOSI_PIN, -1);    
  hspi.begin(EPD_SCK_PIN, SD_MISO_PIN, EPD_MOSI_PIN, -1);

  // Initialize Display
  display.epd2.selectSPI(hspi, SPISettings(2000000, MSBFIRST, SPI_MODE0));

  pinMode(EPD_RES_PIN, OUTPUT);
  pinMode(EPD_DC_PIN, OUTPUT);
  pinMode(EPD_CS_PIN, OUTPUT);
  
  // Initialize display
  display.init(0);  
  
  delay(100);  // Allow circuit to stabilize  
}

void ReTerminalE1001::getTemperatureAndHumidity(float& aTemperature, float& aHumidity) {
  uint16_t error;
  char errorMessage[256];  
  error = sht4x.measureHighPrecision(aTemperature, aHumidity);

  if (error) {
          Serial1.print("Error trying to execute measureHighPrecision(): ");
          errorToString(error, errorMessage, 256);
          Serial1.println(errorMessage);
  }
}

bool ReTerminalE1001::mountSDCard() {
  if (digitalRead(SD_DET_PIN) == HIGH) {
    Serial1.println("No SD card detected. Please insert a card.");
    return false;
  }

  Serial1.println("SD card detected, attempting to mount...");
  if (!SD.begin(SD_CS_PIN, hspi)) {
    Serial1.println("SD Card Mount Failed!");
    return false;
  }
  Serial1.println("SD card mounted successfully.");  
  return true;
}

/**
 * @brief Zeller's congruence to compute day of week (0=Sunday).
 * @param y Year
 * @param m Month
 * @param d Day
 * @return Weekday index
 */
int ReTerminalE1001::dayOfTheWeek(int y, int m, int d) {
  if (m < 3) {
    m += 12;
    y -= 1;
  }
  int K = y % 100;
  int J = y / 100;
  int h = (d + 13 * (m + 1) / 5 + K + K / 4 + J / 4 + 5 * J) % 7;
  return ((h + 6) % 7);  // 0=Sunday
}

/**
 * @brief Formats ISO date string as US date for display.
 * @param dateStr "YYYY-MM-DD HH:MM"
 * @return "Saturday, October 4, 2025" style string.
 */
String ReTerminalE1001::formatDateEN(String dateStr) {
  int y = dateStr.substring(0, 4).toInt();
  int m = dateStr.substring(5, 7).toInt();
  int d = dateStr.substring(8, 10).toInt();
  int wday = dayOfTheWeek(y, m, d);
  return String(String(lang.days[wday]) + ", " + String(lang.months[m - 1]) + " " + String(d) + ", " + String(y));
}

/**
 * @brief Formats ISO date string as French date for display.
 * @param dateStr "YYYY-MM-DD HH:MM"
 * @return "Samedi 04 Octobre" style string.
 */
String ReTerminalE1001::formatDateFR(String dateStr) {
  int y = dateStr.substring(0, 4).toInt();
  int m = dateStr.substring(5, 7).toInt();
  int d = dateStr.substring(8, 10).toInt();  
  int wday = dayOfTheWeek(y, m, d);
  return String(lang.days[wday]) + " " + String(d) + " " + lang.months[m - 1];
}

/**
 * @brief Formats ISO date string as French day with shift
 * @param dateStr "YYYY-MM-DD HH:MM"
 * @param shift   number
 * @return "Samedi" style string.
 */
String ReTerminalE1001::dayNameFR(String dateStr,int shift) {
  int y = dateStr.substring(0, 4).toInt();
  int m = dateStr.substring(5, 7).toInt();
  int d = dateStr.substring(8, 10).toInt();
  int wday = (dayOfTheWeek(y, m, d)+shift)%7;
  return String(lang.days[wday]);
}

void ReTerminalE1001::message(const char *msg)
{
  display.setRotation(0);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  display.setFullWindow();
  display.firstPage();  
  do
  {
    display.fillScreen(GxEPD_WHITE);
    homeAssistant(20,20);
    display.setCursor(200, 50);
    display.print(msg);
  }
  while (display.nextPage());    
}

void ReTerminalE1001::homeAssistant(uint16_t x, uint16_t y)
{
  display.drawBitmap(x, y, epd_bitmap_ha, 136, 120, GxEPD_BLACK);
}

void ReTerminalE1001::noWifi(uint16_t x, uint16_t y)
{
  display.drawBitmap(x, y, epd_bitmap_no_wifi, 108, 104, GxEPD_BLACK);
}

void ReTerminalE1001::cloud(uint16_t x, uint16_t y)
{
  display.drawBitmap(x+12, y+12, epd_bitmap_cloud, 103, 74, GxEPD_BLACK);
}

void ReTerminalE1001::sun(uint16_t x, uint16_t y)
{
  display.drawBitmap(x+23, y+12, epd_bitmap_sun, 82, 82, GxEPD_BLACK);
}

void ReTerminalE1001::moon(uint16_t x, uint16_t y)
{
  display.drawBitmap(x+26, y+12, epd_bitmap_moon, 75, 72, GxEPD_BLACK);
}

void ReTerminalE1001::wind(uint16_t x, uint16_t y)
{
  display.drawBitmap(x+13, y+54, epd_bitmap_wind, 102, 21, GxEPD_BLACK);
  display.drawBitmap(x+13, y+94, epd_bitmap_wind, 102, 21, GxEPD_BLACK);
}

void ReTerminalE1001::cloudAndWind(uint16_t x, uint16_t y)
{
  wind(x,y);
  cloud(x,y);
}

void ReTerminalE1001::fog(uint16_t x, uint16_t y)
{
  cloud(x,y);
  display.drawBitmap(x+22, y+90, epd_bitmap_fog, 84, 24, GxEPD_BLACK);
}

void ReTerminalE1001::snow(uint16_t x, uint16_t y)
{
  cloud(x,y);
  display.drawBitmap(x+39, y+92, epd_bitmap_snow, 20, 20, GxEPD_BLACK);
  display.drawBitmap(x+69, y+92, epd_bitmap_snow, 20, 20, GxEPD_BLACK);
}

void ReTerminalE1001::rain(uint16_t x, uint16_t y)
{
  display.drawBitmap(x, y, epd_bitmap_rain, 9, 19, GxEPD_BLACK);
}

void ReTerminalE1001::lightning(uint16_t x, uint16_t y)
{
  cloud(x,y);
  display.drawBitmap(x+46, y+56, epd_bitmap_lightning, 35, 57, GxEPD_BLACK);
  display.drawBitmap(x+46, y+56, epd_bitmap_lightning, 35, 28, GxEPD_WHITE);
}

void ReTerminalE1001::lightningRainy(uint16_t x, uint16_t y)
{
  lightning(x, y);
  rain(x+33,y+94);
  rain(x+74,y+94); 
}

void ReTerminalE1001::lightningRainySunny(uint16_t x, uint16_t y)
{
  littleCloud(x,y);
  display.drawBitmap(x+74, y+80, epd_bitmap_lightning1, 18, 29, GxEPD_BLACK);
  display.drawBitmap(x+74, y+80, epd_bitmap_lightning1, 18, 16, GxEPD_WHITE);
  display.drawBitmap(x+64, y+98, epd_bitmap_rain1, 5, 10, GxEPD_BLACK);
  display.drawBitmap(x+92, y+98, epd_bitmap_rain1, 5, 10, GxEPD_BLACK);
}


void ReTerminalE1001::littleCloud(uint16_t x, uint16_t y)
{
  sun(x,y);
  display.drawBitmap(x+48, y+48, epd_bitmap_little_cloud_plain, 66, 48, GxEPD_WHITE);
  display.drawBitmap(x+52, y+48, epd_bitmap_little_cloud_plain, 66, 48, GxEPD_WHITE);
  display.drawBitmap(x+50, y+50, epd_bitmap_little_cloud, 66, 48, GxEPD_BLACK);
}

void ReTerminalE1001::rain(uint16_t x, uint16_t y,uint8_t intensity)
{
  cloud(x,y);
  switch (intensity) 
  {
    case 1:
        rain(x+59,y+94);
      break;
    case 2:
        rain(x+49,y+94);
        rain(x+69,y+94);              
      break;
    case 3:
        rain(x+39,y+94);
        rain(x+59,y+94);
        rain(x+79,y+94);                  
      break;
    default:
      break;
  }  
}

void ReTerminalE1001::thermometer(uint16_t x, uint16_t y)
{
  display.drawBitmap(x, y, epd_bitmap_thermometer, 10, 32, GxEPD_BLACK);
}

void ReTerminalE1001::garageOpen(uint16_t x, uint16_t y)
{
  display.drawBitmap(x+14, y+20, epd_bitmap_garage_door_open, 100, 100, GxEPD_BLACK);
}

void ReTerminalE1001::grid()
{ 
  uint16_t y_pos=200;
  for (uint8_t x=0;x<7;x++)
  {
    display.drawLine(128*x+16,y_pos, 128*x+16, y_pos+128*2, GxEPD_BLACK); 
  }
  display.drawLine(16,y_pos,        128*6+16, y_pos,        GxEPD_BLACK); 
  display.drawLine(16,y_pos+128,    128*6+16, y_pos+128,    GxEPD_BLACK);
  display.drawLine(16,y_pos+128*2,  128*6+16, y_pos+128*2,  GxEPD_BLACK);
}

void ReTerminalE1001::icons()
{
  uint16_t y_pos=200;
  cloud(16,y_pos);
  rain(16+128,y_pos,2);
  rain(16+128*2,y_pos,3);
  fog(16+128*3,y_pos);
  cloudAndWind(16+128*4,y_pos);
  wind(16+128*5,y_pos);
  snow(16,y_pos+128);
  moon(16+128,y_pos+128);
  sun(16+128*2,y_pos+128);
  lightning(16+128*3,y_pos+128);
  lightningRainy(16+128*4,y_pos+128);
  littleCloud(16+128*5,y_pos+128);
}

void ReTerminalE1001::weather(char *weather,uint16_t x,uint16_t y)
{
  if      (strcmp(weather,"clear-night")==0)     { moon(x,y);           } 
  else if (strcmp(weather,"cloudy")==0)          { cloud(x,y);          } 
  else if (strcmp(weather,"exceptional")==0)     { sun(x,y);            }
  else if (strcmp(weather,"fog")==0)             { fog(x,y);            }
  else if (strcmp(weather,"hail")==0)            { 
    // TODO HAIL => Grêle
    rain(3,x,y);
  }
  else if (strcmp(weather,"lightning")==0)             { lightning(x,y);           }
  else if (strcmp(weather,"lightning-rainy")==0)       { lightningRainy(x,y);      }
  else if (strcmp(weather,"lightning-rainysunny")==0)  { lightningRainySunny(x,y); }  
  else if (strcmp(weather,"partlycloudy")==0)          { littleCloud(x,y);         }
  else if (strcmp(weather,"pouring")==0)               { rain(x,y,3);              }
  else if (strcmp(weather,"rainy")==0)                 { rain(x,y,2);              }
  else if (strcmp(weather,"Slight rain")==0)           { rain(x,y,1);              }
  else if (strcmp(weather,"snowy")==0)                 { snow(x,y);                }
  else if (strcmp(weather,"snowy-rainy")==0)           { 
    // TODO: add rain
    snow(x,y);
  }
  else if (strcmp(weather,"sunny")==0)           { sun(x,y);            }
  else if (strcmp(weather,"windy")==0)           { wind(x,y);           }
  else if (strcmp(weather,"windy-variant")==0)   { cloudAndWind(x,y);   }
  else if (strcmp(weather,"N/A")==0)             {                      }
  else                                           { 
    Serial1.print  ("Weather condition not recognized: ");
    Serial1.println(weather);
  }
}

void ReTerminalE1001::hibernate()
{
  display.hibernate();
}

void ReTerminalE1001::forecast(uint16_t x, uint16_t y, char state[], float temp, float temp_low)
{
  char txt_temp[10];
  char txt_temp_low[10];
  sprintf(txt_temp, "%.1f", temp);
  sprintf(txt_temp_low, "%.1f", temp_low);

  display.setRotation(0);
  display.setTextColor(GxEPD_BLACK);  
  display.setFont(&FreeMonoBold9pt7b);
  weather(state,x,y);
  display.setCursor(x+16+11*(5-strlen(txt_temp)), y+136);
  display.print(txt_temp);
  display.drawCircle(x+20+11*5, y+128, 2, GxEPD_BLACK);
  display.setCursor(x+16+11*(5-strlen(txt_temp_low)), y+152);
  display.print(txt_temp_low);  
  display.drawCircle(x+20+11*5, y+144, 2, GxEPD_BLACK);
  thermometer(x+6,y+122);
}

// === BMP Drawing Function ===
// Helper functions to read values from the BMP file
uint16_t ReTerminalE1001::read16(File &f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t ReTerminalE1001::read32(File &f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

// This function reads a BMP file and draws it to the screen.
// It includes robust error checking and a color-matching algorithm.
bool ReTerminalE1001::drawBmp(String filename, int16_t x, int16_t y) {
  y=-y;
  File bmpFile;
  int32_t bmpWidth, bmpHeight;
  uint16_t bmpDepth;
  uint32_t bmpImageoffset;
  bool flip = true;

  if ((x >= display.width()) || (y >= display.height())) return false;

  bmpFile = SD.open(filename, FILE_READ);
  if (!bmpFile) {
    Serial1.println("File not found");
    return false;
  }

  if (read16(bmpFile) != 0x4D42) {
    Serial1.println("Not a valid BMP file");
    bmpFile.close();
    return false;
  }

  read32(bmpFile);
  read32(bmpFile);
  bmpImageoffset = read32(bmpFile);
  read32(bmpFile);
  bmpWidth = read32(bmpFile);
  bmpHeight = read32(bmpFile);
  
  if (read16(bmpFile) != 1) {
    Serial1.println("Unsupported BMP format (planes)");
    bmpFile.close();
    return false;
  }
  
  bmpDepth = read16(bmpFile);
  uint32_t compression = read32(bmpFile);

  if (compression != 0) {
    if (compression == 3) {
      Serial1.println("Error: BMP file uses BI_BITFIELDS compression.");
      Serial1.println("This example only supports uncompressed BMPs.");
      Serial1.println("Please re-save the image with standard R8G8B8 (24-bit) or A8R8G8B8 (32-bit) format.");
    } else {
      Serial1.printf("Unsupported BMP format. Depth: %d, Compression: %d\n", bmpDepth, compression);
    }
    bmpFile.close();
    return false;
  }

  if (bmpDepth != 1) {
      Serial1.printf("Unsupported BMP bit depth: %d. Only 1 bit is supported.\n", bmpDepth);
      bmpFile.close();
      return false;
  }

  if (bmpHeight < 0) {
    bmpHeight = -bmpHeight;
    flip = false;
  }

  uint32_t rowSize = (ceil((float)bmpDepth*bmpWidth/32))*4;
  uint8_t sdbuffer[rowSize];
  uint8_t byte;
  uint8_t mask;
  
  uint32_t rowpos = bmpImageoffset;
  for (int16_t row = 0; row < bmpHeight; row++) {                    
    bmpFile.seek(rowpos);
    bmpFile.read(sdbuffer, rowSize);
    for (int16_t col=0; col < bmpWidth; col++)
    {
      if (col%8==0)
      {
        byte = sdbuffer[col/8];
        mask=0b10000000;
      }
      if (byte&mask)
      {
        display.drawPixel(x+col,bmpHeight-(y+row+1),GxEPD_BLACK);
      }
      
      mask=mask>>1;
    }
    rowpos+=rowSize;
  }
  bmpFile.close();
  return true;
}

void ReTerminalE1001::welcome(const char *version,bool wifi)
{
  bool img=false;
  display.setRotation(0);
  display.setFont(&FreeSerifBold24pt7b);
  display.setTextColor(GxEPD_BLACK);
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    if (!wifi)
    {
      noWifi(128*6+16-108, 20);
      img=drawBmp(img_sleep,0,0);
    }
    
    if (!img)
    {      
      display.setCursor(200, 50);
      display.print("HomeAssistant");    
      display.setCursor(200, 94);
      display.print("Viewer for E1001");
      display.setCursor(200, 138);
      display.print(version);    
      homeAssistant(20,20);
      grid();
      icons();
    }
  }
  while (display.nextPage());  
  hibernate();
}

void ReTerminalE1001::mainMenu(StationData stationData)
{
  int right_panel_x=800-128;
  int left_panel_x=128;
  char txt_tmp[10];  

  String filename = "/512_400/";
  filename.concat(stationData.getForecast(0));
  filename.concat(".bmp");

  display.firstPage();

  do
  {
    display.setRotation(0);
    display.fillScreen(GxEPD_WHITE);
    display.setFont(&FreeSerifBold24pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setFullWindow();    

    for (int i=0;i<16;i++)
    {
      display.drawLine(right_panel_x-i,0,right_panel_x-i, 479, GxEPD_BLACK); // Vertical
      display.drawLine(left_panel_x+i ,0,left_panel_x+i , 479, GxEPD_BLACK); // Vertical
    }    

    display.drawLine(right_panel_x,160,799, 160, GxEPD_BLACK);   // Horizontal
    display.drawLine(right_panel_x,320,799, 320, GxEPD_BLACK);   // Horizontal

    display.drawLine(0,160,left_panel_x, 160, GxEPD_BLACK);   // Horizontal
    display.drawLine(0,220,left_panel_x, 220, GxEPD_BLACK);   // Horizontal
    display.drawLine(0,280,left_panel_x, 280, GxEPD_BLACK);   // Horizontal
    display.drawLine(0,340,left_panel_x, 340, GxEPD_BLACK);   // Horizontal


    display.setCursor(240, 50);
    display.print(formatDateFR(stationData.getDate()));

    display.drawLine(left_panel_x,80,right_panel_x, 80, GxEPD_BLACK);   // Horizontal

    thermometer(6,166);
    sprintf(txt_tmp, "%2.1f", stationData.getTemperatureOutdoor());
    display.setCursor(30, 197);
    display.print(txt_tmp);

    thermometer(6,226);
    sprintf(txt_tmp, "%2.1f", stationData.getTemperatureIndoor());
    display.setCursor(30, 257);
    display.print(txt_tmp);

    thermometer(6,286);
    sprintf(txt_tmp, "%2.1f", stationData.getTemperatureOther());
    display.setCursor(30, 317);
    display.print(txt_tmp);    

    // Current Weather    
    forecast(0, 0, stationData.getForecast(0), stationData.getTemperature(0), stationData.getTemperatureLow(0));
    
    // Display forecast for next 3 days
    for (int i=1;i<4;i++)
    {
      forecast(right_panel_x, (i-1)*160, stationData.getForecast(i), stationData.getTemperature(i), stationData.getTemperatureLow(i));
      display.setRotation(1);
      display.setTextColor(GxEPD_WHITE);    
      display.setCursor(i*160-150, 139);      
      display.print(dayNameFR(stationData.getDate(),i));      
    } 

    display.setCursor(10,right_panel_x-4);
    display.print(lang.forecast_text);

    display.setCursor(160+10,right_panel_x-4);
    display.print(lang.temperatures_text);    

    display.setRotation(0);
    display.setTextColor(GxEPD_BLACK);   
    display.setCursor(6, 214);
    display.print(lang.outdoor_text);       
    display.setCursor(6, 274);
    display.print(lang.indoor_text);   
    display.setCursor(6, 334);
    display.print(lang.other_text); 
    if (stationData.getBatteryPercent()<5)
    {
      drawBmp(img_battery_low,left_panel_x+16,80);
    }
    else
    {
      drawBmp(filename,left_panel_x+16,80);
    }
    if (stationData.isGarageOpen())
    {
      garageOpen(0,340);
    }
  }
  while (display.nextPage());    
  
  hibernate();
}

