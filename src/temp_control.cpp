
/**
 * @author: Alejandro Rituerto
 **/

#include "temp_control.hpp"

#define PELTIER_MODE_CHANGE_DELAY 10.0

fermenter_state_t get_fermenter_state(struct temp_sensor_handle_t *ts_handle, struct temp_control_handle_t *tc_handle)
{
    fermenter_state_t output;
    time(&output.time);
    output.temp = temp_get(ts_handle);
    output.ref_temp = tc_handle->ref_temp;
    if ((tc_handle->cooling_active) & (tc_handle->warming_active))
    {
        output.state = error;
    }
    else if (tc_handle->warming_active)
    {
        output.state = warming;
    }
    else if (tc_handle->warming_active)
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
    ESP_LOGI("TEMP_SENSOR", "Device adress: %02X %02X %02X %02X %02X %02X %02X %02X",
             deviceAddress[0],
             deviceAddress[1],
             deviceAddress[2],
             deviceAddress[3],
             deviceAddress[4],
             deviceAddress[5],
             deviceAddress[6],
             deviceAddress[7]);
}

void temp_sensor_start(struct temp_sensor_handle_t *ts_handle, int temp_sensor_pin)
{
    ESP_LOGI("TEMP_SENSOR", "Initializing temperature sensor ");
    ts_handle->oneWire.begin(temp_sensor_pin);
    ts_handle->sensors.setOneWire(&ts_handle->oneWire);
    ts_handle->sensors.begin();

    ESP_LOGI("TEMP_SENSOR", "Found ");
    ESP_LOGI("TEMP_SENSOR", "%d", ts_handle->sensors.getDS18Count(), DEC);
    ESP_LOGI("TEMP_SENSOR", " DS18 sensor(s).");

    if (!ts_handle->sensors.getAddress(ts_handle->insideThermometer, 0))
    {
        ESP_LOGE("TEMP_SENSOR", "Unable to find address for Device 0");
    }
    print_sensor_address(ts_handle->insideThermometer);

    ESP_LOGI("TEMP_SENSOR", "DONE!\n");
}

float temp_get(struct temp_sensor_handle_t *ts_handle)
{
    ts_handle->sensors.requestTemperatures();
    float temp = ts_handle->sensors.getTempC(ts_handle->insideThermometer);
    return temp;
};

void temp_control_configure(struct temp_control_handle_t *tc_handle)
{

    ESP_LOGI("TEMP_CONTROL", "Initializing control ");
    pinMode(tc_handle->relay_a, OUTPUT);
    pinMode(tc_handle->relay_b, OUTPUT);
    pinMode(tc_handle->relay_c, OUTPUT);
    digitalWrite(tc_handle->relay_a, HIGH);
    digitalWrite(tc_handle->relay_b, HIGH);
    digitalWrite(tc_handle->relay_c, LOW);
    ESP_LOGI("TEMP_CONTROL", "DONE!\n");
}

void set_temp_th(struct temp_control_handle_t *tc_handle, float th_temp)
{
    tc_handle->th_temp = th_temp;
}

void set_temp_ref(struct temp_control_handle_t *tc_handle, float ref_temp)
{
    tc_handle->ref_temp = ref_temp;
}

void set_control(struct temp_control_handle_t *tc_handle, bool control)
{
    if (tc_handle->control_on != control)
    {
        ESP_LOGI("TEMP_CONTROL", "Control change!");
    }
    tc_handle->control_on = control;
}

void set_warming_only(struct temp_control_handle_t *tc_handle, bool warming_only) { tc_handle->warming_only = warming_only; }

void set_cooling_only(struct temp_control_handle_t *tc_handle, bool cooling_only) { tc_handle->cooling_only = cooling_only; }

void temp_control_set_warming(struct temp_control_handle_t *tc_handle)
{
    if (!tc_handle->warming_active)
    {
        tc_handle->cooling_active = false;
        tc_handle->warming_active = true;
        tc_handle->mode_change = true;
        ESP_LOGI("TEMP_CONTROL", "CONTROL SET TO WARMING");
    }
}

void temp_control_set_cooling(struct temp_control_handle_t *tc_handle)
{
    if (!tc_handle->cooling_active)
    {
        tc_handle->warming_active = false;
        tc_handle->cooling_active = true;
        tc_handle->mode_change = true;
        ESP_LOGI("TEMP_CONTROL", "CONTROL SET TO COOLING");
    }
}

void temp_control_set_off(struct temp_control_handle_t *tc_handle)
{
    if ((tc_handle->warming_active) || (tc_handle->cooling_active))
    {
        tc_handle->warming_active = false;
        tc_handle->cooling_active = false;
        tc_handle->mode_change = true;
        ESP_LOGI("TEMP_CONTROL", "CONTROL SET OFF");
    }
}

void temp_control_run(struct temp_sensor_handle_t *ts_handle, struct temp_control_handle_t *tc_handle)
{
    // GET TEMPERATURES
    float current_temp = temp_get(ts_handle);

    float temp_diff = tc_handle->ref_temp - current_temp;

    if (tc_handle->control_on)
    {
        ESP_LOGI("TEMP_CONTROL", "TEMP. CONTROL ACTIVE");
    }
    ESP_LOGI("TEMP_CONTROL", "Reference temperature:     % 8.3f C", tc_handle->ref_temp);
    ESP_LOGI("TEMP_CONTROL", "Current temperature:       % 8.3f C", current_temp);
    ESP_LOGI("TEMP_CONTROL", "Temperature difference:    % 8.3f C (th: %5.2f C)", temp_diff, tc_handle->th_temp);
    if ((tc_handle->cooling_active) & (!tc_handle->warming_active))
    {
        ESP_LOGI("TEMP_CONTROL", "COOLING");
    }
    else if ((!tc_handle->cooling_active) & (tc_handle->warming_active))
    {
        ESP_LOGI("TEMP_CONTROL", "WARMING");
    }
    else
    {
        ESP_LOGI("TEMP_CONTROL", "OFF");
    }

    if (tc_handle->cooling_only)
    {
        ESP_LOGI("TEMP_CONTROL", "COOLING ONLY");
    }

    if (tc_handle->warming_only)
    {
        ESP_LOGI("TEMP_CONTROL", "WARMING ONLY");
    }

    if (tc_handle->control_on)
    {
        if (tc_handle->cooling_active)
        {
            if (current_temp < (tc_handle->ref_temp - 0.5 * tc_handle->th_temp))
            {
                temp_control_set_off(tc_handle);
            }
        }
        else if (tc_handle->warming_active)
        {
            if (current_temp > (tc_handle->ref_temp + 0.5 * tc_handle->th_temp))
            {
                temp_control_set_off(tc_handle);
            }
        }
        else
        {
            if (abs(temp_diff) > abs(tc_handle->th_temp))
            {
                if (temp_diff > 0.0)
                {
                    if (!tc_handle->cooling_only)
                    {
                        temp_control_set_warming(tc_handle);
                    }
                }
                else
                {
                    if (!tc_handle->warming_only)
                    {
                        temp_control_set_cooling(tc_handle);
                    }
                }
            }
            else
            {
                temp_control_set_off(tc_handle);
            }
        }
    }
    else
    {
        temp_control_set_off(tc_handle);
    }

    if (tc_handle->mode_change)
    {
        ESP_LOGI("TEMP_CONTROL", "Turning OFF peltier cell");
        digitalWrite(tc_handle->relay_a, HIGH);
        digitalWrite(tc_handle->relay_b, HIGH);
        digitalWrite(tc_handle->relay_c, LOW);
        ESP_LOGI("TEMP_CONTROL", " DONE");

        sleep(PELTIER_MODE_CHANGE_DELAY);

        if (tc_handle->warming_active & tc_handle->cooling_active)
        {
            ESP_LOGI("TEMP_CONTROL", "Temperature control error - warming and cooling ON!");
        }
        else
        {
            if (tc_handle->warming_active)
            {
                ESP_LOGI("TEMP_CONTROL", "Turning ON warming");
                digitalWrite(tc_handle->relay_a, LOW);
                digitalWrite(tc_handle->relay_b, HIGH);
                digitalWrite(tc_handle->relay_c, HIGH);
                ESP_LOGI("TEMP_CONTROL", " DONE");
            }
            else if (tc_handle->cooling_active)
            {
                ESP_LOGI("TEMP_CONTROL", "Turning ON cooling");
                digitalWrite(tc_handle->relay_a, HIGH);
                digitalWrite(tc_handle->relay_b, LOW);
                digitalWrite(tc_handle->relay_c, HIGH);
                ESP_LOGI("TEMP_CONTROL", " DONE");
            }
        }
        tc_handle->mode_change = false;
    }
}
