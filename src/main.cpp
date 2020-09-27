#include <Arduino.h>
#include <WiFi.h>
#include "math.h"

#include "temp_control.hpp"

// TODO: Add ESP_LOG and ESP_ERR. Use functions output for check success

#define TEMP_SENSOR_PIN 2
#define RELAY_WARMING_ON 0
#define RELAY_COOLING_ON 4
#define LOOP_TIME 5.0

#define NETWORK_NAME "GregoriaNet"
#define NETWORK_PSWD "HansTiberioNacioEnEl2018"

float temp_read_period = 5.0; // Seconds betwing temperature reads
time_t last_temp_read;
time_t current_time;

struct temp_control_handle_t tc_handle = {
    .ref_temp = 43.0,
    .th_temp = 3.0,
    .current_temp = 30.0,
    .relay_warming_pin = RELAY_WARMING_ON,
    .relay_cooling_pin = RELAY_COOLING_ON,
    .warming_on = false,
    .cooling_on = false};

struct temp_sensor_handle_t ts_handle;

void connect_wifi(const char *network_name, const char *network_pswd)
{
  // Connect to Wifi
  WiFi.begin(network_name, network_pswd);
  Serial.printf("Connecting to %s ", network_name);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print("*");
  }
  delay(100);
  Serial.print(" DONE!\n");
}

void setup()
{
  time(&last_temp_read);

  Serial.begin(115200);
  connect_wifi(NETWORK_NAME, NETWORK_PSWD);
  temp_sensor_start(&ts_handle, TEMP_SENSOR_PIN);
  temp_control_configure(&tc_handle);
}

void loop()
{
  time(&current_time);
  if ((current_time - last_temp_read) >= temp_read_period)
  {

    temp_control_run(&ts_handle, &tc_handle);
    time(&last_temp_read);
  }
  sleep(LOOP_TIME);
}