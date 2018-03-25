#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>

#include <ESP8266WiFi.h>
#include <time.h>

#include "meteo.h"
#include "Wifi.h"

/* For the update */
HTTPClient http;

/* Weather */
int high = 0;
int low = 0;
int temp = 0;

/* Exchange */
float EUR_USD = 0;
float USD_EUR = 0;

/* Knowledge */
String knowledge = " ";

/* Util */
Adafruit_SSD1306 display(0);

WiFiClient myclient;

time_t now = 0;
time_t next_update = 0;

typedef void (* Function) ();

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

/* Util */
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

/* Screen */
char time_b[10];

void dateTime() {
  int date_size = 0;
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

void weather() {
  int pos = 60;
  if ( temp >= 10 ) pos = 70;
  
  display.setCursor(35,3);
  display.setTextSize(3);
  display.print(temp);
  
  display.setTextSize(1);
  display.setCursor(pos, 3);
  
  display.print(high);
  display.print((char)247);
  display.println("C");

  display.setCursor(pos, 15);
  display.print(low);
  display.print((char)247);
  display.println("C");

  display.setCursor(10, 20);  
  display.setFont(&Weathericon);
  display.write(66);
  display.setFont();

  display.display();
}

void trade() {
  display.setCursor(0,0);
  display.println("");
  display.print("  1 EUR = ");
  display.print(EUR_USD);
  display.println(" USD");

  display.print("  1 USD = ");
  display.print(USD_EUR);
  display.println(" EUR");
}

/* Useless - Just show multi screen handle */
void uselessKnowledge() {
  display.setCursor(0,0);
  display.println(knowledge);
}

/* Updating weather adn some other data */

void updateWeather(String _data) {
  const size_t capacity = 4*JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(4);
  DynamicJsonBuffer jsonBuffer(capacity);
  JsonObject& root = jsonBuffer.parseObject(_data);
  
  if (!root.success()) {
    Serial.println(F("Parsing failed!"));
    return;
  }

  temp = root["query"]["results"]["channel"]["item"]["condition"]["temp"];
  high = root["query"]["results"]["channel"]["item"]["forecast"]["high"];
  low  = root["query"]["results"]["channel"]["item"]["forecast"]["low"];
}

void updateExchange(String _data) {
  const size_t capacity = JSON_OBJECT_SIZE(2);
  DynamicJsonBuffer jsonBuffer(capacity);
  JsonObject& root = jsonBuffer.parseObject(_data);

  if (!root.success()) {
    Serial.println(F("Parsing failed!"));
    return;
  }

  EUR_USD = root["EUR_USD"];
  USD_EUR = root["USD_EUR"];
}

void updateKnowledge(String _data) {
  Serial.print(_data);
  knowledge = _data;
}

void updateData() {   
  display.setCursor(25,15);
  display.println("Updating Data");

  display.drawLine(17, 28, 110, 28, WHITE);
  display.display();

  http.begin("http://query.yahooapis.com/v1/public/yql?q=select%20item.forecast.high,%20item.forecast.low,%20item.condition.temp,%20item.forecast.text%20from%20weather.forecast%20where%20woeid%20=590999%20and%20u=%27c%27%20limit%201&format=json");  //Specify request destination
  if (http.GET() == 200) updateWeather(http.getString());

  http.begin("http://free.currencyconverterapi.com/api/v5/convert?q=USD_EUR,EUR_USD&compact=ultra");
  if (http.GET() == 200) updateExchange(http.getString());
  
  display.clearDisplay();
}

/* REAL PROGRAM - Need splitting function up their */
#define UPDATE_INTERVAL 5 * 60 // Update every 5 min 

Function screen[] = {dateTime, weather, uselessKnowledge, trade};
int total_screen = sizeof(screen) / sizeof(screen[0]);

void setup() {     
  Serial.begin(115200);
  WiFi.begin(SSID, PASS);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  configTime(3600, 7200, "pool.ntp.org", "time.nist.gov"); // Offset normal - Offset été

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32
  display.display();
  delay(2000);
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);  

  /* Start by setting parameter in order to update first */
  now = time(nullptr);
  next_update = 0;
}

void loop() {
  if ( now >= next_update ) { 
    updateData();
    next_update = now + UPDATE_INTERVAL;
  }

  // For this one - Update each screen 
  http.begin("http://kaldoran.fr/rdm.php");
  if (http.GET() == 200 ) updateKnowledge(http.getString());
    
  for ( int curr_screen = 0; curr_screen < total_screen; curr_screen++) {
    /* Compare with last value of now, just to not slow proces with 2 now */    
    now = time(nullptr);
    screen[curr_screen]();
    waiting(curr_screen + 1, total_screen);   
  }
}
