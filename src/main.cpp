/**
 * @author: Alejandro Rituerto
 **/
#include <WiFi.h>
#include <SPIFFS.h>
#include "math.h"
#include "ESPAsyncWebServer.h"
#include "AsyncTCP.h"
#include "esp32-hal-log.h"
#include "mdns.h"

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#include "temp_control.hpp"

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
volatile bool server_control_active = false;
volatile bool server_warming_only = false;
volatile bool server_cooling_only = false;

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

String float2string(float val)
{
  char output[20];
  sprintf(output, "%.2f", val);
  return output;
}

String bool2string(bool val)
{
  return val ? "checked" : "";
}

void start_mdns_service()
{
  //initialize mDNS service
  esp_err_t err = mdns_init();
  if (err)
  {
    ESP_LOGE("MDNS", "MDNS Init failed: %d\n", err);
    return;
  }

  //set hostname
  mdns_hostname_set("iotfermenter");
  //set default instance
  mdns_instance_name_set("IoT Fermenter");
}

void start()
{
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html", "text/html");
    ESP_LOGD("SERVER", "GET /");
  });

  server.on("/js/bootstrap.bundle.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/js/bootstrap.bundle.min.js", "text/javascript");
    ESP_LOGD("SERVER", "GET /js/bootstrap.bundle.min.js");
  });

  server.on("/js/jquery-3.5.1.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/js/jquery-3.5.1.min.js", "text/javascript");
    ESP_LOGD("SERVER", "GET /js/jquery-3.5.1.min.js");
  });

  server.on("/css/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/css/bootstrap.min.css", "text/css");
    ESP_LOGD("SERVER", "GET /css/bootstrap.min.css");
  });

  server.on("/current_temp", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", float2string(temp_get(&ts_handle)));
    ESP_LOGD("SERVER", "GET /current_temp");
  });

  server.on("/ref_temp", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", float2string(server_ref_temp));
    ESP_LOGD("SERVER", "GET /ref_temp");
  });

  server.on("/th_temp", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", float2string(server_th_temp));
    ESP_LOGD("SERVER", "GET /th_temp");
  });

  server.on("/loop_time", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", float2string(server_loop_time));
    ESP_LOGD("SERVER", "GET /loop_time");
  });

  server.on("/set", HTTP_GET, [](AsyncWebServerRequest *request) {
    String inputMessage;
    float inputValue;
    String inputParam;

    ESP_LOGD("SERVER", "GET /set call received!");

    if (request->hasParam("ref_temp"))
    {
      inputMessage = request->getParam("ref_temp")->value();
      inputValue = atof(inputMessage.c_str());
      server_ref_temp = inputValue;
      inputParam = "ref_temp";
      ESP_LOGD("SERVER", "GET /set %s - %s : %5.2f", inputParam, inputMessage, inputValue);
    }
    if (request->hasParam("th_temp"))
    {
      inputMessage = request->getParam("th_temp")->value();
      inputValue = atof(inputMessage.c_str());
      server_th_temp = inputValue;
      inputParam = "th_temp";
      ESP_LOGD("SERVER", "GET /set %s - %s : %5.2f", inputParam, inputMessage, inputValue);
    }
    if (request->hasParam("loop_time"))
    {
      inputMessage = request->getParam("loop_time")->value();
      inputValue = atof(inputMessage.c_str());
      server_loop_time = inputValue;
      inputParam = "loop_time";
      ESP_LOGD("SERVER", "GET /set %s - %s : %5.2f", inputParam, inputMessage, inputValue);
    }
    // request->send(200);
    request->redirect("/");
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
        server_control_active = state.toInt();
        if (server_control_active == 0)
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
    ESP_LOGD("SERVER", "GET /update %s: %s\n", output, state);
    request->send(200);
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
  ESP_LOGI("WIFI", "%c", WiFi.localIP().toString());
}

void setup()
{

  Serial.begin(115200);

  if (!SPIFFS.begin())
  {
    ESP_LOGE(TAG, "An Error has occurred while mounting SPIFFS");
    return;
  }
  {
    ESP_LOGI(TAG, "FILE LIST:");
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while (file)
    {
      ESP_LOGI(TAG, "  %s", file.name());
      file = root.openNextFile();
    }
    file.close();
    root.close();
  }

  temp_sensor_start(&ts_handle, TEMP_SENSOR_PIN);
  temp_control_configure(&tc_handle);

  // create_wifi(ESP32_NETWORK_NAME, ESP32_NETWORK_PSWD);
  connect_wifi(NETWORK_NAME, NETWORK_PSWD);

  // start_mdns_service();

  start();
}

void loop()
{
  float ref_temp = server_ref_temp;
  set_temp_ref(&tc_handle, ref_temp);

  float th_temp = server_th_temp;
  set_temp_th(&tc_handle, th_temp);

  temp_control_run(&ts_handle, &tc_handle, server_control_active);

  ESP_LOGD("LOOP", "server_control_active = %s\n", server_control_active ? "true" : "false");

  float loop_time = server_loop_time;
  sleep(loop_time);
}