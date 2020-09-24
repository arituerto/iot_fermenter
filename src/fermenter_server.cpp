#include "esp_http_server.h"
#include "temp_control.hpp"
#include "esp_err.h"

struct fermenter_state_log_t
{
    time_t start_time;
    time_t current_time;
    bool warming_on;
    bool cooling_on;
    float ref_temp;
    float current_temp;
};

struct fermenter_config_t {
    float ref_temp;
    float th_temp;
};

struct fermenter_state_log_t hist[1024];

