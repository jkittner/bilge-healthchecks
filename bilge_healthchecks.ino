#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>

/* configuration section */
/**  WiFi credentials */
const char *ssid = "";
const char *password = "";
/** your ping key get it from https://healthchecks.io */
const String ping_key = "";
/** there is no https possible without including the certificate */
const String ping_base_url = "http://hc-ping.com/";
const unsigned long health_check_interval = 5UL * 60UL * 1000UL;  // 5 minutes
/* end of configuration section */

struct Sensor {
    const char *sensor_name;
    int pin;
    bool state;
    unsigned long lastPingTime;
};

/* new sensors can be added here */
Sensor sensors[]{{"bilge-forepeak", D1, true, 0},
                 {"bilge-midships", D2, true, 0},
                 {"bilge-aft", D5, true, 0}};

WiFiClient client;

void
WiFiSetup()
{
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.println(WiFi.localIP());
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    Serial.println("\nWiFi connected");
}

void
setup()
{
    Serial.begin(115200);
    delay(1000);

    for (int i = 0; i < sizeof(sensors) / sizeof(sensors[0]); i++) {
        // NC input - use internal pull-up resistor
        pinMode(sensors[i].pin, INPUT_PULLUP);
    }

    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    WiFiSetup();
}

void
loop()
{
    unsigned long now = millis();

    for (int i = 0; i < sizeof(sensors) / sizeof(sensors[0]); i++) {
        Sensor &current_sensor = sensors[i];
        /* LOW = OK (closed), HIGH = open (ALARM) */
        bool current_state = digitalRead(current_sensor.pin);
        // check if there was a change in the sensor state and update the remote
        // if so
        if (current_state != current_sensor.state) {
            current_sensor.state = current_state;
            updateRemoteSensorState(current_sensor);
        }
        /* periodic check to see if the wifi is still online */
        if (millis() - current_sensor.lastPingTime >= health_check_interval) {
            Serial.println("sending periodic ping");
            updateRemoteSensorState(current_sensor);
            current_sensor.lastPingTime = millis();
        }
    }
    delay(200);  // Poll every 200ms
}

void
updateRemoteSensorState(Sensor &sensor)
{
    HTTPClient http;
    String target_url;
    if (sensor.state == LOW) {
        target_url =
            ping_base_url + ping_key + "/" + String(sensor.sensor_name);
        Serial.println("OK: Contact closed!");
    }
    else {
        target_url = ping_base_url + ping_key + "/" +
                     String(sensor.sensor_name) + "/fail";
        Serial.println("FAULT: Contact open!");
    }
    http.begin(client, target_url);
    int httpCode = http.GET();
    http.end();

    Serial.printf("Sent ping for %s, status code: %d\n", sensor.sensor_name,
                  httpCode);
}
