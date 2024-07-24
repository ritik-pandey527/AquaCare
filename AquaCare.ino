#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif

#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Servo.h>
#include <OneWire.h>
#include <WiFiManager.h>
#include <DallasTemperature.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>

#define BOTtoken "6130639245:AAFAH_cr9zYHGBPVbwGo6gI7xfPOg7AsqJs"
String CHAT_ID = "1316357329";
String ALLOWED_CHAT_IDS = "1316357329";

#ifdef ESP8266
X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiServer server(80);
#endif

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

int botRequestDelay = 500;
unsigned long lastTimeBotRan;

#define PH_PIN A0
#define Turbudity_PIN D0
#define SERVO_PIN D1
#define LIGHT_PIN D2
#define PUMP_PIN D3
#define FILTER_PIN D4
#define ONE_WIRE_BUS D5
#define PIN_RED D6
#define PIN_GREEN D7
#define PIN_BLUE D8

int red1;
int blue1;
int green1;

unsigned long int avgValue;
int buf[10], temp;

String userChatID = "";
bool ledState = LOW;
bool isAddingChatID = false;
bool isRemovingChatID = false;
Servo servo;

void setColor(int red, int green, int blue) {
  analogWrite(PIN_RED, red);
  analogWrite(PIN_GREEN, green);
  analogWrite(PIN_BLUE, blue);
  red1 = red;
  green1 = green;
  blue1 = blue;
}

void setBrightness(int brightness) {
  analogWrite(PIN_RED, brightness);
  analogWrite(PIN_GREEN, brightness);
  analogWrite(PIN_BLUE, brightness);
}

float readTurbidity() {
  int rawValue = analogRead(Turbudity_PIN);
  float turbidity = map(rawValue, 0, 1023, 0, 1000);  // Assuming 0 to 100 as the turbidity range
  return turbidity;
}

void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    if (ALLOWED_CHAT_IDS.indexOf(chat_id) == -1) {
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }

    String text = bot.messages[i].text;
    Serial.println(text);

    String from_name = bot.messages[i].from_name;

    if (text == "/start") {
      bot.sendMessage(chat_id, "Wait...", "");
      String welcome = "Welcome, " + from_name + ".\n";
      welcome += "Use the following commands to control the system:\n\n";
      welcome += "/A - Check Water Quality\n";
      welcome += "/B - Check Water Temperature\n";
      welcome += "/C - Control the Feeder\n";
      welcome += "/D - Control the Light\n";
      welcome += "/E - Control the Pump\n";
      welcome += "/Filter_control - Control the Filter\n";
      welcome += "/Users - Check Allowed Chat IDs\n";
      welcome += "/User_add - Add Chat ID\n";
      welcome += "/User_remove - Remove Chat ID\n";
      bot.sendMessage(chat_id, welcome, "");
    } else if (text == "/A") {
      String options = "Water Quality Check:\n\n";
      options += "/PH_value to check the PH Value\n";
      options += "/Turbidity to check the Turbidity Value\n";
      bot.sendMessage(chat_id, options, "");
    } else if (text == "/PH_value") {
      bot.sendMessage(chat_id, "Wait...", "");
      bot.sendMessage(chat_id, "Measuring Water PH...", "");
      for (int i = 0; i < 10; i++) {
        buf[i] = analogRead(PH_PIN);
        delay(10);
      }
      for (int i = 0; i < 9; i++) {
        for (int j = i + 1; j < 10; j++) {
          if (buf[i] > buf[j]) {
            temp = buf[i];
            buf[i] = buf[j];
            buf[j] = temp;
          }
        }
      }
      avgValue = 0;
      for (int i = 2; i < 8; i++)
        avgValue += buf[i];
      float phValue = (float)avgValue * 5.0 / 1024 / 6;
      phValue = 3.5 * phValue;
      if (phValue >= 6.5 && phValue <= 8.5) {
        String phValue1 = "PH of Water is : " + String(phValue) + " \n";
        bot.sendMessage(chat_id, "Water is Clean", "");
        bot.sendMessage(chat_id, phValue1, "");
      } else if (phValue > 8.5) {
        String phValue1 = "PH of Water is : " + String(phValue) + " \n";
        bot.sendMessage(chat_id, "Water is Dirty", "");
        bot.sendMessage(chat_id, phValue1, "");
      } else if (phValue < 6.5) {
        String phValue1 = "PH of Water is : " + String(phValue) + " \n";
        bot.sendMessage(chat_id, "Water is Not Drinkable", "");
        bot.sendMessage(chat_id, phValue1, "");
      }
     else if (phValue > 8.5) {
      String phValue1 = "PH of Water is : " + String(phValue) + " \n";
      // bot.sendMessage(chat_id, "Water is Dirty", "");
      bot.sendMessage(chat_id, phValue1, "");
    } else if (phValue < 6.5) {
      String phValue1 = "PH of Water is : " + String(phValue) + " \n";
      // bot.sendMessage(chat_id, "Water is Not Drinkable", "");
      bot.sendMessage(chat_id, phValue1, "");
    }

  }
  else if (text == "/Turbidity") {
    bot.sendMessage(chat_id, "Wait...", "");
    bot.sendMessage(chat_id, "Measuring Water Turbudity...", "");
    int sensorValue = analogRead(Turbudity_PIN);
    float voltage = sensorValue * (5.0 / 1024.0);
    int turbidityCondition = voltage;
    Serial.print("Turbidity Condition: ");
    Serial.println(turbidityCondition);
    String turbidityMessage = "Turbidity of Water is : " + String(turbidityCondition) + " PPM\n";
    if (turbidityCondition <= 5.0) {
      bot.sendMessage(chat_id, "Water is Clean", "");
    } else if (turbidityCondition > 5.0 && turbidityCondition <= 10.0) {
      bot.sendMessage(chat_id, "Water is Moderately Turbid", "");
    } else {
      bot.sendMessage(chat_id, "Water is Highly Turbid", "");
    }
    bot.sendMessage(chat_id, turbidityMessage, "");
  }
  else if (text == "/B") {
    bot.sendMessage(chat_id, "Wait...", "");
    bot.sendMessage(chat_id, "Measuring Water Temperature...", "");
    OneWire oneWire(ONE_WIRE_BUS);
    DallasTemperature sensors(&oneWire);
    sensors.requestTemperatures();
    float sensorValue = sensors.getTempCByIndex(0);
    String tempSensor = "Water Temperature: " + String(sensorValue) + " °C\n";
    bot.sendMessage(chat_id, tempSensor, "");
  }
  else if (text == "/C") {
    bot.sendMessage(chat_id, "Wait...", "");
    bot.sendMessage(chat_id, "Feeder Started...", "");
    bot.sendMessage(chat_id, "Please wait...", "");
    servo.attach(SERVO_PIN);
    for (int i = 1; i < 5; i++) {
      servo.write(0);
      delay(2000);
      servo.write(180);
      delay(2000);
    }
    bot.sendMessage(chat_id, "Done: Feeder Task Completed.", "");
  }
  else if (text == "/D") {
    bot.sendMessage(chat_id, "Wait...", "");
    String options = "Light Control Options:\n\n";
    options += "/1 to Check Light Status\n";
    options += "/2 to Turn ON Light\n";
    options += "/3 to Turn OFF Light\n";
    options += "/Colour_control to Control Colour\n";
    options += "/Brightness_control to Control Brightness\n";
    bot.sendMessage(chat_id, options, "");
  }
  else if (text == "/E") {
    bot.sendMessage(chat_id, "Wait...", "");
    String options = "Pump Control Options:\n\n";
    options += "/4 to Check Pump Status\n";
    options += "/5 to Turn ON Pump\n";
    options += "/6 to Turn OFF Pump\n";
    bot.sendMessage(chat_id, options, "");
  }
  else if (text == "/1") {
    bot.sendMessage(chat_id, "Wait...", "");
    pinMode(LIGHT_PIN, OUTPUT);
    String lightStatus = digitalRead(LIGHT_PIN) ? "LIGHT is ON" : "LIGHT is OFF";
    bot.sendMessage(chat_id, lightStatus, "");
  }
  else if (text == "/2") {
    bot.sendMessage(chat_id, "Wait...", "");
    pinMode(LIGHT_PIN, OUTPUT);
    digitalWrite(LIGHT_PIN, HIGH);
    bot.sendMessage(chat_id, "Done: Light Turned ON", "");
  }
  else if (text == "/3") {
    bot.sendMessage(chat_id, "Wait...", "");
    pinMode(LIGHT_PIN, OUTPUT);
    digitalWrite(LIGHT_PIN, LOW);
    bot.sendMessage(chat_id, "Done: Light Turned OFF", "");
  }
  else if (text == "/4") {
    bot.sendMessage(chat_id, "Wait...", "");
    pinMode(PUMP_PIN, OUTPUT);
    String pumpStatus = digitalRead(PUMP_PIN) ? "Pump is ON" : "Pump is OFF";
    bot.sendMessage(chat_id, pumpStatus, "");
  }
  else if (text == "/5") {
    pinMode(PUMP_PIN, OUTPUT);
    bot.sendMessage(chat_id, "Wait...", "");
    digitalWrite(PUMP_PIN, HIGH);
    bot.sendMessage(chat_id, "Done: Pump Turned ON", "");
  }
  else if (text == "/6") {
    bot.sendMessage(chat_id, "Wait...", "");
    pinMode(PUMP_PIN, OUTPUT);
    digitalWrite(PUMP_PIN, LOW);
    bot.sendMessage(chat_id, "Done: Pump Turned OFF", "");
  }
  else if (text == "/Users") {
    bot.sendMessage(chat_id, "Wait...", "");
    String message = "Allowed Chat IDs:\n";
    for (size_t i = 0; i < ALLOWED_CHAT_IDS.length(); i++) {
      if (ALLOWED_CHAT_IDS[i] == ',') {
        message += '\n';
      } else {
        message += ALLOWED_CHAT_IDS[i];
      }
    }
    bot.sendMessage(chat_id, message, "");
  }
  else if (text == "/User_add") {
    bot.sendMessage(chat_id, "Wait...", "");
    if (chat_id == CHAT_ID) {
      bot.sendMessage(chat_id, "Please provide the chat ID you want to add to the allowed list:", "");
      isAddingChatID = true;
    } else {
      bot.sendMessage(chat_id, "Unauthorized access. Only the bot owner can grant user control.", "");
    }
  }
  else if (isAddingChatID && chat_id == CHAT_ID) {
    if (userChatID == "") {
      userChatID = text;
      bot.sendMessage(chat_id, "Please enter the chat ID again to confirm:", "");
    } else if (userChatID == text) {
      ALLOWED_CHAT_IDS += "," + text;
      bot.sendMessage(chat_id, "User chat ID added to allowed list.", "");
      userChatID = "";
      isAddingChatID = false;
    }
  }
  else if (text == "/User_remove") {
    bot.sendMessage(chat_id, "Wait...", "");
    if (chat_id == CHAT_ID) {
      bot.sendMessage(chat_id, "Please provide the chat ID you want to remove from the allowed list:", "");
      isRemovingChatID = true;
    } else {
      bot.sendMessage(chat_id, "Unauthorized access. Only the bot owner can remove users.", "");
    }
  }
  else if (isRemovingChatID && chat_id == CHAT_ID) {
    int userIndex = ALLOWED_CHAT_IDS.indexOf(text);
    if (userIndex != -1) {
      ALLOWED_CHAT_IDS.remove(userIndex, text.length() + 1);  // +1 to remove comma as well
      bot.sendMessage(chat_id, "User chat ID removed from allowed list.", "");
    } else {
      bot.sendMessage(chat_id, "The provided chat ID was not found in the allowed list.", "");
    }
    isRemovingChatID = false;
  }
  else if (text == "/Colour_control") {
    bot.sendMessage(chat_id, "Wait...", "");
    String options = "Colour Control Options:\n";
    options += "/Red \n";
    options += "/Blue \n";
    options += "/Green \n";
    options += "/White \n";
    bot.sendMessage(chat_id, options, "");
  }
  else if (text == "/Brightness_control") {
    bot.sendMessage(chat_id, "Wait...", "");
    String options = "Brightness Control Options:\n";
    options += "/20% \n";
    options += "/40% \n";
    options += "/60% \n";
    options += "/80% \n";
    options += "/100% \n";
    bot.sendMessage(chat_id, options, "");
  }
  else if (text == "/Red") {
    bot.sendMessage(chat_id, "Wait...", "");
    // color code #00C9CC (R = 0,   G = 201, B = 204)
    setColor(0, 255, 255);
    bot.sendMessage(chat_id, "Done: Light Set to Red Colour", "");
  }
  else if (text == "/Green") {
    bot.sendMessage(chat_id, "Wait...", "");
    // color code #00C9CC (R = 0,   G = 201, B = 204)
    setColor(255, 0, 255);
    bot.sendMessage(chat_id, "Done: Light Set to Green Colour", "");
  }
  else if (text == "/Blue") {
    bot.sendMessage(chat_id, "Wait...", "");
    // color code #00C9CC (R = 0,   G = 201, B = 204)
    setColor(255, 255, 0);
    bot.sendMessage(chat_id, "Done: Light Set to Blue Colour", "");
  }
  else if (text == "/White") {
    bot.sendMessage(chat_id, "Wait...", "");
    // color code #00C9CC (R = 0,   G = 201, B = 204)
    setColor(0, 0, 0);
    bot.sendMessage(chat_id, "Done: Light Set to White Colour", "");
  }
  else if (text == "/20") {
    bot.sendMessage(chat_id, "Wait...", "");
    setBrightness(255);
    bot.sendMessage(chat_id, "Done: Light Brightness Set to 20%", "");
  }
  else if (text == "/40") {
    bot.sendMessage(chat_id, "Wait...", "");
    setBrightness(204);
    bot.sendMessage(chat_id, "Done: Light Brightness Set to 40%", "");
  }
  else if (text == "/60") {
    bot.sendMessage(chat_id, "Wait...", "");
    setColor(red1 * 0.5, green1 * 0.5, blue1 * 0.5);
    bot.sendMessage(chat_id, "Done: Light Brightness Set to 60%", "");
  }
  else if (text == "/80") {
    bot.sendMessage(chat_id, "Wait...", "");
    setBrightness(102);
    bot.sendMessage(chat_id, "Done: Light Brightness Set to 80%", "");
  }
  else if (text == "/100") {
    bot.sendMessage(chat_id, "Wait...", "");
    setBrightness(51);
    bot.sendMessage(chat_id, "Done: Light Brightness Set to 100%", "");
  }
  else if (text == "/Filter_control") {
    bot.sendMessage(chat_id, "Wait...", "");
    String options = "Pump Control Options:\n\n";
    options += "/7 to Check Pump Status\n";
    options += "/8 to Turn ON Pump\n";
    options += "/9 to Turn OFF Pump\n";
    bot.sendMessage(chat_id, options, "");
  }
  else if (text == "/7") {
    bot.sendMessage(chat_id, "Wait...", "");
    pinMode(FILTER_PIN, OUTPUT);
    String filterstatus = digitalRead(FILTER_PIN) ? "Filter is ON" : "Filter is OFF";
    bot.sendMessage(chat_id, filterstatus, "");
  }
  else if (text == "/8") {
    bot.sendMessage(chat_id, "Wait...", "");
    pinMode(FILTER_PIN, OUTPUT);
    digitalWrite(FILTER_PIN, HIGH);
    bot.sendMessage(chat_id, "Done: Filter Turned ON", "");
  }
  else if (text == "/9") {
    bot.sendMessage(chat_id, "Wait...", "");
    pinMode(FILTER_PIN, OUTPUT);
    digitalWrite(FILTER_PIN, LOW);
    bot.sendMessage(chat_id, "Done: Filter Turned OFF", "");
  }

  else {
    bot.sendMessage(chat_id, "Wait...", "");
    bot.sendMessage(chat_id, "Invalid Input", "");
    String welcome = "Welcome, " + from_name + ".\n";
    welcome += "Use the following commands to control the system:\n\n";
    welcome += "/A - Check Water Quality\n";
    welcome += "/B - Check Water Temperature\n";
    welcome += "/C - Control the Feeder\n";
    welcome += "/D - Control the Light\n";
    welcome += "/E - Control the Pump\n";
    welcome += "/Users - Check Allowed Chat IDs\n";
    welcome += "/User_add - Add Chat ID\n";
    welcome += "/User_remove - Remove Chat ID\n";
    bot.sendMessage(chat_id, welcome, "");
  }
}
}

void setup() {
  Serial.begin(115200);

#ifdef ESP8266
  configTime(0, 0, "pool.ntp.org");
  client.setTrustAnchors(&cert);
#endif

  pinMode(LIGHT_PIN, OUTPUT);
  digitalWrite(LIGHT_PIN, LOW);
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW);
  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_BLUE, OUTPUT);
  pinMode(PH_PIN, INPUT);
  pinMode(Turbudity_PIN, INPUT);
  digitalWrite(LIGHT_PIN, HIGH);

  WiFi.mode(WIFI_STA);
#ifdef ESP32
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
#endif
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println(WiFi.localIP());
  servo.attach(SERVO_PIN);
  
  WiFiManager wifiManager;
  if (!wifiManager.autoConnect("Aqua_Care")) {
    Serial.println("Failed to connect and hit timeout");
    delay(3000);
    ESP.restart();
  }
}

void loop() {
  static unsigned long previousMillisFeeder = 0;
  static unsigned long previousMillisPump = 0;
  static unsigned long previousMillisTemperature = 0;
  unsigned long currentMillis = millis();
  const unsigned long feederInterval = 5 * 60 * 60 * 1000;     // 5 Hours in milliseconds
  const unsigned long pumpInterval = 7 * 24 * 60 * 60 * 1000;  // 7 days in milliseconds
  const unsigned long temperatureInterval = 30 * 60 * 1000;    // 30 Minutes in milliseconds

  if (millis() > lastTimeBotRan + botRequestDelay) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    if (numNewMessages > 0) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
    }
    lastTimeBotRan = millis();
  } else if (currentMillis - previousMillisFeeder >= feederInterval) {
    previousMillisFeeder = currentMillis;
    for (int i = 1; i < 5; i++) {
      servo.write(0);
      delay(2000);
      servo.write(180);
      delay(2000);
    }
  } else if (currentMillis - previousMillisPump >= pumpInterval) {
    previousMillisPump = currentMillis;
    // Time = Volume / Flow rate
    // Time = 8 liters / 1 liter per minute
    // Time = 8 minutes = 480 milliseconds
    for (int i = 0; i < 480; i++) {
      pinMode(PUMP_PIN, OUTPUT);
      digitalWrite(PUMP_PIN, HIGH);
      delay(1);
    }
  } else if (currentMillis - previousMillisTemperature >= temperatureInterval) {
    previousMillisTemperature = currentMillis;
    OneWire oneWire(ONE_WIRE_BUS);
    DallasTemperature sensors(&oneWire);
    sensors.requestTemperatures();
    float sensorValue = sensors.getTempCByIndex(0);

    if (sensorValue >= 20 && sensorValue <= 30) {
      String alertMessage = "Alert: Temperature exceeds threshold!\n";
      alertMessage += "Current Temperature: " + String(sensorValue) + " °C\n";
      bot.sendMessage(CHAT_ID, alertMessage, "");
    }
  }
}