

#include <OneWire.h>
#include <DallasTemperature.h>

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
    float current_temp;
    int relay_warming_pin;
    int relay_cooling_pin;
    bool warming_on;
    bool cooling_on;
};

void set_temp_th(struct temp_control_handle_t *tc_handle, float th_temp);

void set_temp_ref(struct temp_control_handle_t *tc_handle, float ref_temp);

void print_sensor_address(DeviceAddress deviceAddress);

void temp_sensor_start(struct temp_sensor_handle_t *ts_handle, int temp_sensor_pin);

float temp_get(struct temp_sensor_handle_t *ts_handle);

void temp_control_configure(struct temp_control_handle_t *tc_handle);

void temp_control_set_warming(struct temp_control_handle_t *tc_handle);

void temp_control_set_cooling(struct temp_control_handle_t *tc_handle);

void temp_control_set_off(struct temp_control_handle_t *tc_handle);

void temp_control_run(struct temp_sensor_handle_t *ts_handle, struct temp_control_handle_t *tc_handle);
