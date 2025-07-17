[![pre-commit.ci status](https://results.pre-commit.ci/badge/github/jkittner/bilge-healthchecks/main.svg)](https://results.pre-commit.ci/latest/github/jkittner/bilge-healthchecks/main)

# bilge-healthchecks

Monitoring tool using [healthchecks.io](https://healthchecks.io/) for monitoring the
state of normally-closed switches (bilge sensors).

## Getting started

This is built on top of a NodeMCU (ESP8266) with WiFi.

For the configuration you will need to fill out the configuration section in
`bilge_healthchecks.ino`.

```cpp
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
```

|       variable        | description                                                                                                           |
| :-------------------: | --------------------------------------------------------------------------------------------------------------------- |
|         ssid          | name of the WiFi network (2.4 GHz)                                                                                    |
|       password        | WiFi Password for the network                                                                                         |
|       ping_key        | when using slug-based pinging                                                                                         |
|     ping_base_url     | the base url where the healthchecks instance is hosted. Usually `http://hc-ping.com/` but different when self-hosted. |
| health_check_interval | interval in milliseconds to send status pings even if the status has not changed. Currently 5 minutes.                |

Compile and send the program to the NodeMCU.

### Wiring

The following pins are used

| pin | target sensor  |
| :-: | -------------- |
| D2  | bilge-forepeak |
| D2  | bilge-midships |
| D5  | bilge-aft      |

A closed circuit means healthy, an open circuit means faulty. This can be tested by
simply using a cable and pull the pins to ground (GND).

### LED indicators

- blue LED flashes every `health_check_interval` milliseconds n-sensor times. E.g. If we
  have three sensors, and our `health_check_interval` is 5 minutes the blue LED will
  flash three times every 5 minutes. This indicates a healthy state if no red LED is
  active.
- red LED lights up consistently: This means there is an ongoing alarm with any of the
  sensors connected
- red LED permanently flashes: The WiFi connection is lost no data can be transmitted
  and alarms will be lost. The device is in unhealthy state.

## Adding additional sensors

You may add additional sensors by extending using the array of `Sensor`. e.g. like this

```cpp
{"bilge-somewhere-else", D6, true, 0},
```

The first is the name of the sensor which must match the slug in healthchecks.io, the
2nd entry is the pin the switch will be connected to, the 3rd entry is the initial state
when the controller start, the 4th is the initial state for the last periodic ping.
