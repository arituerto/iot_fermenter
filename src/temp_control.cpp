
#include "temp_control.hpp"

#include <Arduino.h>

void temp_control_configure(struct temp_control_handle_t *temp_control_handle)
{
    pinMode(temp_control_handle->relay_warming_pin, INPUT);
    pinMode(temp_control_handle->relay_cooling_pin, INPUT);
}

void temp_control_perform(struct temp_control_handle_t *temp_control_handle)
{
    if (temp_control_handle->warming_on & temp_control_handle->cooling_on)
    {
        Serial.println("Temperature control error - warming and cooling ON!");
    }
    else
    {
        if (temp_control_handle->warming_on)
        {
            Serial.print("Turning OFF cooling");
            digitalWrite(temp_control_handle->relay_cooling_pin, LOW);
            Serial.println(" DONE");
            sleep(10.0);
            Serial.print("Turning ON warming");
            digitalWrite(temp_control_handle->relay_warming_pin, HIGH);
            Serial.println(" DONE");
        }
        else if (temp_control_handle->cooling_on)
        {
            Serial.print("Turning OFF warming");
            digitalWrite(temp_control_handle->relay_warming_pin, LOW);
            Serial.println(" DONE");
            sleep(10.0);
            Serial.print("Turning ON cooling");
            digitalWrite(temp_control_handle->relay_cooling_pin, HIGH);
            Serial.println(" DONE");
        }
    }
}

void temp_control_set_warming(struct temp_control_handle_t *temp_control_handle)
{
    temp_control_handle->cooling_on = false;
    temp_control_handle->warming_on = true;
    Serial.println("CONTROL SET TO WARMING");
}

void temp_control_set_cooling(struct temp_control_handle_t *temp_control_handle)
{
    temp_control_handle->warming_on = false;
    temp_control_handle->cooling_on = true;
    Serial.println("CONTROL SET TO COOLING");
}

void temp_control_set_off(struct temp_control_handle_t *temp_control_handle)
{
    temp_control_handle->warming_on = false;
    temp_control_handle->cooling_on = false;
    Serial.println("CONTROL SET OFF");
}
