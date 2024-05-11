#include <ESP8266WiFi.h>
#include <Arduino.h>
#include <FirebaseClient.h>

#if defined(ESP32) || defined(ESP8266) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFiClientSecure.h>
WiFiClientSecure ssl_client;
#elif defined(ARDUINO_ARCH_SAMD) || defined(ARDUINO_UNOWIFIR4) || defined(ARDUINO_GIGA) || defined(ARDUINO_PORTENTA_C33) || defined(ARDUINO_NANO_RP2040_CONNECT)
#include <WiFiSSLClient.h>
WiFiSSLClient ssl_client;
#endif

const char* wifi_ssid     = "";
const char* wifi_password = "";

const char* apiKey = "";
const char* dbUrl = "";
const char* userEmail = "";
const char* userPassword = "";

DefaultNetwork network;
UserAuth user_auth(apiKey, userEmail, userPassword);

using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client, getNetwork(network));

FirebaseApp app;
RealtimeDatabase Database;
AsyncResult aResult_no_callback;

#include <Servo.h>

Servo myservo;

int prevAngle = 0;
void setup() {
  establishWifiConnection(wifi_ssid, wifi_password);
  establishFirebaseConnection();

  // D1
  pinMode(5, OUTPUT);
  myservo.attach(4);

  Database.set<int>(aClient, "/motor_angle", 90);
}

void loop() {
  bool shouldFeed = Database.get<bool>(aClient, "/should_feed");
  if(shouldFeed) {
    rotateServo();
    rotateServo();
    Database.set<bool>(aClient, "/should_feed", false);

    int amount = Database.get<int>(aClient, "/amount");
    Database.set<int>(aClient, "/amount", amount + 1);
  }

  bool blinkLed = Database.get<bool>(aClient, "/LED");
  if(blinkLed) {
    digitalWrite(5, HIGH);
  }
  else {
    digitalWrite(5, LOW);
  }
}

void rotateServo(){
  int init = 90;
  myservo.write(init);
  delay(700);
  myservo.write(init + 75);
  delay(700);
  myservo.write(init - 75);
  delay(700);
  myservo.write(init);
  delay(700);
}

void establishFirebaseConnection() {
  Firebase.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);

  #if defined(ESP32) || defined(ESP8266) || defined(PICO_RP2040)
  ssl_client.setInsecure();
  #if defined(ESP8266)
  ssl_client.setBufferSizes(4096, 1024);
  #endif
  #endif

  initializeApp(aClient, app, getAuth(user_auth), aResult_no_callback);

  authHandler();

  app.getApp<RealtimeDatabase>(Database);

  Database.url(dbUrl);
}

void establishWifiConnection(String ssid, String password) {
  Serial.begin(115200);
  delay(10);
  WiFi.begin(ssid, password);

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }
}

void authHandler()
{
  unsigned long ms = millis();
  while (app.isInitialized() && !app.ready() && millis() - ms < 120 * 1000)
  {
    JWT.loop(app.getAuth());
  }
}

void printError(int code, const String &msg)
{
  Firebase.printf("Error, msg: %s, code: %d\n", msg.c_str(), code);
}