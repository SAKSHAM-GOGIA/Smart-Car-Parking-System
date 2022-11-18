#include <Wire.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include <ESP8266WiFi.h>
#include <Servo.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>;
#include <WiFiUdp.h>
#include <deprecated.h>
#include <MFRC522.h>
#include <MFRC522Extended.h>
#include <require_cpp11.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <SPI.h>
#include <MFRC522.h>
const char *ssid = "Sumit's Galaxy A73 5G"; 
const char *pass = "anand123"; 
#define MQTT_SERV "io.adafruit.com"
#define MQTT_PORT 1883
#define MQTT_NAME "anandsumit31"
#define MQTT_PASS "aio_TcRU00Hb0gUIOy1xmGgNMNa7pbg5"
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800, 60000);

constexpr uint8_t RST_PIN = D3;     // Configurable, see typical pin layout above
constexpr uint8_t SS_PIN = D0;     // Configurable, see typical pin layout above

int buz = A0;
int exit_sensor = D8;
MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
MFRC522::MIFARE_Key key;
Servo myservo;
String tag;

int slot1 = D1;
int hh, mm, ss;
int pos;
int pos1;
int count = 0;
int CLOSE_ANGLE = 80; // The closing angle of the servo motor arm
int OPEN_ANGLE = 0;  

String h, m, EntryTimeSlot1, ExitTimeSlot1, EntryTimeSlot2, ExitTimeSlot2;
boolean entrysensor, exitsensor, s1;  //, s2;

boolean s1_occupied = false;

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, MQTT_SERV, MQTT_PORT, MQTT_NAME, MQTT_PASS);

// Set up the feed you're subscribing to
Adafruit_MQTT_Subscribe EntryGate = Adafruit_MQTT_Subscribe(&mqtt, MQTT_NAME "/f/EntryGate");
//
// Set up the feed you're publishing to
Adafruit_MQTT_Publish CarsParked = Adafruit_MQTT_Publish(&mqtt, MQTT_NAME "/f/CarsParked");
Adafruit_MQTT_Publish EntrySlot1 = Adafruit_MQTT_Publish(&mqtt, MQTT_NAME "/f/EntrySlot1");
Adafruit_MQTT_Publish ExitSlot1 = Adafruit_MQTT_Publish(&mqtt, MQTT_NAME "/f/ExitSlot1");

// Setup function

void setup() {
  delay(1000);
  Serial.begin(9600);
  mqtt.subscribe(&EntryGate);
  timeClient.begin();
  myservo.attach(D2);
  myservo.write(-30);
  delay(2000);
  pinMode(slot1, INPUT);
  pinMode(exit_sensor, INPUT); // ir as input

  WiFi.begin(ssid, pass); // try to connect with wifi
  Serial.print("Connecting to ");
  Serial.print(ssid); // display ssid
  while (WiFi.status() != WL_CONNECTED) 
  {
    Serial.print("."); // if not connected print this
    delay(500);
  }
  Serial.println();
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP Address is : ");
  Serial.println(WiFi.localIP()); // print local IP address
  Wire.begin(2, 0);
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522    
}

void loop() {

  MQTT_connect();
  timeClient.update();
  hh = timeClient.getHours();
  mm = timeClient.getMinutes();
  ss = timeClient.getSeconds();
  h = String(hh);
  m = String(mm);
  h + " :" + m;

  s1 = !digitalRead(slot1);

  if (!digitalRead(exit_sensor) == 1)
  {
    if (count <= 0)
    {
      count = 0;
    }
    else
    {
      count = count-1;
      Serial.println("Car Exited !!");
    }
    Serial.println("Total Cars Parked: ");
    Serial.println(count);
    delay(2000);
  }

  if ( ! rfid.PICC_IsNewCardPresent())
    return;
  if (rfid.PICC_ReadCardSerial()) {
  for (byte i = 0; i < 4; i++) {
    tag += rfid.uid.uidByte[i];
  }
  Serial.println(tag);
  tag = "";
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  }

  String content= "";
  byte letter;
  for (byte i = 0; i < rfid.uid.size; i++) 
  {
     Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
     Serial.print(rfid.uid.uidByte[i], HEX);
     content.concat(String(rfid.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(rfid.uid.uidByte[i], HEX));
  }
  Serial.println();
  Serial.print("Message : ");
  content.toUpperCase();
  if (content.substring(1) == "CC D2 0C 4A") //change here the UID of the card/cards that you want to give access
  {
    Serial.println("Authorized access");
    Serial.println();
    count = count + 1; // increment count
        if (count <= 2)
        {
            myservo.write(80);
            delay(3000);
            myservo.write(0);
        }
        else
        {
            count = 2;
        }
  }
 
  else   
  {
    Serial.println(" Access denied");
    analogWrite(buz, 255);
    delay(1000);
  }

  if (!CarsParked.publish(count))
  {
  }
    
  if (s1 == 1 && s1_occupied == false)
    {
        Serial.println("Occupied 1 ");
        EntryTimeSlot1 = h + " :" + m;
        Serial.print("EntryTimeSlot1 ");
        Serial.println(EntryTimeSlot1);
        s1_occupied = true;
        EntrySlot1.publish((char *)EntryTimeSlot1.c_str());
        
    }
    if (s1 == 0 && s1_occupied == true)
    {
        Serial.println("Available 1 ");
        ExitTimeSlot1 = h + " :" + m;
        Serial.print("ExitTimeSlot1 ");
        Serial.println(ExitTimeSlot1);
        s1_occupied = false;
        ExitSlot1.publish((char *)ExitTimeSlot1.c_str());
    }
    
  Serial.print("Total Cars Parked: ");
  Serial.println(count);
//  delay(1000);
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(5000)))
  {

      if (subscription == &EntryGate)
      {
          // Print the new value to the serial monitor
          Serial.print("Button Value: ");
          Serial.println((char *)EntryGate.lastread);

          if (!strcmp((char *)EntryGate.lastread, "ON"))
          {
              myservo.write(180);
//              delay(3000);
//              myservo.write(0);
          }

          if (!strcmp((char *)EntryGate.lastread, "OFF"))
          {
//              myservo.write(180);
//              delay(3000);
              myservo.write(0);
          }
      }
  }
  
}

void MQTT_connect()
{
    int8_t ret;

    // Stop if already connected.
    if (mqtt.connected())
    {
        return;
    }

    uint8_t retries = 3;
    while ((ret = mqtt.connect()) != 0) // connect will return 0 for connected
    {
        mqtt.disconnect();
        delay(5000); // wait 5 seconds
        retries--;
        if (retries == 0)
        {
            // basically die and wait for WDT to reset me
            while (1)
                ;
        }
    }
}
