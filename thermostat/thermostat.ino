// #include <Arduino.h>
// #include <U8g2lib.h>
//
// //U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
// U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ D4, /* clock=*/ 5, /* data=*/ 4);
//
//
// void setup() {
// //  Serial.begin(115200);
//   // --- Display ---
//   u8g2.begin();
// }
//
// // void loop() {
// //   Serial.println("Writing start");
// //   u8g2.firstPage();
// //   do {
// //     u8g2.setFont(u8g2_font_ncenB14_tr);
// //     u8g2.drawStr(0,20,"Hello World!");
// //     Serial.println("Writing Page");
// //   } while ( u8g2.nextPage() );
// //   Serial.println("Writing Done");
// // }
// void loop(void) {
//   u8g2.clearBuffer();                   // clear the internal memory
//   u8g2.setFont(u8g2_font_ncenB08_tr);   // choose a suitable font
//   u8g2.drawStr(0,10,"Hello World!");    // write something to the internal memory
//   u8g2.sendBuffer();                    // transfer internal memory to the display
//   delay(1000);
// }

#include <stdio.h>
#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <mqtt-wrapper.h>
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#ifndef NAME
#define NAME "NEW-thermostat"
#endif

#ifndef TOPIC
#define TOPIC "i3/program-me/NEW-thermostat"
#endif

#ifndef WIFI_SSID
#define WIFI_SSID "i3detroit-wpa"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "i3detroit"
#endif

#ifndef MQTT_SERVER
#define MQTT_SERVER "10.13.0.22"
#endif

#ifndef MQTT_PORT
#define MQTT_PORT 1883
#endif

char buf[1024];
char topicBuf[1024];
char floatBuf[16];
char bmpTemp[16];

struct mqtt_wrapper_options mqtt_options;


unsigned long nextStatus = 0UL;
unsigned long statusInterval = 60000UL;

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R3, /* reset=*/ D4, /* clock=*/ 5, /* data=*/ 4);
Adafruit_BMP280 bmp;

char *ftoa(char *a, double f, int precision) {
  long p[] = {
    0,10,100,1000,10000,100000,1000000,10000000,100000000  };

  char *ret = a;
  long heiltal = (long)f;
  itoa(heiltal, a, 10);
  while (*a != '\0') a++;
  *a++ = '.';
  long decimal = abs((long)((f - heiltal) * p[precision]));
  itoa(decimal, a, 10);
  return ret;
}

void readTemp() {
    ftoa(bmpTemp, bmp.readTemperature(), 2);
    ftoa(floatBuf, bmp.readPressure(), 2);
}
void reportTemp(PubSubClient *client) {
    sprintf(topicBuf, "tele/%s/bmp280", TOPIC);
    sprintf(buf, "{\"Temperature\":%s, \"Pressure\":%s}", bmpTemp, floatBuf);
    client->publish(topicBuf, buf);
    Serial.print(topicBuf);
    Serial.print(" ");
    Serial.println(buf);
}
void displayTemp() {
  u8g2.clearBuffer();
  sprintf(buf, "temp: %s", bmpTemp);
  u8g2.drawStr(0, 10, buf);
  u8g2.sendBuffer();
}

void callback(char* topic, byte* payload, unsigned int length, PubSubClient *client) {
}

void connectSuccess(PubSubClient* client, char* ip) {
  //Are subscribed to cmnd/fullTopic/+
  u8g2.clearBuffer();
  u8g2.drawStr(0,10,ip);
  u8g2.sendBuffer();
  readTemp();
  reportTemp(client);
  delay(3000);
  displayTemp();
}


void setup() {
  Serial.begin(115200);
  // --- Display ---
  u8g2.begin();
  u8g2.clearBuffer();                   // clear the internal memory
  u8g2.setFont(u8g2_font_ncenB08_tr);   // choose a suitable font
  u8g2.drawStr(0,10,"Hello World!");    // write something to the internal memory
  u8g2.sendBuffer();                    // transfer internal memory to the display
  Serial.println("Writing Done");

//  display.init();
//  display.flipScreenVertically();
//  display.setContrast(255);
//
//  display.setTextAlignment(TEXT_ALIGN_LEFT);
//  display.setFont(Monospaced_plain_16);
//  display.drawString(0,0, "Hello World:\"',");
//  display.drawString(0,20, "0123456789{}<>?.");
//  display.drawString(0,40, "~!@#$%^&*()_+[]/");
//  display.display();

  // --- MQTT ---
  mqtt_options.connectedLoop = connectedLoop;
  mqtt_options.callback = callback;
  mqtt_options.connectSuccess = connectSuccess;
  mqtt_options.ssid = WIFI_SSID;
  mqtt_options.password = WIFI_PASSWORD;
  mqtt_options.mqtt_server = MQTT_SERVER;
  mqtt_options.mqtt_port = MQTT_PORT;
  mqtt_options.host_name = NAME;
  mqtt_options.fullTopic = TOPIC;
  mqtt_options.debug_print = true;
  setup_mqtt(&mqtt_options);

  // --- Sensors ---
  if (!bmp.begin(0x76)) {
    Serial.println("Could not find BMP 280 sensor");
    while (1) {}
  }
}

void connectedLoop(PubSubClient* client) {
  if( (long)( millis() - nextStatus ) >= 0) {
    reportTemp(client);
  }
}

void loop() {
  loop_mqtt();
  if( (long)( millis() - nextStatus ) >= 0) {
    nextStatus = millis() + statusInterval;
    readTemp();
    displayTemp();
  }
}
