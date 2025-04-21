#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>

const char* ssid = "ESP";
const char* password = "ESP328266";
const char* host = "192.168.0.50";  // ESP32 IP

const int switchPinRoom1 = D5;  // Room 1 Occupancy
const int switchPinRoom2 =D6 ;  // Room 2 Occupancy
const int switchPinFan = D2;    // Fan switch
const int switchPinLight = D7;  // Light switch

WebSocketsClient webSocket;
bool lastRoom1State = HIGH;
bool lastRoom2State = HIGH;
bool lastFanState = HIGH;
bool lastLightState = HIGH;

unsigned long lastDebounceTimeRoom1 = 0;
unsigned long lastDebounceTimeRoom2 = 0;
unsigned long lastDebounceTimeFan = 0;
unsigned long lastDebounceTimeLight = 0;

const unsigned long debounceDelay = 50;

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    if (type == WStype_DISCONNECTED) {
        Serial.println("Disconnected from WebSocket server");
    } else if (type == WStype_CONNECTED) {
        Serial.println("Connected to WebSocket server");
    }
}

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    
    Serial.print("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi Connected!");

    webSocket.begin(host, 81, "/");
    webSocket.onEvent(webSocketEvent);

    pinMode(switchPinRoom1, INPUT_PULLUP);
    pinMode(switchPinRoom2, INPUT_PULLUP);
    pinMode(switchPinFan, INPUT_PULLUP);
    pinMode(switchPinLight, INPUT_PULLUP);
}

void loop() {
    webSocket.loop();

    int room1State = digitalRead(switchPinRoom1);
    int room2State = digitalRead(switchPinRoom2);
    int fanState = digitalRead(switchPinFan);
    int lightState = digitalRead(switchPinLight);

    // ROOM 1 OCCUPANCY CHECK
    if (room1State != lastRoom1State && millis() - lastDebounceTimeRoom1 > debounceDelay) {
        lastDebounceTimeRoom1 = millis();
        if (room1State == HIGH) { // Room is now vacant
            Serial.println("Room 1 Vacant");
            webSocket.sendTXT("RM1_VACANT");
            checkFanLightStatus();  // Check if fan or light is still ON
        } else {
            Serial.println("Room 1 Occupied");
            webSocket.sendTXT("RM1_OCCUPIED");
        }
    }

    // ROOM 2 OCCUPANCY CHECK
    if (room2State != lastRoom2State && millis() - lastDebounceTimeRoom2 > debounceDelay) {
        lastDebounceTimeRoom2 = millis();
        if (room2State == HIGH) { // Room is now vacant
            Serial.println("Room 2 Vacant");
            webSocket.sendTXT("RM2_VACANT");
            checkFanLightStatus();
        } else {
            Serial.println("Room 2 Occupied");
            webSocket.sendTXT("RM2_OCCUPIED");
        }
    }

    // FAN SWITCH CHECK
    if (fanState != lastFanState && millis() - lastDebounceTimeFan > debounceDelay) {
        lastDebounceTimeFan = millis();
        Serial.println(fanState == LOW ? "Fan turned ON" : "Fan turned OFF");
        webSocket.sendTXT(fanState == LOW ? "FAN_ON" : "FAN_OFF");
    }

    // LIGHT SWITCH CHECK
    if (lightState != lastLightState && millis() - lastDebounceTimeLight > debounceDelay) {
        lastDebounceTimeLight = millis();
        Serial.println(lightState == LOW ? "Light turned ON" : "Light turned OFF");
        webSocket.sendTXT(lightState == LOW ? "LIGHT_ON" : "LIGHT_OFF");
    }

    // Update last known states
    lastRoom1State = room1State;
    lastRoom2State = room2State;
    lastFanState = fanState;
    lastLightState = lightState;
}

// FUNCTION TO CHECK FAN & LIGHT STATUS WHEN ROOM IS VACANT
void checkFanLightStatus() {
    int fanState = digitalRead(switchPinFan);
    int lightState = digitalRead(switchPinLight);

    Serial.print("Checking Fan and Light: ");
    Serial.print("Fan = "); Serial.print(fanState == LOW ? "ON" : "OFF");
    Serial.print(", Light = "); Serial.println(lightState == LOW ? "ON" : "OFF");

    if (fanState == LOW) {
        Serial.println("Fan is still ON after vacancy!");
        webSocket.sendTXT("FAN_ON");
    }
    if (lightState == LOW) {
        Serial.println("Light is still ON after vacancy!");
        webSocket.sendTXT("LIGHT_ON");
    }

    if (fanState == HIGH) {
        Serial.println("Fan is still OFF after vacancy!");
        webSocket.sendTXT("FAN_OFF");
    }
    if (lightState == HIGH) {
        Serial.println("Light is still OFF after vacancy!");
        webSocket.sendTXT("LIGHT_OFF");
    }
}
