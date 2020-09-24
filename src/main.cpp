#include <Arduino.h>
#include <WiFi.h>
#include "math.h"

#include <OneWire.h>
#include <DallasTemperature.h>

#include "temp_control.hpp"

#define TEMP_SENSOR_PIN 2
#define RELAY_WARMING_ON 0
#define RELAY_COOLING_ON 4
#define LOOP_TIME 5.0

#define NETWORK_NAME "GregoriaNet"
#define NETWORK_PSWD "HansTiberioNacioEnEl2018"

// GLOBALS
OneWire oneWire(TEMP_SENSOR_PIN);
DallasTemperature sensors(&oneWire);
DeviceAddress insideThermometer;

void print_sensor_address(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16)
      Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

void temp_sensor_start()
{
  sensors.begin();

  Serial.print("Found ");
  Serial.print(sensors.getDS18Count(), DEC);
  Serial.println(" DS18 sensor(s).");

  if (!sensors.getAddress(insideThermometer, 0))
  {
    Serial.println("Unable to find address for Device 0");
  }
  Serial.print("Device 0 Address: ");
  print_sensor_address(insideThermometer);
  Serial.println();
}

float temp_get()
{
  sensors.requestTemperatures();
  float temp = sensors.getTempC(insideThermometer);
  return temp;
};

float temp_read_period = 30.0; // Seconds betwing temperature reads
time_t last_temp_read;
time_t current_time;

float ref_temp = 25.0;
float th_temp = 5.0;
float current_temp;

struct temp_control_handle_t tc_handle = {
    .relay_warming_pin = RELAY_WARMING_ON,
    .relay_cooling_pin = RELAY_COOLING_ON,
    .warming_on = false,
    .cooling_on = false};

void setup()
{
  // Initialize timer
  time(&last_temp_read);

  Serial.begin(115200);

  temp_sensor_start();

  temp_control_configure(&tc_handle);

  // Connect to Wifi
  WiFi.begin(NETWORK_NAME, NETWORK_PSWD);
  Serial.printf("Connecting to %s ", NETWORK_NAME);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print("*");
  }
  delay(100);
  Serial.print(" DONE!\n");
}

void loop()
{
  time(&current_time);

  if ((current_time - last_temp_read) >= temp_read_period)
  {
    // GET TEMPERATURES
    current_temp = temp_get();

    float temp_diff = ref_temp - current_temp;

    Serial.printf("Reference temperature:     % 8.3f C\n", ref_temp);
    Serial.printf("Current temperature:       % 8.3f C\n", current_temp);
    Serial.printf("Temperature difference:    % 8.3f C\n", temp_diff);

    if (abs(temp_diff) > abs(th_temp))
    {
      if (temp_diff > 0.0)
      {
        temp_control_set_warming(&tc_handle);
      }
      else
      {
        temp_control_set_cooling(&tc_handle);
      }
    }
    else
    {
      temp_control_set_off(&tc_handle);
    }

    temp_control_perform(&tc_handle);

    time(&last_temp_read);
  }

  sleep(LOOP_TIME);
}