#include <M5Stack.h>

#include <SPI.h>
#include "RF24.h"
#include "nRF24L01.h"
#include <RF24Network.h>
#include "printf.h"
#include <EEPROM.h>
#ifdef ESP8266
#include <ESP8266WiFi.h>
#else //ESP32
#include <WiFi.h>
#endif
#include <ModbusIP_ESP8266.h>

struct TModbus {
	unsigned int Error;
	unsigned int ErrorRate;
	unsigned int Temp;
	unsigned int Hum;
	unsigned int Volt;
	unsigned int Bat;
	unsigned int ErrorsCount;
	unsigned int RadioChangesCount;
	unsigned int RadioPower;
};

struct payload_t
{
  unsigned int Pos;
  unsigned int DataLength;
  unsigned int SendSeconds;
  TModbus Data;
  unsigned int CRC;
  unsigned int TMP[3];
};

payload_t Payload;

// Modbus Registers Offsets
const int TEST_HREG = 50;

//ModbusIP object
ModbusIP mb;

byte networkChannel = 0;
RF24 radio(16, 17);
RF24Network network(radio);
const short num_channels = 128;
short values[num_channels];

// the setup routine runs once when M5Stack starts up
void setup()
{

  Serial.begin(115200);
  printf_begin();

  // initialize the M5Stack object
  M5.begin();

  /*
    Power chip connected to gpio21, gpio22, I2C device
    Set battery charging voltage and current
    If used battery, please call this function in your project
  */
  M5.Power.begin();

  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);

  M5.Lcd.setCursor(10, 10);
  M5.Lcd.printf("Connecting to wifi...");
  WiFi.begin("iSOWifi", "1235813213455");
/*
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }*/
  M5.Lcd.setCursor(10, 40);
  IPAddress ip = WiFi.localIP();
  M5.Lcd.printf("Connected! %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

  mb.slave();
  for (int i = 0; i < 20; i++)
  {
    mb.addHreg(TEST_HREG + i, i);
  }

  if (!EEPROM.begin(10)) {
    Serial.println("failed to initialise EEPROM");
  }
  networkChannel = EEPROM.read(0) + 1;
  if (networkChannel > 127) {
    networkChannel = 0;
  }
  delay(100);
  EEPROM.write(0, networkChannel);
  EEPROM.commit();

  // text print
  M5.Lcd.setCursor(10, 70);
  M5.Lcd.printf("Channel: %d", networkChannel);

  M5.Lcd.setCursor(10, 130);


  radio.begin();
  network.begin(networkChannel, 0);
  //radio.setAutoAck(false);

  // Get into standby mode
//  radio.startListening();
//  radio.stopListening();

  // Print out header, high then low digit
  int i = 0;
  while (i < num_channels)
  {
    printf("%x", i >> 4);
    ++i;
  }
  printf("\n\r");
  i = 0;
  while (i < num_channels)
  {
    printf("%x", i & 0xf);
    ++i;
  }
  printf("\n\r");

  //M5.Speaker.beep(); //beep
  M5.Speaker.mute();
  M5.Lcd.setBrightness(80);
  dacWrite(25, 0);
}
const short num_reps = 100;

// the loop routine runs over and over again forever
void loop()
{
  /*
  // Clear measurement values
  memset(values, 0, num_channels);

  // Scan all channels num_reps times
  int rep_counter = num_reps;
  while (rep_counter--)
  {
    int i = num_channels;
    while (i--)
    {
      // Select this channel
      radio.setChannel(i);

      // Listen for a little
      radio.startListening();
      delayMicroseconds(128);
      radio.stopListening();

      // Did we get a carrier?
      if (radio.testCarrier())
        ++values[i];
    }
  }

  // Print out channel measurements, clamped to a single hex digit
  int i = 0;
  while (i < num_channels)
  {
    printf("%x", min(0xf, values[i] & 0xf));
    ++i;
  }
  printf("\n\r");

  M5.update();
  */

  network.update();
  while (network.available())
  { // Is there anything ready for us?

    RF24NetworkHeader header; // If so, grab it and print it out
    payload_t payload;
    network.read(header, &payload, sizeof(payload));
    Serial.print("Received packet #");
    Serial.print(payload.Pos);
    Serial.print(" at ");
    Serial.println(payload.Data.Hum);
  }

  mb.task();
  delay(2);
  //Serial.print(".");
}