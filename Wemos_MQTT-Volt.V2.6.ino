//-------Wemos---V2.6------------

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>

String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

int volt[10];
int amp[10];
float value[10];
const int numberOfPieces = 10;
int pieces[numberOfPieces];
int lastIndex = 0;
int counter = 0;

//String SENSOR_ID = "9708b900-1f15-11e6-a724-a34111381c19";
String SENSOR_ID = "9708b901-1f15-11e6-a724-a34111381c19";
//String SENSOR_ID = "9708b902-1f15-11e6-a724-a34111381c19";

String NETWORK_ID = "55af3b51-1df1-11e6-a17d-bd3a9d3e91df";

#define D0 16  // USER LED Wake
#define D1 5   // Switch_Pin 
#define D2 4   // LED1
#define D3 0   // FLASH
#define D4 2   // TXD1 , DHT22 ****
#define D5 14  // LED2
#define D6 12  // LED3
#define D7 13  // RXD2
#define D8 15  // TXD2,LED4 ****
#define D9 3   // RXD0
#define D10 1  // TXD0,LED5
#define SD2 9
#define SD3 10
#define Switch_Pin  D1     // the number of the pushbutton pin

byte mac[6];

const char* ssid      = "potiya";
const char* password  = "0819669737";

const char *Maker_Event = "Your EVENT Name";
const char *MQTTBroker = "greatodev.com";
const char *MQTTuser = "test";
const char *MQTTpassword = "test";
uint16_t port =   1883;

const char *Topic = "/sensors/all/set";
const char *Topicid = "response";

void connectWifi();
void reconnectWifiIfLinkDown();

WiFiClient wclient;
PubSubClient mqtt(wclient, MQTTBroker, port);

//Time starmp set
unsigned int localPort = 2390;      // local port to listen for UDP packets
unsigned long timestamp ;
/* Don't hardwire the IP address or we won't get the benefits of the pool.
    Lookup the IP address for the host name instead */

IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "th.pool.ntp.org";
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

void callback(const MQTT::Publish& pub) {
  // handle message arrived
  Serial.println(pub.payload_string());
  delay(500);
  String payload = pub.payload_string();
  Serial.print(payload);
  if (String(pub.topic()) == (String (ESP.getChipId()))) {
  }
}

void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);

  WiFi.mode(WIFI_STA);
  delay(10);
  connectWifi();
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Starting UDP");
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());
  WiFi.printDiag(Serial);

  //----------------------เปลี่ยนข้อมูล ESP8266-----------------------------------------
  mqtt.set_callback(callback);  //Set Callback function
  if (mqtt.connect(MQTT::Connect(String (ESP.getChipId())).set_auth(MQTTuser, MQTTpassword))) {
  }
  ArduinoOTA.onStart([]()
  {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]()
  {
    Serial.println("End");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
  {
    Serial.printf("Progress: %u%%\n", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error)
  {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
    ESP.restart();
  });
  ArduinoOTA.begin(); // setup the OTA server
}

void loop() {

  delay(3000);
  serialEvent();
  ArduinoOTA.handle();
}

void NPTService() {
  WiFi.hostByName(ntpServerName, timeServerIP);

  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  delay(1000);
  int cb = udp.parsePacket();
  if (!cb) {
    Serial.println("no packet yet");
  }
  else {
    Serial.print("packet received, length=");
    Serial.println(cb);
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    Serial.print("Seconds since Jan 1 1900 = " );
    Serial.println(secsSince1900);
    // now convert NTP time into everyday time:
    Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;
    // print Unix time:
    Serial.println(epoch);
    settimestamp(epoch);
    // print the hour, minute and second:
    Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    Serial.print(((epoch  % 86400L) / 3600) + 7); // print the hour (86400 equals secs per day)
    Serial.print(':');
    if ( ((epoch % 3600) / 60) < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
    Serial.print(':');
    if ( (epoch % 60) < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.println(epoch % 60); // print the second
  }
}

void MQTTService() {
  NPTService();

  String message = "";
  message += "{";
  message +=    "\"time\":";
  message +=     gettimestamp();
  message +=  ",";
  message +=    "\"sensorId\":\"";
  message +=     SENSOR_ID;
  message +=  "\",";
  message +=    "\"networkId\":\"";
  message +=     NETWORK_ID;
  message +=  "\",";
  message +=    "\"wifiId\":";
  message +=    String (ESP.getChipId());   
  message +=    ",";
  message +=    "\"type\":1,";
  message +=    "\"sensors\":[";
  message +=      "{";
  message +=        "\"type\":\"amp\",";
  message +=        "\"value\":\"";
  message +=        value[0];
  message +=        "\"";
  message +=        ",\"measurement_id\":\"amp1\"";
  message +=      "},";

  message +=      "{";
  message +=        "\"type\":\"amp\",";
  message +=        "\"value\":\"";
  message +=        value[1];
  message +=        "\"";
  message +=        ",\"measurement_id\":\"amp2\"";
  message +=      "},";

  message +=      "{";
  message +=        "\"type\":\"amp\",";
  message +=        "\"value\":\"";
  message +=        value[2];
  message +=        "\"";
  message +=        ",\"measurement_id\":\"amp3\"";
  message +=      "},";

  message +=      "{";
  message +=        "\"type\":\"volt\",";
  message +=        "\"value\":\"";
  message +=        value[3];
  message +=        "\"";
  message +=        ",\"measurement_id\":\"volt1\"";
  message +=      "},";

  message +=      "{";
  message +=        "\"type\":\"volt\",";
  message +=        "\"value\":\"";
  message +=        value[4];
  message +=        "\"";
  message +=        ",\"measurement_id\":\"volt2\"";
  message +=      "},";

  message +=      "{";
  message +=        "\"type\":\"volt\",";
  message +=        "\"value\":\"";
  message +=        value[5];
  message +=        "\"";
  message +=        ",\"measurement_id\":\"volt3\"";
  message +=      "}";
  message +=    "]";
  message +=  "}";

  Serial.println(message);

  if (gettimestamp() != 0) {
    //Serial.println(SenttoMQTT);
    mqtt.publish(Topic, message);  //publish
  } else {
    Serial.println("GetTime Error");
  }
  //----------------------เปลี่ยนข้อมูล ESP8266-----------------------------------------
  mqtt.loop();
  mqtt.set_callback(callback);  //Set Callback function
  if (mqtt.connect(MQTT::Connect(String (ESP.getChipId())).set_auth(MQTTuser, MQTTpassword))) {
  }
}
//-----------------------------------------------------------------------------------

void connectWifi() {
  Serial.println();
  Serial.println();
  Serial.println("Connecting to ");
  Serial.print(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    //digitalWrite(ledPin1, LOW);
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC Address is :");
  WiFi.macAddress(mac);
  Serial.print("MAC: ");
  Serial.print(mac[5], HEX);
  Serial.print(":");
  Serial.print(mac[4], HEX);
  Serial.print(":");
  Serial.print(mac[3], HEX);
  Serial.print(":");
  Serial.print(mac[2], HEX);
  Serial.print(":");
  Serial.print(mac[1], HEX);
  Serial.print(":");
  Serial.println(mac[0], HEX);
}

void reconnectWifiIfLinkDown() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WIFI DISCONNECTED");
    //digitalWrite(ledPin1, LOW);
    connectWifi();
  }
}

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    inputString += inChar;
    if (inChar == '\n') {
      stringComplete = true;
      if (inChar > 0) {
        Getdata();
      }

    }
  }
}

void Getdata()
{
  if (stringComplete)
  {
    delay(1000);
    Serial.print(">");
    Serial.println(inputString);
    for (int i = 0; i < inputString.length(); i++) {
      if (inputString.substring(i, i + 1) == ",") {
        value[counter] = inputString.substring(lastIndex, i).toFloat();
        lastIndex = i + 1;
        counter++;
      }
      if (i == inputString.length() - 1) {
        value[counter] = inputString.substring(lastIndex, i).toFloat();
      }
    }
    MQTTService();
    inputString = "";
    counter = 0;
    lastIndex = 0;
  }
  delay(5000);
}

unsigned long sendNTPpacket(IPAddress& address)
{
  Serial.println("sending NTP packet...");
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

void  settimestamp(long Stamp) {
  timestamp = Stamp;
}
long gettimestamp() {
  return timestamp;
}

//////////////////////////////////////////////////////////////

