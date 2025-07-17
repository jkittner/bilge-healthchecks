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
    String url_ok;
    String url_fail;
};

/* new sensors can be added here */
Sensor sensors[]{{"bilge-forepeak", D1, true, 0},
                 {"bilge-midships", D2, true, 0},
                 {"bilge-aft", D5, true, 0}};

WiFiClient client;
bool wifiWasConnected = true;

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
    /* red fault LED */
    pinMode(D0, OUTPUT);
    /* blue status LED */
    pinMode(D4, OUTPUT);
    digitalWrite(D4, HIGH);
    for (int i = 0; i < sizeof(sensors) / sizeof(sensors[0]); i++) {
        // NC input - use internal pull-up resistor
        pinMode(sensors[i].pin, INPUT_PULLUP);
        /* build the ping URLs dynamically per sensor */
        sensors[i].url_ok =
            ping_base_url + ping_key + "/" + String(sensors[i].sensor_name);
        sensors[i].url_fail = ping_base_url + ping_key + "/" +
                              String(sensors[i].sensor_name) + "/fail";
    }
    WiFiSetup();
}

void
loop()
{
    bool currentlyConnected = WiFi.status() == WL_CONNECTED;
    if (!currentlyConnected) {
        digitalWrite(D0, LOW);
        delay(200);
        digitalWrite(D0, HIGH);
        wifiWasConnected = false;
    }
    for (int i = 0; i < sizeof(sensors) / sizeof(sensors[0]); i++) {
        Sensor &current_sensor = sensors[i];
        /* LOW = OK (closed), HIGH = open (ALARM) */
        bool current_state = digitalRead(current_sensor.pin);
        // check if there was a change in the sensor state and update the remote
        // if so
        if ((current_state != current_sensor.state && currentlyConnected) ||
            (currentlyConnected && !wifiWasConnected)) {
            current_sensor.state = current_state;
            updateRemoteSensorState(current_sensor);
            updateFaultLed();
        }
        /* periodic check to see if the wifi is still online
           https://www.norwegiancreations.com/2018/10/arduino-tutorial-avoiding-the-overflow-issue-when-using-millis-and-micros/
           This still works fine when the millis() overflow e.g. last ping was
           at 4294967290, however, a bit later millis() returns e.g. 320000
           320000 - 4294967290 casted to an unsigned long results in 4294647290
           which is greater than the health_check_interval hence the call is
           triggered. The new lastPingTime would be 320000 hence we're back to
           normal.
        */
        if ((unsigned long)(millis() - current_sensor.lastPingTime) >=
            health_check_interval) {
            Serial.println("sending periodic ping");
            updateRemoteSensorState(current_sensor);
            updateFaultLed();
            current_sensor.lastPingTime = millis();
            /* signal we're still alive */
            digitalWrite(D4, LOW);
            delay(100);
            digitalWrite(D4, HIGH);
            delay(100);
        }
    }
    if (currentlyConnected) {
        wifiWasConnected = true;
    }
    delay(200);  // Poll every 200ms
}

void
updateRemoteSensorState(Sensor &sensor)
{
    HTTPClient http;
    String target_url;
    if (sensor.state == LOW) {
        http.begin(client, sensor.url_ok);
        Serial.println("OK: Contact closed!");
    }
    else {
        http.begin(client, sensor.url_fail);
        Serial.println("FAULT: Contact open!");
    }
    int httpCode = http.GET();
    http.end();

    Serial.printf("Sent ping for %s, status code: %d\n", sensor.sensor_name,
                  httpCode);
}

void
updateFaultLed()
{
    bool anyFault = false;
    for (int i = 0; i < sizeof(sensors) / sizeof(sensors[0]); i++) {
        if (sensors[i].state == HIGH) {
            anyFault = true;
            break;
        }
    }
    digitalWrite(D0, anyFault ? LOW : HIGH);
}
