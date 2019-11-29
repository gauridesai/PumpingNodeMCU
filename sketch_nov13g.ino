#include <FS.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <time.h>
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

IPAddress remote_addr;
  
// Insert your FQDN of your MQTT Broker
#define MQTT_SERVER "a1jdp1fi6njxnj-ats.iot.us-east-1.amazonaws.com"
const char* mqtt_server = MQTT_SERVER;

// WiFi Credentials
const char* ssid = "pumphotspot";
const char* password = "gauri1234";

boolean connectionStatus = false;
boolean mqtt_connected = false;

#define LED_BUILTIN 2

// Topic
char* topic;
char* in_topic;

long lastMsg = 0;
int test_para = 2000;
unsigned long startMills;

int r1=16;
int r2=5;
int r3=4;
int r4=0;// GPIO5 or for NodeMCU you can directly write D1

void led1TurnOn();
void led1TurnOff();
void led2TurnOn();
void led2TurnOff();
void led3TurnOn();
void led3TurnOff();
void led4TurnOn();
void led4TurnOff();

void callback(char* in_topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(in_topic);
  Serial.print("] ");
  String jsonString = "";
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    jsonString.concat((char)payload[i]);
  }
  
  Serial.println();  
  Serial.println(jsonString);
  
  if(jsonString.indexOf("led1_on") > 0) {
    Serial.println("Turning on LED 1");    
    led1TurnOn();
  }
    
  if(jsonString.indexOf("led1_off") > 0) {
    Serial.println("Turning off LED 1");    
    led1TurnOff();
  }  
  
  if(jsonString.indexOf("led2_on") > 0) {
    Serial.println("Turning on LED 2");        
    led2TurnOn();
  }   
  
  if(jsonString.indexOf("led2_off") > 0) {
    Serial.println("Turning off LED 2");        
    led2TurnOff();
  }  
  
  if(jsonString.indexOf("led3_on") > 0) {
    Serial.println("Turning on LED 3");        
    led3TurnOn();
  }  
  
  if(jsonString.indexOf("led3_off") > 0) {
    Serial.println("Turning off LED 3");            
    led3TurnOff();
  }  
  if(jsonString.indexOf("led4_on") > 0) {
    Serial.println("Turning on LED 4");              
    led4TurnOn();
  }
    
  if(jsonString.indexOf("led4_off") > 0) {
    Serial.println("Turning off LED 4");                  
    led4TurnOff();
  }

  Serial.println();

}


WiFiClientSecure wifiClient;
PubSubClient client(mqtt_server, 8883, callback, wifiClient);

// Load Certificates
void loadcerts() {

  if (!SPIFFS.begin()) {
   Serial.println("Failed to mount file system");
   return;
 }

 // Load client certificate file from SPIFFS
 File cert = SPIFFS.open("/b3f4f0bea6-certificate.pem.crt", "r"); //replace esp.der with your uploaded file name
 if (!cert) {
   Serial.println("Failed to open cert file");
 }
 else
   Serial.println("Success to open cert file");

 delay(1000);

 // Set client certificate
 if (wifiClient.loadCertificate(cert))
   Serial.println("cert loaded");
 else
   Serial.println("cert not loaded");

 // Load client private key file from SPIFFS
 File private_key = SPIFFS.open("/b3f4f0bea6-private.pem.key", "r"); //replace espkey.der with your uploaded file name
 if (!private_key) {
   Serial.println("Failed to open private cert file");
 }
 else
   Serial.println("Success to open private cert file");

 delay(1000);

 // Set client private key
 if (wifiClient.loadPrivateKey(private_key))
   Serial.println("private key loaded");
 else
   Serial.println("private key not loaded");


 // Load CA file from SPIFFS in DER
 File ca_der = SPIFFS.open("/root-ca.der", "r"); //replace ca.der with your uploaded file name
 if (!ca_der) {
   Serial.println("Failed to open ca_der ");
 }
 else
  Serial.println("Success to open ca_der");
  delay(1000);

  // Set server CA file
   if(wifiClient.loadCACert(ca_der))
   Serial.println("ca_der loaded");
   else
   Serial.println("ca_der failed");

}

void getTime(){
  // Synchronize time useing SNTP. This is necessary to verify that
  // the TLS certificates offered by the server are currently valid.
  Serial.print("Setting time using SNTP");
  configTime(8 * 3600, 0, "de.pool.ntp.org");
  time_t now = time(nullptr);
  while (now < 1000) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
 }

boolean reconnect()
{
  WiFi.hostByName(mqtt_server, remote_addr);  
  client.setServer(remote_addr, 8883);
  Serial.print("Set IP Address: ");          
  Serial.println(remote_addr);          
  if (!client.connected()) {
 //  try {    
    connectionStatus = client.connect("NodeMCU", "gaurid380", "g@uri1234");
  // } catch (std::bad_alloc& ba) {
     //   Serial.println("bad allocation error caught");        
   //}
    if (connectionStatus) {
        Serial.println("===> mqtt connected");        
        mqtt_connected = true;
        return true;
    } else {
        Serial.print("---> mqtt failed, rc=");
        Serial.print(client.state());        
        mqtt_connected = false;
        return false;
    }
  }
}

void wifi_connect()
{
  if (WiFi.status() != WL_CONNECTED) {
    // WIFI
      Serial.println();
      Serial.print("===> WIFI ---> Connecting to ");
      Serial.println(ssid);

      delay(10);
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid, password);

      int Attempt = 0;
      while (WiFi.status() != WL_CONNECTED) {
        Serial.print(". ");
        Serial.print(Attempt);

        delay(100);
        Attempt++;
        if (Attempt == 150) {
           Serial.println();
           Serial.println("-----> Could not connect to WIFI");

           ESP.restart();
           delay(200);
         }
      }
      Serial.println();
      Serial.print("===> WiFi connected");
      Serial.print(" ------> IP address: ");
      Serial.println(WiFi.localIP());
     }    
 }

void setup()
{

  WiFi.disconnect();
  
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  digitalWrite(LED_BUILTIN, HIGH);
  
  pinMode(r1, OUTPUT);
  pinMode(r2, OUTPUT);
  pinMode(r3, OUTPUT);
  pinMode(r4, OUTPUT);
  
  digitalWrite(r1, HIGH);
  digitalWrite(r2, HIGH);
  digitalWrite(r3, HIGH);
  digitalWrite(r4, HIGH);
  
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  startMills = millis();

//  WiFiManager wifiManager;
//  wifiManager.setTimeout(180);
//  
//  if(!wifiManager.autoConnect("apname", "appassword")) {
//    Serial.println("failed to connect and hit timeout");
//    delay(3000);
//    ESP.reset();
//    delay(5000);
//  }   

// Serial.println("connected...yeey :)");
  
  wifi_connect();
  delay(500);
  getTime();
  delay(500);
  loadcerts();
  delay(200);  
  
}

void loop()
{
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqtt_connected) {
        reconnect();
    } else {
      long now = millis();
      if (now - lastMsg > test_para) {
        lastMsg = now;
        String payload = "{\"startMills\":";
        payload += (millis() - startMills);
        payload += "}";
        topic = "$aws/things/NodeMCU/shadow/update";
        //sendmqttMsg(topic, payload);
        in_topic="NodeMCU/inTopic";
        client.subscribe(in_topic);
      }

    }
  } else {
    Serial.println("WiFi could not be connected !!");        
    wifi_connect();
  }

  client.loop();

}

void sendmqttMsg(char* topictosend, String payload)
{

  if (mqtt_connected) {
      Serial.print("Sending payload: ");
      Serial.print(payload);

    unsigned int msg_length = payload.length();

      Serial.print(" length: ");
      Serial.println(msg_length);

    byte* p = (byte*)malloc(msg_length);
    memcpy(p, (char*) payload.c_str(), msg_length);

    int publishStatus = client.publish(topictosend, (char*) payload.c_str());
    Serial.print("Publish Status: ");
    Serial.print(publishStatus);

  }
}


void led1TurnOn(){

  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(r1, LOW);  
}

void led1TurnOff(){

  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(r1, HIGH);
}

void led2TurnOn(){

  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(r2, LOW);  
}

void led2TurnOff(){

  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(r2, HIGH);
}

void led3TurnOn(){

  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(r3, LOW);  
}

void led3TurnOff(){

  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(r3, HIGH);
}

void led4TurnOn(){

  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(r4, LOW);  
}

void led4TurnOff(){

  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(r4, HIGH);
}
