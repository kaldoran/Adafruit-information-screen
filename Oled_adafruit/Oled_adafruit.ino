#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>

#include <ESP8266WiFi.h>
#include <time.h>

#include "meteo.h"
#include "Wifi.h"

Adafruit_SSD1306 display(0);

WiFiClient myclient;
StaticJsonBuffer<200> jsonBuffer;

#define UPDATE_INTERVAL 60 // Update every 5 min 

char time_b[10];
time_t now = 0;
time_t next_update = 0;

// https://query.yahooapis.com/v1/public/yql?q=select%20woeid%20from%20geo.places(1)%20where%20text=%22Firminy%22&format=json > WOEID
// https://query.yahooapis.com/v1/public/yql?q=select%20item.forecast.high,%20item.forecast.low,%20item.condition.temp,%20item.forecast.text%20from%20weather.forecast%20where%20woeid%20=590999%20and%20u=%27c%27%20limit%201&format=json
void setup()   {     
  Serial.begin(115200);
  WiFi.begin(SSID, PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  configTime(3600, 0, "pool.ntp.org", "time.nist.gov"); // Offset normal - Offset été
   
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32
  display.display();
  delay(2000);
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);  

  now = time(nullptr);
  next_update = now + UPDATE_INTERVAL;
}

void my_strftime(char *_buffer, int size, const char* const _format, struct tm* _tm) {
  const char *f = _format;
  int pos = 0;

  const char* day[9] = {"Dimanche", "Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi"}; 

  memset(_buffer, 0, size);
  
  while ( *f != '\0' ) {      
    if ( *f != '%' ) _buffer[pos++] = *f;
    else {
      ++f;
      switch(*f) {
        case 'A':
          pos += sprintf(_buffer + pos, "%s", day[_tm->tm_wday]);
          break;
        case 'd':
          pos += sprintf(_buffer + pos, "%02d", _tm->tm_mday);
          break;
        case 'm':
          pos += sprintf(_buffer + pos, "%02d", _tm->tm_mon + 1);
          break;
        case 'y':
          pos += sprintf(_buffer + pos, "%02d", _tm->tm_year % 100);
          break;
        case 'H':
          pos += sprintf(_buffer + pos, "%02d", _tm->tm_hour);
          break;
        case 'M': 
          pos += sprintf(_buffer + pos, "%02d", _tm->tm_min);
          break;
        default:
          _buffer[pos++] = *f;
          break;
      }
    }
    
    ++f;
  }

  _buffer[pos] = '\0';
}

void dateTime() {
  Serial.print(now);
  struct tm *local = localtime(&now);
  
  display.setCursor(57,4);
  display.setTextSize(2);
  my_strftime(time_b, 10, "%H:%M", local);
  display.print(time_b);
  
  display.setTextSize(1);
 
  display.setCursor(25, 20);
  display.print(WiFi.localIP());
 
  display.setCursor(0,2);
  my_strftime(time_b, 10, "%A", local);
  display.println(time_b);
 
  my_strftime(time_b, 10, "%d/%m/%y", local);
  display.print(time_b);
  display.display();
}

void emptyScreen() {
  display.setCursor(0,0);
  display.println("Aucune idee"); 
}

void updateData() {
  display.setCursor(25,15);
  display.println("Updating Data");

  display.drawLine(17, 28, 110, 28, WHITE);
  display.display();

  // Doing some stuff
  delay(10000);
  
  display.clearDisplay();
}


typedef void (* Function) ();

Function screen[] = {dateTime, emptyScreen};
int total_screen = sizeof(screen) / sizeof(screen[0]);


void loop() {
  
  for ( int curr_screen = 0; curr_screen < total_screen; curr_screen++) {
    /* Compare with last value of now, just to not slow proces with 2 now */    
    if ( now >= next_update ) { 
      updateData();
      next_update = now + UPDATE_INTERVAL;
    }

    now = time(nullptr);
    screen[curr_screen]();
    waiting(curr_screen + 1, total_screen);   
    delay(200);
  }
}
 
void waiting(uint8_t screen, uint8_t out_of) {
  uint8_t marging = 3;
  uint8_t screenHeight = display.height() - 1;
  uint8_t screenWidth = display.width();
  uint8_t pos = (screenHeight - marging) / out_of;
 
  for ( uint8_t i = 1; i < out_of + 1; i++) {
    if ( i == screen )
      display.fillCircle(screenWidth - 5, pos * i - marging, 2, WHITE);
    else
      display.drawCircle(screenWidth - 5, pos * i - marging, 2, WHITE);
  }
 
  for (uint8_t y=0; y < screenWidth ; y++) {  
     display.drawPixel(y, screenHeight, WHITE);
     delay(1);
     display.display();
  }
  
  delay(350);
  display.clearDisplay();
}
