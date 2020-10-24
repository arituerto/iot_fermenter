#include "fermenter_server.hpp"

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

void notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Not found");
}

void FermenterServer::set_ref_temp(float value)
{
    ref_temp = value;
}
float FermenterServer::get_ref_temp()
{
    return ref_temp;
}
void FermenterServer::set_th_temp(float value)
{
    th_temp = value;
}
float FermenterServer::get_th_temp()
{
    return th_temp;
}
void FermenterServer::set_loop_time(float value)
{
    loop_time = value;
}
float FermenterServer::get_loop_time()
{
    return loop_time;
}

void FermenterServer::start()
{
    server.onNotFound(notFound);

    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", index_html);
    });

    server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request) {
        String inputMessage;
        float inputValue;
        String inputParam;

        // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
        if (request->hasParam("ref_temp"))
        {
            inputMessage = request->getParam("ref_temp")->value();
            inputValue = atof(inputMessage.c_str());
            // set_ref_temp(inputValue);
            inputParam = "ref_temp";
        }
        // GET input2 value on <ESP_IP>/get?input2=<inputMessage>
        else if (request->hasParam("th_temp"))
        {
            inputMessage = request->getParam("th_temp")->value();
            inputValue = atof(inputMessage.c_str());
            // set_th_temp(inputValue);
            inputParam = "th_temp";
        }
        // GET input3 value on <ESP_IP>/get?input3=<inputMessage>
        else if (request->hasParam("loop_time"))
        {
            inputMessage = request->getParam("loop_time")->value();
            inputValue = atof(inputMessage.c_str());
            // set_loop_time(inputValue);
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

    // Start server
    server.begin();
}

void FermenterServer::stop()
{
    server.end();
}