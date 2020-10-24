#include <Arduino.h>
#include <WiFi.h>
#include "math.h"

#include "temp_control.hpp"
#include "fermenter_server.hpp"

// TODO: Add ESP_LOG and ESP_ERR. Use functions output for check success. handle setters
// TODO: Define state struct to serialize
// TODO: Define fermentation profile: time, temp pairs

#define TEMP_SENSOR_PIN 25

#define RELAY_A 32
#define RELAY_B 33
#define RELAY_C 14

#define LOOP_TIME 5.0

#define ESP32_NETWORK_NAME "ESP32_fermenter"
#define ESP32_NETWORK_PSWD "a123456789*"

#define NETWORK_NAME "GregoriaNet"
#define NETWORK_PSWD "HansTiberioNacioEnEl2018"

IPAddress local_IP(10, 0, 0, 1);
IPAddress gateway_IP(10, 0, 0, 1);
IPAddress subnet(255, 255, 0, 0);

struct temp_control_handle_t tc_handle = {
    .ref_temp = 43.0,
    .th_temp = 3.0,
    .relay_a = RELAY_A,
    .relay_b = RELAY_B,
    .relay_c = RELAY_C,
    .warming_on = false,
    .cooling_on = false,
    .mode_change = false};

struct temp_sensor_handle_t ts_handle;

FermenterServer f_server;

void create_wifi(const char *network_name, const char *network_pswd)
{

  Serial.print("Setting soft-AP configuration ... ");
  Serial.println(WiFi.softAPConfig(local_IP, gateway_IP, subnet) ? "Ready" : "Failed!");

  Serial.print("Setting soft-AP ... ");
  Serial.println(WiFi.softAP(network_name, network_pswd) ? "Ready" : "Failed!");

  Serial.print("Soft-AP IP address = ");
  Serial.println(WiFi.softAPIP());
}

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

  Serial.print("IoT fermenter IP: ");
  Serial.println(WiFi.localIP());
}

void setup()
{
  Serial.begin(115200);

  // create_wifi(ESP32_NETWORK_NAME, ESP32_NETWORK_PSWD);
  connect_wifi(NETWORK_NAME, NETWORK_PSWD);
  temp_sensor_start(&ts_handle, TEMP_SENSOR_PIN);
  temp_control_configure(&tc_handle);

  f_server.start();
}

void loop()
{
  set_temp_ref(&tc_handle, f_server.get_ref_temp());
  set_temp_th(&tc_handle, f_server.get_th_temp());

  temp_control_run(&ts_handle, &tc_handle);

  sleep(f_server.get_loop_time());
}