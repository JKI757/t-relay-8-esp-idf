#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <inttypes.h>


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"


#include "esp_rom_gpio.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_http_server.h"

#include "hal/emac_hal.h"
#include "hal/gpio_types.h"

#include "nvs_flash.h"


#include "defs.hpp"

#include "wifi-setup.hpp"

static const char *TAG = "ESP-R-8-IDF";



void toggle_GPIO(const gpio_num_t& gpio_pin);
void gpio_off(const gpio_num_t& gpio_pin);
void gpio_on(const gpio_num_t& gpio_pin);

void init_relays() {
    esp_rom_gpio_pad_select_gpio(RELAY_PIN_1);
    esp_rom_gpio_pad_select_gpio(RELAY_PIN_2);
    esp_rom_gpio_pad_select_gpio(RELAY_PIN_3);
    esp_rom_gpio_pad_select_gpio(RELAY_PIN_4);
    esp_rom_gpio_pad_select_gpio(RELAY_PIN_5);
    esp_rom_gpio_pad_select_gpio(RELAY_PIN_6);
    esp_rom_gpio_pad_select_gpio(RELAY_PIN_7);
    esp_rom_gpio_pad_select_gpio(RELAY_PIN_8);
    esp_rom_gpio_pad_select_gpio(LED_PIN);

    gpio_set_direction(RELAY_PIN_1, GPIO_MODE_INPUT_OUTPUT);
    gpio_set_direction(RELAY_PIN_2, GPIO_MODE_INPUT_OUTPUT);
    gpio_set_direction(RELAY_PIN_3, GPIO_MODE_INPUT_OUTPUT);
    gpio_set_direction(RELAY_PIN_4, GPIO_MODE_INPUT_OUTPUT);
    gpio_set_direction(RELAY_PIN_5, GPIO_MODE_INPUT_OUTPUT);
    gpio_set_direction(RELAY_PIN_6, GPIO_MODE_INPUT_OUTPUT);
    gpio_set_direction(RELAY_PIN_7, GPIO_MODE_INPUT_OUTPUT);
    gpio_set_direction(RELAY_PIN_8, GPIO_MODE_INPUT_OUTPUT);
    gpio_set_direction(LED_PIN, GPIO_MODE_INPUT_OUTPUT);
    
    gpio_off(RELAY_PIN_1);
    gpio_off(RELAY_PIN_2);
    gpio_off(RELAY_PIN_3);
    gpio_off(RELAY_PIN_4);
    gpio_off(RELAY_PIN_5);
    gpio_off(RELAY_PIN_6);
    gpio_off(RELAY_PIN_7);
    gpio_off(RELAY_PIN_8);
}

void toggle_GPIO(const gpio_num_t& gpio_pin) {
	bool state = gpio_get_level(gpio_pin);
    gpio_set_level(gpio_pin, state ? 0 : 1);
}

void gpio_off(const gpio_num_t& gpio_pin){
	    gpio_set_level(gpio_pin, 0);
}
void gpio_on(const gpio_num_t& gpio_pin){
	    gpio_set_level(gpio_pin, 1);
}

static void Task1(void *arg){
	
	while (true){
		vTaskDelay(pdMS_TO_TICKS(1000));
        ESP_LOGI(TAG, "tick %d\n ", gpio_get_level(LED_PIN));
        toggle_GPIO(LED_PIN);
	}
}



extern "C" void app_main()
{
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);


	vTaskDelay(pdMS_TO_TICKS(100));
	
	
	init_relays();    

    
    char ssid[32] = {0};
    char pass[64] = {0};
    nvs_handle_t nvs_handle;
    nvs_open("storage", NVS_READONLY, &nvs_handle);
    size_t ssid_len = sizeof(ssid);
    size_t pass_len = sizeof(pass);
    nvs_get_str(nvs_handle, "ssid", ssid, &ssid_len);
    nvs_get_str(nvs_handle, "password", pass, &pass_len);
    nvs_close(nvs_handle);

    bool wifi_credentials_exist = (ssid[0] != 0 && pass[0] != 0);

    if (wifi_credentials_exist) {
        wifi_init_sta(ssid, pass);
    } else {
        wifi_init_softap();
    }

    start_webserver(wifi_credentials_exist);

   	xTaskCreatePinnedToCore(Task1, "cmdProc", 4096, NULL, 1, NULL, tskNO_AFFINITY);
}
