#include "power.h"

void enableCharger()
{
    pinMode(GPIO_NUM_10, OUTPUT);
    // gpio_hold_dis(GPIO_NUM_10); // Disable hold if it was enabled previously
    digitalWrite(GPIO_NUM_10, HIGH);
    gpio_hold_en(GPIO_NUM_10);
    gpio_deep_sleep_hold_en();
}

void disableCharger()
{
    pinMode(GPIO_NUM_10, OUTPUT);
    gpio_hold_dis(GPIO_NUM_10); // Disable hold if it was enabled previously
    digitalWrite(GPIO_NUM_10, LOW);
    gpio_hold_en(GPIO_NUM_10);
    gpio_deep_sleep_hold_en();
}
