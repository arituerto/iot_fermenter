#include "esp_err.h"
#include "ESPAsyncWebServer.h"
#include "AsyncTCP.h"

class FermenterServer
{
private:
    AsyncWebServer server = AsyncWebServer(80);
    volatile float ref_temp = 25.0;
    volatile float th_temp = 3.0;
    volatile float loop_time = 30.0;

public:
    FermenterServer() { ; };
    ~FermenterServer() { ; };
    void start();
    void stop();
    void set_ref_temp(float value);
    float get_ref_temp();
    void set_th_temp(float value);
    float get_th_temp();
    void set_loop_time(float value);
    float get_loop_time();
};