#include <Arduino.h>
#include <WiFi.h>
#include "math.h"
#include "ESPAsyncWebServer.h"
#include "AsyncTCP.h"

#include "temp_control.hpp"

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

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>IoT Fermenter Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 6px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 3px}
    input:checked+.slider {background-color: #b30000}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
<body>
  <h2>IoT Fermenter Web Server</h2>
  <form action="/get">
    Reference Temperature: <input type="number" name="ref_temp" step="0.01" min="10.0" max="65.0">
    <input type="submit" value="Submit">
  </form>
  <form action="/get">
    Temperature threshold: <input type="number" name="th_temp" step="0.01" min="0.5" max="15.0">
    <input type="submit" value="Submit">
  </form>
  <form action="/get">
    Loop time: <input type="number" name="loop_time" step="0.01" min="5.0" max="300.0">
    <input type="submit" value="Submit">
  </form>
</body>
</html>
)rawliteral";

IPAddress local_IP(10, 0, 0, 1);
IPAddress gateway_IP(10, 0, 0, 1);
IPAddress subnet(255, 255, 0, 0);

volatile float server_ref_temp = 25.0;
volatile float server_th_temp = 3.0;
volatile float server_loop_time = 30.0;

AsyncWebServer server(80);

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

void start()
{
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });

  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request) {
    String inputMessage;
    float inputValue;
    String inputParam;

    if (request->hasParam("ref_temp"))
    {
      inputMessage = request->getParam("ref_temp")->value();
      inputValue = atof(inputMessage.c_str());
      server_ref_temp = inputValue;
      inputParam = "ref_temp";
    }
    else if (request->hasParam("th_temp"))
    {
      inputMessage = request->getParam("th_temp")->value();
      inputValue = atof(inputMessage.c_str());
      server_th_temp = inputValue;
      inputParam = "th_temp";
    }
    else if (request->hasParam("loop_time"))
    {
      inputMessage = request->getParam("loop_time")->value();
      inputValue = atof(inputMessage.c_str());
      server_loop_time = inputValue;
      inputParam = "loop_time";
    }
    else
    {
      inputMessage = "No message sent";
      inputValue = NAN;
      inputParam = "none";
    }
    Serial.printf("/GET %s - %s : %5.2f\n", inputParam, inputMessage, inputValue);
    request->send(200, "text/html", index_html);
  });
  
  server.begin(); 
}

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

  start();
}

void loop()
{
  float ref_temp = server_ref_temp;
  set_temp_ref(&tc_handle, ref_temp);
  float th_temp = server_th_temp;
  set_temp_th(&tc_handle, th_temp);

  temp_control_run(&ts_handle, &tc_handle);

  float loop_time = server_loop_time;
  sleep(loop_time);
}