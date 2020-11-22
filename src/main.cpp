#include <Arduino.h>
#include <WiFi.h>
#include "math.h"
#include "ESPAsyncWebServer.h"
#include "AsyncTCP.h"
#include "esp32-hal-log.h"

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#include "temp_control.hpp"

// TODO: Add ESP_LOG and ESP_ERR. Use functions output for check success. handle setters
// TODO: Define state struct to serialize
// TODO: Define fermentation profile: time, temp pairs

#define TEMP_SENSOR_PIN 33

#define RELAY_A 19
#define RELAY_B 18
#define RELAY_C 21

#define LOOP_TIME 5.0

#define ESP32_NETWORK_NAME "ESP32_fermenter"
#define ESP32_NETWORK_PSWD "a123456789*"

#define NETWORK_NAME "GregoriaNet"
#define NETWORK_PSWD "HansTiberioNacioEnEl2018"

// AP VARIABLES
IPAddress local_IP(10, 0, 0, 1);
IPAddress gateway_IP(10, 0, 0, 1);
IPAddress subnet(255, 255, 0, 0);

//SERVER READING VARIABLES
volatile float server_ref_temp = 25.0;
volatile float server_th_temp = 3.0;
volatile float server_loop_time = 30.0;
volatile bool server_fermenter_on = false;

// SERVER INSTANCE
AsyncWebServer server(80);

// TEMP CONTROL INSTANCE
struct temp_control_handle_t tc_handle = {
    .ref_temp = 43.0,
    .th_temp = 3.0,
    .relay_a = RELAY_A,
    .relay_b = RELAY_B,
    .relay_c = RELAY_C,
    .warming_on = false,
    .cooling_on = false,
    .mode_change = false};

// TEMP SENSOR INSTANCE
struct temp_sensor_handle_t ts_handle;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>IoT Fermenter Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {font-family: Helvetivca; display: inline-block; text-align: center;}
    h1 {font-size: 3.0rem;}
    h2 {font-size: 2.0rem;}
    p {font-size: 1.0rem;}
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
  <h1>IoT Fermenter Web Server</h1>
  %CURRENTSTATE%
  %FORMPLACEHOLDER%
<script>function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?output="+element.id+"&state=1", true); }
  else { xhr.open("GET", "/update?output="+element.id+"&state=0", true); }
  xhr.send();
}
</script>
</body>
</html>
)rawliteral";

String float2string(float val)
{
  char output[5];
  sprintf(output, "%.2f", val);
  return output;
}

String bool2string(bool val)
{
  return val ? "checked" : "";
}

String processor(const String &var)
{
  if (var == "FORMPLACEHOLDER")
  {
    String output = "";
    output += "<form action=\"/get\"> Reference Temperature: <input type=\"number\" name=\"ref_temp\" step=\"0.01\" min=\"10.0\" max=\"65.0\" value=\"" + float2string(server_ref_temp) + "\" size=\"20\"> <input type=\"submit\" value=\"Submit\"> </form>";
    output += "<form action=\"/get\"> Temperature threshold: <input type=\"number\" name=\"th_temp\" step=\"0.01\" min=\"0.5\" max=\"15.0\" value=\"" + float2string(server_th_temp) + "\" size=\"20\"> <input type=\"submit\" value=\"Submit\"> </form>";
    output += "<form action=\"/get\"> Loop time: <input type=\"number\" name=\"loop_time\" step=\"0.01\" min=\"5.0\" max=\"300.0\" value=\"" + float2string(server_loop_time) + "\" size=\"20\"> <input type=\"submit\" value=\"Submit\"> </form>";
    output += "<h4>Fermenting!!</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"fermenter_on\" " + bool2string(server_fermenter_on) + "><span class=\"slider\"></span></label>";
    return output;
  }
  else if (var == "CURRENTSTATE")
  {
    String output = "";
    output += "<div style=\"border: 1px solid black\">";
    output += "<h1>Current state:</h1>";
    output += "<p><b>Temperature</b>: " + float2string(temp_get(&ts_handle)) + "</p>";
    output += "<p><b>Ref. Temperature</b>: " + float2string(tc_handle.ref_temp) + "</p>";
    if (tc_handle.warming_on)
    {
      output += "<p>Peltier <b>WARMING</b></p>";
    }
    else if (tc_handle.cooling_on)
    {
      output += "<p>Peltier <b>COOLING</b></p>";
    }
    else
    {
      output += "<p>Peltier <b>OFF</b></p>";
    }
    output += "</div>";
    return output;
  }

  return "";
};

void start()
{
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html, processor);
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
    ESP_LOGD("SERVER", "/GET %s - %s : %5.2f\n", inputParam, inputMessage, inputValue);
    request->redirect("/");
    // request->send(200, "text/html", "OK");
  });

  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
    String output;
    String state;
    if (request->hasParam("output") && request->hasParam("state"))
    {
      output = request->getParam("output")->value();
      state = request->getParam("state")->value();
      if (output == "fermenter_on")
      {
        server_fermenter_on = state.toInt();
        if (server_fermenter_on == 0)
        {
          temp_control_set_off(&tc_handle);
        }
      }
    }
    else
    {
      output = "none";
      state = "none";
    }
    ESP_LOGD("SERVER", "/UPDATE %s: %s\n", output, state);
    request->redirect("/");
    // request->send(200, "text/plain", "OK");
  });

  server.begin();
}

void create_wifi(const char *network_name, const char *network_pswd)
{

  ESP_LOGI("WIFI", "Setting soft-AP configuration ... ");
  ESP_LOGI("WIFI", "%s", WiFi.softAPConfig(local_IP, gateway_IP, subnet) ? "Ready" : "Failed!");

  ESP_LOGI("WIFI", "Setting soft-AP ... ");
  ESP_LOGI("WIFI", "%s", WiFi.softAP(network_name, network_pswd) ? "Ready" : "Failed!");

  ESP_LOGI("WIFI", "Soft-AP IP address = ");
  ESP_LOGI("WIFI", "%s", WiFi.softAPIP());
}

void connect_wifi(const char *network_name, const char *network_pswd)
{
  // Connect to Wifi
  WiFi.begin(network_name, network_pswd);
  ESP_LOGI("WIFI", "Connecting to %s ...", network_name);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
  }
  delay(100);
  ESP_LOGI("WIFI", " DONE!\n");

  ESP_LOGI("WIFI", "IoT fermenter IP: ");
  ESP_LOGI("WIFI", "%s", WiFi.localIP());
}

void setup()
{

  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  Serial.begin(115200);

  temp_sensor_start(&ts_handle, TEMP_SENSOR_PIN);
  temp_control_configure(&tc_handle);
  
  // create_wifi(ESP32_NETWORK_NAME, ESP32_NETWORK_PSWD);
  connect_wifi(NETWORK_NAME, NETWORK_PSWD);

  start();
}

void loop()
{
  float ref_temp = server_ref_temp;
  set_temp_ref(&tc_handle, ref_temp);

  float th_temp = server_th_temp;
  set_temp_th(&tc_handle, th_temp);

  temp_control_run(&ts_handle, &tc_handle, server_fermenter_on);

  ESP_LOGD("LOOP", "server_fermenter_on = %s\n", server_fermenter_on ? "true" : "false");

  float loop_time = server_loop_time;
  sleep(loop_time);
}