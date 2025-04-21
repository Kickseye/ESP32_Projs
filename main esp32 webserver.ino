#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <TimeLib.h>
#include <Preferences.h>


Preferences preferences;

// Replace with your network credentials
const char* ssid = "ESP";
const char* password = "ESP328266";

// Define GPIO pins for Room 1
#define FAN1 5
#define LIGHT1 18
#define SWITCH1 4  // Switch for Room 1
#define SWITCH2 13  // Switch for Room 2

// Define GPIO pins for Room 2
#define FAN2 19
#define LIGHT2 21

WebServer server(80);
WebSocketsServer webSocket(81);

bool status1State = false;  // Track toggle switch state
bool status2State = false;  // Track toggle switch state

unsigned long occupiedTime1 = 0;
unsigned long occupiedTime2 = 0;

const int physicalSwitchPin = 4; // GPIO for physical switch
const int physicalSwitchPin2 = 13; // GPIO for physical switch

bool physicalSwitchState = false;
bool physicalSwitchState2 = false;

bool lastSwitchState = HIGH;
int room_number=0;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;
unsigned long occupiedStartTime = 0;

// HTML Page (optimized for ESP32)
const char html_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Academic Building UI</title>
    <style>
        body { background: #772D2D; font-family: Arial, sans-serif; text-align: center; color: white; }
        .header { display: flex; justify-content: space-between; align-items: center; padding: 10px; background: #5A1F1F; }
        .header button { background: none; border: 2px solid white; padding: 10px 20px; color: white; font-weight: bold; cursor: pointer; }
        .clock { font-size: 18px; font-weight: bold; }
        .content { display: flex; justify-content: center; gap: 20px; margin-top: 30px; }
        .card { background: rgba(255, 255, 255, 0.2); padding: 20px; border-radius: 10px; width: 250px; text-align: left; }
        .status, .switch-container, .time-container { display: flex; align-items: center; gap: 10px; margin-top: 10px; }
        .switch { position: relative; width: 40px; height: 20px; }
        .switch input { opacity: 0; width: 0; height: 0; }
        .slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background: #ccc; transition: 0.4s; border-radius: 20px; }
        .slider:before { position: absolute; content: ""; height: 14px; width: 14px; left: 3px; bottom: 3px; background: white; transition: 0.4s; border-radius: 50%; }
        input:checked + .slider { background: #2196F3; }
        input:checked + .slider:before { transform: translateX(18px); }
    </style>
    <script>
       

       document.addEventListener("DOMContentLoaded", function () {
        fetch('/getStatus')
        .then(response => response.json())
        .then(data => {
            console.log("Received status:", data);

            // Update Room 1 Switches
            document.getElementById("rm_disable1").checked = data.room1.room_av;
            document.getElementById("status1").checked = data.room1.status;
            document.getElementById("fan1").checked = data.room1.fan;
            document.getElementById("light1").checked = data.room1.light;

            // Update Room 2 Switches
            document.getElementById("rm_disable2").checked = data.room2.room_av;
            document.getElementById("status2").checked = data.room2.status;
            document.getElementById("fan2").checked = data.room2.fan;
            document.getElementById("light2").checked = data.room2.light;
        })
        .catch(error => console.error("Error fetching status:", error));
    });

        function updateClock() { document.getElementById('clock').textContent = new Date().toLocaleTimeString(); }
        setInterval(updateClock, 1000);
        window.onload = updateClock;

        let timers = {};
        function toggleRoomStatus(s, f, l, labelId, timeElapsedId, timeStampId) {
            let label = document.getElementById(labelId), timeElapsed = document.getElementById(timeElapsedId), timeStamp = document.getElementById(timeStampId);
            if (s.checked) {
                f.checked = l.checked = true;
                label.textContent = "OCCUPIED";
                 timeStamp.textContent = new Date().toLocaleTimeString();
                if (!timers[timeElapsedId]) {
                    timers[timeElapsedId] = { startTime: Date.now(), interval: setInterval(() => updateElapsedTime(timeElapsedId), 1000) };
                }
            } else {
                f.checked = l.checked = false;
                label.textContent = "VACANT";
                clearInterval(timers[timeElapsedId]?.interval);
                delete timers[timeElapsedId];
            }
        }
        function updateElapsedTime(id) {
            if (!timers[id]) return;
            let e = Math.floor((Date.now() - timers[id].startTime) / 1000), h = Math.floor(e / 3600), m = Math.floor((e % 3600) / 60), s = e % 60;
            document.getElementById(id).textContent = `${h}:${m.toString().padStart(2,'0')}:${s.toString().padStart(2,'0')}`;
        }
        var ws = new WebSocket("ws://" + location.hostname + ":81/");
        
        ws.onmessage = function(event) {
            try {
               console.log("Raw JSON Data:", event.data); // Log raw JSON file
                let data = JSON.parse(event.data);
                console.log("Received WebSocket Data:", data);

                if (data.room) {
                  if (data.room_av == 0){
                    let AVSwitch = document.getElementById("rm_disable" + data.room);
                    AVSwitch.checked = false;
                    let toggleSwitch = document.getElementById("status" + data.room);
                    if (toggleSwitch && toggleSwitch.checked !== data.status) {
                        toggleSwitch.checked = false;
                        toggleSwitch.dispatchEvent(new Event('input'));

                    }
                  }
                  if (data.room_av == 1){
                    let AVSwitch = document.getElementById("rm_disable" + data.room);
                    AVSwitch.checked = true;
                    let toggleSwitch = document.getElementById("status" + data.room);
                    if (toggleSwitch && toggleSwitch.checked !== data.status) {
                        toggleSwitch.checked = data.status;
                        toggleSwitch.dispatchEvent(new Event('input'));
                    }
                  }
                    
                }
                   
                

                if (data.fan) {
                    let fanSwitch = document.getElementById("fan" + data.room);
                    if (fanSwitch && fanSwitch.checked !== data.fan) {
                        fanSwitch.checked = data.fan;
                    }
                }

                if (data.light) {
                    let lightSwitch = document.getElementById("light" + data.room);
                    if (lightSwitch && lightSwitch.checked !== data.light) {
                        lightSwitch.checked = data.light;
                    }
                }
                // if (data.elapsedTime) {
                //         let occupiedTimeElem = document.getElementById("timeStamp"+ data.room);
                //         let hours = Math.floor(data.elapsedTime / 3600);
                //         let minutes = Math.floor((data.elapsedTime % 3600) / 60);
                //         let seconds = data.elapsedTime % 60;
                //         occupiedTimeElem.textContent = `${hours}h ${minutes}m ${seconds}s`;
                //           //.textContent = "Occupied Since: " + new Date(data.time * 1000).toLocaleString();
                //     }
            } catch (error) {
                console.error("Error processing WebSocket message:", error);
            }
        };
    </script>
</head>
<body>
    <div class="header">
        <button>ACADEMIC BUILDING</button>
        <div id="clock" class="clock"></div>
    </div>
    <div class="content">
        <div class="card">
            <h2>ROOM #1</h2>
            <div class="status"><label>Room Availability</label>
                <label class="switch"><input type="checkbox" id="rm_disable1" ><span class="slider"></span></label>
            </div>
            <div class="time-container"><span>Time Occupied:</span> <p id="timeStamp1">--:--</p></div>
            <div class="status"><label id="statusLabel1">VACANT</label>
                <label class="switch"><input type="checkbox" id="status1" oninput="toggleRoomStatus(this, fan1, light1, 'statusLabel1', 'timeElapsed1', 'timeStamp1')"><span class="slider"></span></label>
            </div>
            <div class="switch-container">
                <label>Fan</label><label class="switch"><input type="checkbox" id="fan1"><span class="slider"></span></label>
                <label>Light</label><label class="switch"><input type="checkbox" id="light1"><span class="slider"></span></label>
            </div>
            <div class="time-container"><span>TIME ELAPSED:</span> <p id="timeElapsed1">0:00</p></div>
        </div>
        <div class="card">
            <h2>ROOM #2</h2>
            <div class="status"><label>Room Availability</label>
                <label class="switch"><input type="checkbox" id="rm_disable2" ><span class="slider"></span></label>
            </div>
            <div class="time-container"><span>Time Occupied:</span> <p id="timeStamp2">--:--</p></div>
            <div class="status"><label id="statusLabel2">VACANT</label>
                <label class="switch"><input type="checkbox" id="status2" oninput="toggleRoomStatus(this, fan2, light2, 'statusLabel2', 'timeElapsed2', 'timeStamp2')"><span class="slider"></span></label>
            </div>
            <div class="switch-container">
                <label>Fan</label><label class="switch"><input type="checkbox" id="fan2"><span class="slider"></span></label>
                <label>Light</label><label class="switch"><input type="checkbox" id="light2"><span class="slider"></span></label>
            </div>
            <div class="time-container"><span>TIME ELAPSED:</span> <p id="timeElapsed2">0:00</p></div>
        </div>
    </div>
</body>
</html>

)rawliteral";

// Handles requests from the web UI
void handleUpdate() {
    int room = server.arg("room").toInt();
    bool status = server.arg("status") == "true";
    bool fan = server.arg("fan") == "true";
    bool fan2 = server.arg("fan2") == "true";
    bool light = server.arg("light") == "true";
    bool light2 = server.arg("light2") == "true";

    if (room == 1) {
        digitalWrite(FAN1, fan);
        digitalWrite(LIGHT1, light);
        status1State = status;
    } else if (room == 2) {
        digitalWrite(FAN2, fan);
        digitalWrite(LIGHT2, light);
    }

    server.send(200, "text/plain", "OK");
}

void handleRoot() {
    server.send_P(200, "text/html", html_page);
}

void saveOccupancyTime(int room) {
    unsigned long nowTime = now();
    if (room == 1) {
        preferences.putUInt("occupied1", nowTime);
    } else if (room == 2) {
        preferences.putUInt("occupied2", nowTime);
    }
}
unsigned long getOccupancyTime(int room) {
    if (room == 1) {
        return preferences.getUInt("occupied1", 0);
    } else if (room == 2) {
        return preferences.getUInt("occupied2", 0);
    }
    return 0;
}


void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    if (type == WStype_TEXT) {
        String message = String((char *)payload);
        Serial.println("Received WebSocket Message: " + message);

        bool updateUI = false; // Track whether we need to update the UI
        bool fanState = digitalRead(FAN1);
        bool fanState2 = digitalRead(FAN2);
        bool lightState = digitalRead(LIGHT1);
        bool lightState2 = digitalRead(LIGHT2);

        if (message == "RM1_OCCUPIED") {
            status1State = true;
            room_number = 1;
            digitalWrite(FAN1, HIGH);
            digitalWrite(LIGHT1, HIGH);
            preferences.putBool("status1", true);
            preferences.putBool("fan1", true);
            preferences.putBool("light1", true);
            saveOccupancyTime(1);

            updateUI = true;
        } 
        else if (message == "RM1_VACANT") {
            status1State = false;
            preferences.putBool("status1", false);
            room_number = 1;
            if (fanState == LOW || lightState == LOW) {
                updateUI = true; // Update UI to show they are still ON
            } else {
                digitalWrite(FAN1, LOW);
                digitalWrite(LIGHT1, LOW);
                preferences.putBool("fan1", false);
                preferences.putBool("light1", false);
                updateUI = true;
            }
        }
        else if (message == "RM2_OCCUPIED") {
            status2State = true;
            room_number = 2;
            digitalWrite(FAN2, HIGH);
            digitalWrite(LIGHT2, HIGH);
            preferences.putBool("status2", true);
            preferences.putBool("fan2", true);
            preferences.putBool("light2", true);
            saveOccupancyTime(2);
            updateUI = true;
        } 
        else if (message == "RM2_VACANT") {
            status2State = false;
            preferences.putBool("status2", false);
            room_number = 2;
            if (fanState == LOW || lightState == LOW) {
                updateUI = true; // Update UI to show they are still ON
            } else {
                digitalWrite(FAN2, LOW);
                digitalWrite(LIGHT2, LOW);
                preferences.putBool("fan2", false);
                preferences.putBool("light2", false);
                updateUI = true;
               
            }
        }
        else if (message == "FAN_ON") {
            room_number = 1;
            digitalWrite(FAN1, HIGH);
            preferences.putBool("fan1", true);
            fanState = true;
            updateUI = true;
        } 
        else if (message == "FAN_OFF") {
            room_number = 1;
            digitalWrite(FAN1, LOW);
            fanState = false;
            preferences.putBool("fan1", false);
            updateUI = true;
        }
        else if (message == "LIGHT_ON") {
            room_number = 1;
            digitalWrite(LIGHT1, HIGH);
            preferences.putBool("light1", true);
            lightState = true;
            updateUI = true;
        } 
        else if (message == "LIGHT_OFF") {
            room_number = 1;
            digitalWrite(LIGHT1, LOW);
            preferences.putBool("light1", false);
            lightState = false;
            updateUI = true;
        }
        else if (message == "FAN_ON_2") {
            room_number = 2;
            digitalWrite(FAN2, HIGH);
            preferences.putBool("fan2", true);
            fanState2 = true;
            updateUI = true;
        } 
        else if (message == "FAN_OFF_2") {
            room_number = 2;
            digitalWrite(FAN2, LOW);
            preferences.putBool("fan2", false);
            fanState2 = false;
            updateUI = true;
        }
        else if (message == "LIGHT_ON_2") {
            room_number = 2;
            digitalWrite(LIGHT2, HIGH);
            lightState2 = true;
            preferences.putBool("light2", true);
            updateUI = true;
        } 
        else if (message == "LIGHT_OFF_2") {
            room_number = 2;
            digitalWrite(LIGHT2, LOW);
            lightState2 = false;
            preferences.putBool("light2", false);
            updateUI = true;
        }

        // Broadcast updated status to all WebSocket clients if there was a change
        if (updateUI) {
            static unsigned long occupiedTime1 = 0;
            static unsigned long occupiedTime2 = 0;

            String available = physicalSwitchState ? "1" : "0"; //enable : disable
            String available2 = physicalSwitchState2 ? "1" : "0"; //enable : disable

            if (status1State == 1 && occupiedTime1 == 0) {
                occupiedTime1 = millis();  // Store first time when occupied
            } 
            if (status2State == 1 && occupiedTime2 == 0) {
                occupiedTime2 = millis();
            }
        unsigned long currentTime = millis();
        unsigned long elapsed1 = (status1State == 1) ? (currentTime - occupiedTime1) / 1000 : 0;
        unsigned long elapsed2 = (status2State == 1) ? (currentTime - occupiedTime2) / 1000 : 0;



          if(room_number == 1 ){
            String json = "{\"room\":1, \"room_av\": " + String(digitalRead(physicalSwitchPin)) + ", \"status\": " + String(status1State) + 
                          ", \"fan\": " + String(fanState) + 
                          ", \"light\": " + String(lightState) + 
                          ", \"elapsedTime\": " + String(elapsed1) + "}";

            webSocket.broadcastTXT(json);
          }
          else if (room_number == 2) {
             String json = "{\"room\":2, \"room_av\": " + String(digitalRead(physicalSwitchPin2)) + ", \"status\": " + String(status2State) + 
                          ", \"fan\": " + String(fanState2) + 
                          ", \"light\": " + String(lightState2) + 
                          ", \"elapsedTime\": " + String(elapsed2) + "}";

            webSocket.broadcastTXT(json);
          }
        }
    }
}



// Send JavaScript update to change the switch state in the UI
void updateClientStatus() {
    // String script = "<script>document.getElementById('status1').checked = ";
    // script += status1State ? "true" : "false";
    // script += ";</script>";
    // server.send(200, "text/html", script);



     String json = "{";
    json += "\"room1\":{ \"room_av\": " + String(digitalRead(physicalSwitchPin)) +",\"status\":" + String(status1State) + ",\"fan\":" + String(digitalRead(FAN1)) + ",\"light\":" + String(digitalRead(LIGHT1)) + "},";
    json += "\"room2\":{ \"room_av\": " + String(digitalRead(physicalSwitchPin2)) +",\"status\":" + String(status2State) + ",\"fan\":" + String(digitalRead(FAN2)) + ",\"light\":" + String(digitalRead(LIGHT2)) + "}";
    json += "}";

    server.send(200, "application/json", json);
}

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected, IP: " + WiFi.localIP().toString());

    pinMode(FAN1, OUTPUT);
    pinMode(LIGHT1, OUTPUT);
    pinMode(FAN2, OUTPUT);
    pinMode(LIGHT2, OUTPUT);
    pinMode(SWITCH1, INPUT_PULLUP);  // Set button as input with internal pull-up
    pinMode(SWITCH2, INPUT_PULLUP);  // Set button as input with internal pull-up

    server.on("/getStatus", updateClientStatus); //refresh storage

    pinMode(physicalSwitchPin, INPUT_PULLUP);

 // Load saved preferences
    preferences.begin("room-data", false);
    status1State = preferences.getBool("status1", false);
    status2State = preferences.getBool("status2", false);

    digitalWrite(FAN1, preferences.getBool("fan1", false));
    digitalWrite(LIGHT1, preferences.getBool("light1", false));
    digitalWrite(FAN2, preferences.getBool("fan2", false));
    digitalWrite(LIGHT2, preferences.getBool("light2", false));


    occupiedTime1 = getOccupancyTime(1);
    occupiedTime2 = getOccupancyTime(2);


    server.on("/", handleRoot);
    server.on("/update", handleUpdate);
    server.on("/getStatus", updateClientStatus);  // New route to update UI

    server.begin(); 
    
      // WebSocket setup
    webSocket.begin();
    webSocket.onEvent(webSocketEvent); // Register the event handler

}

void notifyClients() {
    String message = physicalSwitchState ? "0" : "1"; //enable : disable
    String json = "{\"room\": 1, \"room_av\": " + message + ",\"status\":" + String(status1State) + ",\"fan\":" + String(digitalRead(FAN1)) + ",\"light\":" + String(digitalRead(LIGHT1)) + 
                  "}";
    webSocket.broadcastTXT(json);
}
void notifyClients2() {
    String message = physicalSwitchState2 ? "0" : "1"; //enable : disable
    String json = "{\"room\": 2, \"room_av\": " + message + ",\"status\":" + String(status2State) + ",\"fan\":" + String(digitalRead(FAN2)) + ",\"light\":" + String(digitalRead(LIGHT2)) + 
                  "}";
    webSocket.broadcastTXT(json);
}


void loop() {
 server.handleClient();
    webSocket.loop();

  bool newPhysicalSwitchState = digitalRead(physicalSwitchPin) == LOW;
  bool newPhysicalSwitchState2 = digitalRead(physicalSwitchPin2) == LOW;
    
    if (newPhysicalSwitchState != physicalSwitchState) {
        physicalSwitchState = newPhysicalSwitchState;
        notifyClients();
    }

    if (newPhysicalSwitchState2 != physicalSwitchState2) {
        physicalSwitchState2 = newPhysicalSwitchState2;
        notifyClients2();
    }
    delay(100);

}
