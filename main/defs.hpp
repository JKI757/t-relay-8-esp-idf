#ifndef DEFS_HPP_
#define DEFS_HPP_

#include "hal/gpio_types.h"

constexpr gpio_num_t RELAY_PIN_1  = GPIO_NUM_33;
constexpr gpio_num_t RELAY_PIN_2  = GPIO_NUM_32;
constexpr gpio_num_t RELAY_PIN_3  = GPIO_NUM_13;
constexpr gpio_num_t RELAY_PIN_4  = GPIO_NUM_12;
constexpr gpio_num_t RELAY_PIN_5  = GPIO_NUM_21;
constexpr gpio_num_t RELAY_PIN_6  = GPIO_NUM_19;
constexpr gpio_num_t RELAY_PIN_7  = GPIO_NUM_18;
constexpr gpio_num_t RELAY_PIN_8  = GPIO_NUM_5;
constexpr gpio_num_t LED_PIN      = GPIO_NUM_25;

constexpr gpio_num_t relay_pins[] = {RELAY_PIN_1, RELAY_PIN_2, RELAY_PIN_3, RELAY_PIN_4, RELAY_PIN_5, RELAY_PIN_6, RELAY_PIN_7, RELAY_PIN_8};



#endif