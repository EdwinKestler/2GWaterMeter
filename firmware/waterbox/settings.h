#ifndef WATERBOX_SETTINGS_H
#define WATERBOX_SETTINGS_H

// Network (Tigo Guatemala 2G M2M). Confirm the APN with the carrier before
// field use; 2G is end-of-life in many markets — see docs/2G-EOL.md.
#define GSMAPN "m2mgt.tigo.com"
#define GSMUSER ""
#define GSMPASSWORD ""

const char mqttServer[] = "iotarduinodaygt.flatbox.io";
const uint16_t mqttPort = 1883;

// Broker auth. Both must be non-empty unless MQTT_ALLOW_ANONYMOUS is 1 (lab only).
const char mqttUser[] = "";
const char mqttPass[] = "";
#define MQTT_ALLOW_ANONYMOUS 0

// Topics are built as waterbox/<IMEI>/{data,cmd,info} after the modem reports IMEI.
#define TOPIC_ROOT "waterbox"

// RPE BVS_RF-3-30L: 245 pulses/litre Hall (black JST), 490 Reed (white JST).
// Set to 1 if the installed pickup is Reed.
#define FLOW_SENSOR_REED 0
#define PULSES_PER_LITER_HALL 245
#define PULSES_PER_LITER_REED 490

// Microduino Core+: Serial1 on D2/D3 (SIM800), PWM on D9/D10 (L9110),
// external interrupts on D2, D3, and D6. D6 is the only free INT pin.
#define PIN_VALVE_IA 9
#define PIN_VALVE_IB 10
#define PIN_FLOW 6
#define PIN_SIM_PWR 5
#define PIN_SIM_RST 7
#define PIN_STATUS_LED 13

// RPE latching L6V coil is specified at 25 ms. Holding (100% ED) coils need a
// longer pulse plus a hold voltage — do not use this value for those SKUs.
#define VALVE_PULSE_MS 25
#define VALVE_SPEED 255

#define PUBLISH_INTERVAL_MS 60000UL
#define CMD_LISTEN_MS 8000UL
#define RADIO_RETRY_MAX 3
#define RADIO_RETRY_DELAY_MS 2000UL
#define SIM_INIT_TRIES 10
#define SIM_RETRY_MS 30000UL
#define FLOW_SAMPLE_MS 1000UL
#define EEPROM_SAVE_MS 60000UL

#endif
