
#include "temp_control.hpp"

#include <Arduino.h>

#define PELTIER_MODE_CHANGE_DELAY 10.0

void print_sensor_address(DeviceAddress deviceAddress)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        if (deviceAddress[i] < 16)
            Serial.print("0");
        Serial.print(deviceAddress[i], HEX);
    }
}

void temp_sensor_start(temp_sensor_handle_t *ts_handle, int temp_sensor_pin)
{
    Serial.print("Initializing temperature sensor ");
    ts_handle->oneWire.begin(temp_sensor_pin);
    ts_handle->sensors.setOneWire(&ts_handle->oneWire);
    ts_handle->sensors.begin();

    Serial.print("Found ");
    Serial.print(ts_handle->sensors.getDS18Count(), DEC);
    Serial.println(" DS18 sensor(s).");

    if (!ts_handle->sensors.getAddress(ts_handle->insideThermometer, 0))
    {
        Serial.println("Unable to find address for Device 0");
    }
    Serial.print("Device 0 Address: ");
    print_sensor_address(ts_handle->insideThermometer);
    Serial.println();

    Serial.print("DONE!\n");
}

float temp_get(temp_sensor_handle_t *ts_handle)
{
    ts_handle->sensors.requestTemperatures();
    float temp = ts_handle->sensors.getTempC(ts_handle->insideThermometer);
    return temp;
};

void temp_control_configure(struct temp_control_handle_t *tc_handle)
{

    Serial.print("Initializing control ");
    pinMode(tc_handle->relay_warming_pin, INPUT);
    pinMode(tc_handle->relay_cooling_pin, INPUT);
    Serial.print("DONE!\n");
}

void set_temp_th(struct temp_control_handle_t *tc_handle, float th_temp)
{
    tc_handle->th_temp = th_temp;
}

void set_temp_ref(struct temp_control_handle_t *tc_handle, float ref_temp)
{
    tc_handle->ref_temp = ref_temp;
}

void temp_control_set_warming(struct temp_control_handle_t *tc_handle)
{
    tc_handle->cooling_on = false;
    tc_handle->warming_on = true;
    Serial.println("CONTROL SET TO WARMING");
}

void temp_control_set_cooling(struct temp_control_handle_t *tc_handle)
{
    tc_handle->warming_on = false;
    tc_handle->cooling_on = true;
    Serial.println("CONTROL SET TO COOLING");
}

void temp_control_set_off(struct temp_control_handle_t *tc_handle)
{
    tc_handle->warming_on = false;
    tc_handle->cooling_on = false;
    Serial.println("CONTROL SET OFF");
}

void temp_control_run(struct temp_sensor_handle_t *ts_handle, struct temp_control_handle_t *tc_handle)
{
    // GET TEMPERATURES
    float current_temp = temp_get(ts_handle);

    float temp_diff = tc_handle->ref_temp - current_temp;

    Serial.printf("Reference temperature:     % 8.3f C\n", tc_handle->ref_temp);
    Serial.printf("Current temperature:       % 8.3f C\n", current_temp);
    Serial.printf("Temperature difference:    % 8.3f C\n", temp_diff);

    if (abs(temp_diff) > abs(tc_handle->th_temp))
    {
        if (temp_diff > 0.0)
        {
            temp_control_set_warming(tc_handle);
        }
        else
        {
            temp_control_set_cooling(tc_handle);
        }
    }
    else
    {
        temp_control_set_off(tc_handle);
    }

    if (tc_handle->warming_on & tc_handle->cooling_on)
    {
        Serial.println("Temperature control error - warming and cooling ON!");
    }
    else
    {
        if (tc_handle->warming_on)
        {
            Serial.print("Turning OFF cooling");
            digitalWrite(tc_handle->relay_cooling_pin, LOW);
            Serial.println(" DONE");
            sleep(PELTIER_MODE_CHANGE_DELAY);
            Serial.print("Turning ON warming");
            digitalWrite(tc_handle->relay_warming_pin, HIGH);
            Serial.println(" DONE");
        }
        else if (tc_handle->cooling_on)
        {
            Serial.print("Turning OFF warming");
            digitalWrite(tc_handle->relay_warming_pin, LOW);
            Serial.println(" DONE");
            sleep(PELTIER_MODE_CHANGE_DELAY);
            Serial.print("Turning ON cooling");
            digitalWrite(tc_handle->relay_cooling_pin, HIGH);
            Serial.println(" DONE");
        }
    }
}
