
#include "temp_control.hpp"

#define PELTIER_MODE_CHANGE_DELAY 10.0

fermenter_state_t get_fermenter_state(struct temp_sensor_handle_t *ts_handle, struct temp_control_handle_t *tc_handle)
{
    fermenter_state_t output;
    time(&output.time);
    output.temp = temp_get(ts_handle);
    output.ref_temp = tc_handle->ref_temp;
    if ((tc_handle->cooling_on) & (tc_handle->warming_on))
    {
        output.state = error;
    }
    else if (tc_handle->warming_on)
    {
        output.state = warming;
    }
    else if (tc_handle->warming_on)
    {
        output.state = cooling;
    }
    else
    {
        output.state = off;
    }

    return output;
}

void print_sensor_address(DeviceAddress deviceAddress)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        if (deviceAddress[i] < 16)
            Serial.print("0");
        Serial.print(deviceAddress[i], HEX);
    }
}

void temp_sensor_start(struct temp_sensor_handle_t *ts_handle, int temp_sensor_pin)
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

float temp_get(struct temp_sensor_handle_t *ts_handle)
{
    ts_handle->sensors.requestTemperatures();
    float temp = ts_handle->sensors.getTempC(ts_handle->insideThermometer);
    return temp;
};

void temp_control_configure(struct temp_control_handle_t *tc_handle)
{

    Serial.print("Initializing control ");
    pinMode(tc_handle->relay_a, OUTPUT);
    pinMode(tc_handle->relay_b, OUTPUT);
    pinMode(tc_handle->relay_c, OUTPUT);
    digitalWrite(tc_handle->relay_a, HIGH);
    digitalWrite(tc_handle->relay_b, HIGH);
    digitalWrite(tc_handle->relay_c, HIGH);
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
    if (!tc_handle->warming_on)
    {
        tc_handle->cooling_on = false;
        tc_handle->warming_on = true;
        tc_handle->mode_change = true;
        Serial.println("CONTROL SET TO WARMING");
    }
}

void temp_control_set_cooling(struct temp_control_handle_t *tc_handle)
{
    if (!tc_handle->cooling_on)
    {
        tc_handle->warming_on = false;
        tc_handle->cooling_on = true;
        tc_handle->mode_change = true;
        Serial.println("CONTROL SET TO COOLING");
    }
}

void temp_control_set_off(struct temp_control_handle_t *tc_handle)
{
    if ((tc_handle->warming_on) || (tc_handle->cooling_on))
    {
        tc_handle->warming_on = false;
        tc_handle->cooling_on = false;
        tc_handle->mode_change = true;
        Serial.println("CONTROL SET OFF");
    }
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

    if (tc_handle->mode_change)
    {
        Serial.print("Turning OFF peltier cell");
        digitalWrite(tc_handle->relay_a, HIGH);
        digitalWrite(tc_handle->relay_b, HIGH);
        digitalWrite(tc_handle->relay_c, HIGH);
        Serial.println(" DONE");

        sleep(PELTIER_MODE_CHANGE_DELAY);

        if (tc_handle->warming_on & tc_handle->cooling_on)
        {
            Serial.println("Temperature control error - warming and cooling ON!");
        }
        else
        {
            if (tc_handle->warming_on)
            {
                Serial.print("Turning ON warming");
                digitalWrite(tc_handle->relay_a, LOW);
                digitalWrite(tc_handle->relay_b, HIGH);
                digitalWrite(tc_handle->relay_c, LOW);
                Serial.println(" DONE");
            }
            else if (tc_handle->cooling_on)
            {
                Serial.print("Turning ON cooling");
                digitalWrite(tc_handle->relay_a, HIGH);
                digitalWrite(tc_handle->relay_b, LOW);
                digitalWrite(tc_handle->relay_c, LOW);
                Serial.println(" DONE");
            }
        }
        tc_handle->mode_change = false;
    }
}
