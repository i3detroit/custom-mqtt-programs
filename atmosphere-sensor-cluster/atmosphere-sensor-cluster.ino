#include "mqtt-wrapper.h"

#ifndef NAME
#define NAME "NEW-sensor-test"
#endif

#ifndef TOPIC
#define TOPIC "i3/program-me/NEW-sensor-test"
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

#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <Adafruit_BMP280.h>
#include <BH1750.h>
#include "DHT.h"

char* topic = TOPIC;

char buf[1024];
char topicBuf[1024];
char floatBuf[8];

Adafruit_BMP280 bmp;
BH1750 lightMeter(0x23);
DHT dht(12, DHT22);

unsigned long status = 0UL;
unsigned long statusInterval = 60000UL;

struct mqtt_wrapper_options mqtt_options;

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

void callback(char* topic, byte* payload, unsigned int length, PubSubClient *client) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
}

void connectSuccess(PubSubClient* client, char* ip) {
}

void setup() {
  Serial.begin(115200);
  mqtt_options.connectedLoop = connectedLoop;
  mqtt_options.callback = callback;
  mqtt_options.connectSuccess = connectSuccess;
  mqtt_options.ssid = WIFI_SSID;
  mqtt_options.password = WIFI_PASSWORD;
  mqtt_options.mqtt_server = MQTT_SERVER;
  mqtt_options.mqtt_port = MQTT_PORT;
  mqtt_options.host_name = NAME;
  mqtt_options.fullTopic = topic;
  mqtt_options.debug_print = true;
  setup_mqtt(&mqtt_options);

  Wire.begin(4, 5);

  if (!bmp.begin(0x76)) {
    Serial.println("Could not find BMP 280 sensor");
    while (1) {}
  }
  lightMeter.begin(BH1750_CONTINUOUS_HIGH_RES_MODE);
  dht.begin();
}

void connectedLoop(PubSubClient* client) {
  if( (long)( millis() - status ) >= 0) {
    status = millis() + statusInterval;

    sprintf(topicBuf, "tele/%s/bmp280", topic);
    ftoa(floatBuf, bmp.readTemperature(), 2);
    sprintf(buf, "{\"Temperature\":%s, \"Pressure\":", floatBuf);
    ftoa(floatBuf, bmp.readPressure(), 2);
    sprintf(buf + strlen(buf), "%s}", floatBuf);
    client->publish(topicBuf, buf);
    Serial.println("bmp280");
    Serial.println(buf);

    sprintf(topicBuf, "tele/%s/lux", topic);
    sprintf(buf, "{\"Lux\":%d}", lightMeter.readLightLevel());
    client->publish(topicBuf, buf);
    Serial.println(buf);

    sprintf(topicBuf, "tele/%s/dht22", topic);
    ftoa(floatBuf, dht.readTemperature(), 2);
    sprintf(buf, "{\"Temperature:%s, \"Humidity\":", floatBuf);
    ftoa(floatBuf, dht.readHumidity(), 2);
    sprintf(buf + strlen(buf), "%s}", floatBuf);
    Serial.println("dht22");
    Serial.println(buf);
    client->publish(topicBuf, buf);
  }
}

void loop() {
  loop_mqtt();
}

//example code for particulate sensor and volume measuring

/*
#include <SPI.h>
#include<string.h>

#define DUST_SENSOR_DIGITAL_PIN_PM10  13
#define DUST_SENSOR_DIGITAL_PIN_PM25  15

byte buff[2];
unsigned long duration;
unsigned long starttime;
unsigned long endtime;
unsigned long sampletime_ms = 30000;
unsigned long lowpulseoccupancy = 0;
float ratio = 0;
float concentration = 0;

int i=0;
void setup()
{
  Serial.begin(115200);
  pinMode(DUST_SENSOR_DIGITAL_PIN_PM10,INPUT);
  starttime = millis();
}
void loop()
{
  duration = pulseIn(DUST_SENSOR_DIGITAL_PIN_PM10, LOW);
  lowpulseoccupancy += duration;
  endtime = millis();
  if ((endtime-starttime) > sampletime_ms)
  {
    ratio = (lowpulseoccupancy-endtime+starttime + sampletime_ms)/(sampletime_ms*10.0);  // Integer percentage 0=>100
    concentration = 1.1*pow(ratio,3)-3.8*pow(ratio,2)+520*ratio+0.62; // using spec sheet curve
    Serial.print("lowpulseoccupancy:");
    Serial.print(lowpulseoccupancy);
    Serial.print("    ratio:");
    Serial.print(ratio);
    Serial.print("    DSM501A:");
    Serial.println(concentration);
    lowpulseoccupancy = 0;
    starttime = millis();
  }
}
*/

/*
const int audioSampleWindow = 5000; // Sample window width in mS (50 mS = 20Hz)
unsigned int audioSample;
unsigned int audioMaxSample = 0;
uint16_t audioMaxTime = 0;
unsigned int audioMinSample = 1024;
uint16_t audioMinTime = 0;

void setup()
{
  Serial.begin(115200);
  pinMode(DSM501A_PIN, INPUT);
}

void loop() {
  audioSample = analogRead(A0);
  uint16_t now = millis();
  if(audioSample < audioMinSample || now - audioMinTime > audioSampleWindow) {
    audioMinSample = audioSample;
    audioMinTime = now;
  } else if(audioSample > audioMaxSample || now - audioMaxTime > audioSampleWindow) {
    audioMaxSample = audioSample;
    audioMaxTime = now;
  }
  //(peak to peak * 3.3) / 1024
  double volts = ((audioMaxSample - audioMinSample) * 3.3) / 1024;  // convert to volts
  Serial.print("audio volts: ");
  Serial.println(volts);


  Serial.println();
  delay(1000);
}
*/
