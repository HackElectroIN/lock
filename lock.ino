/*
 * LOCK
 * Maxime MOREILLON
 * 
 * Board type: Wemos D1 Mini
 * 
 */

// Libraries
#include <ESP8266WiFi.h> // Main ESP8266 library
#include <ArduinoOTA.h> // OTA update library
#include <WiFiUdp.h> // Required for OTA
#include <AsyncMqttClient.h> // MQTT library
#include <Ticker.h> // Used when reconecting MQTT upon wifi drop
#include <ESP8266WebServer.h> // Web server library
#include <Servo.h> // Servo library

#include "credentials.h"; // Credentials and settings

// Wifi
Ticker wifi_reconnect_timer;
WiFiEventHandler wifi_connect_handler;
WiFiEventHandler wifi_disconnect_handler;

// OTA
#define HOSTNAME "lock"

// MQTT
AsyncMqttClient MQTT_client;
Ticker MQTT_reconnect_timer;
#define MQTT_BROKER_ADDRESS IPAddress(192, 168, 1, 2)
#define MQTT_PORT 1883
#define MQTT_COMMAND_TOPIC "lock/command"
#define MQTT_STATUS_TOPIC "lock/status"
#define MQTT_QOS 1
#define MQTT_RETAIN true

// Web server settings
#define WWW_PORT 80

// Pin mapping
#define SERVO_PIN D2
#define UNLOCK_SWITCH_PIN D6
#define LOCK_SWITCH_PIN D5
#define UNLOCK_LED_PIN D7
#define LOCK_LED_PIN D8

// Web server
ESP8266WebServer www_server(WWW_PORT);

// Servo
Servo servo;
#define SERVO_DELAY 6
boolean servo_unlock_request, servo_lock_request;

char* lock_status = "UNKNOWN";

void setup() {
  
  // Mandatory initial delay
  delay(10);
  
  // Serial init
  Serial.begin(115200);
  Serial.println(); // Separate serial stream from initial gibberish
  Serial.println(F(__FILE__ " " __DATE__ " " __TIME__));

  // IO init
  pinMode(UNLOCK_SWITCH_PIN,INPUT_PULLUP);
  pinMode(LOCK_SWITCH_PIN,INPUT_PULLUP);
  pinMode(UNLOCK_LED_PIN,OUTPUT);
  pinMode(LOCK_LED_PIN,OUTPUT);

  servo_setup();
  wifi_setup();
  web_server_setup();
  MQTT_setup();
  OTA_setup();
  
  wifi_connect();
}


void loop() {
  
  ArduinoOTA.handle();
  www_server.handleClient();

  // Servo management
  if(servo_unlock_request){
    servo_unlock();
    servo_unlock_request = false;
  }
  if(servo_lock_request){
    servo_lock();
    servo_lock_request = false;
  }
  
  // Limit switches management for lock state acknowledgement
  if(digitalRead(UNLOCK_SWITCH_PIN) == LOW && strcmp(lock_status,"UNLOCKED")!=0) {
    lock_status = "UNLOCKED";
    Serial.println("Unlocked limit switch hit");
    MQTT_client.publish(MQTT_STATUS_TOPIC, MQTT_QOS, MQTT_RETAIN, lock_status);
    digitalWrite(UNLOCK_LED_PIN,HIGH);
    digitalWrite(LOCK_LED_PIN,LOW);
  }
  
  if(digitalRead(LOCK_SWITCH_PIN) == LOW && strcmp(lock_status,"LOCKED")!=0) {
    lock_status = "LOCKED";
    Serial.println("Locked limit switch hit");
    MQTT_client.publish(MQTT_STATUS_TOPIC, MQTT_QOS, MQTT_RETAIN, lock_status);
    digitalWrite(UNLOCK_LED_PIN,LOW);
    digitalWrite(LOCK_LED_PIN,HIGH);
  }
}
