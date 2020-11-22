

#include <OneWire.h>
#include <DallasTemperature.h>

enum ControlState
{
    off,
    warming,
    cooling,
    error
};

struct fermenter_state_t
{
    time_t time;
    float temp;
    float ref_temp;
    ControlState state;
};

struct temp_sensor_handle_t
{
    OneWire oneWire;
    DallasTemperature sensors;
    DeviceAddress insideThermometer;
};

struct temp_control_handle_t
{
    float ref_temp;
    float th_temp;
    int relay_a;
    int relay_b;
    int relay_c;
    bool warming_on;
    bool cooling_on;
    bool mode_change;
};

fermenter_state_t get_fermenter_state(struct temp_sensor_handle_t *ts_handle, struct temp_control_handle_t *tc_handle);

void print_sensor_address(DeviceAddress deviceAddress);

void set_temp_th(struct temp_control_handle_t *tc_handle, float th_temp);

void set_temp_ref(struct temp_control_handle_t *tc_handle, float ref_temp);

void temp_sensor_start(struct temp_sensor_handle_t *ts_handle, int temp_sensor_pin);

float temp_get(struct temp_sensor_handle_t *ts_handle);

void temp_control_configure(struct temp_control_handle_t *tc_handle);

void temp_control_set_warming(struct temp_control_handle_t *tc_handle);

void temp_control_set_cooling(struct temp_control_handle_t *tc_handle);

void temp_control_set_off(struct temp_control_handle_t *tc_handle);

void temp_control_run(struct temp_sensor_handle_t *ts_handle, struct temp_control_handle_t *tc_handle, bool control_on);
