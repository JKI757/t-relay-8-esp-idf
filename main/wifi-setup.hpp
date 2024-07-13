/*
 * wifi-ap.hpp
 *
 *  Created on: Jul 7, 2024
 *      Author: josh
 */

#ifndef MAIN_WIFI_SETUP_HPP_
#define MAIN_WIFI_SETUP_HPP_


#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "json_generator.h"

#include "esp_netif.h"

#include <ctype.h>

#include "defs.hpp"


#define EXAMPLE_ESP_WIFI_SSID      "myssid"
#define EXAMPLE_ESP_WIFI_PASS      "mypassword"
#define EXAMPLE_MAX_STA_CONN       4

static const char *LocalTAG = "wifi softAP";

extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t relays_html_start[] asm("_binary_relays_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");
extern const uint8_t relays_html_end[] asm("_binary_relays_html_end");


static char hex2char(char hex) {
    if (hex >= '0' && hex <= '9') {
        return hex - '0';
    } else if (hex >= 'A' && hex <= 'F') {
        return hex - 'A' + 10;
    } else if (hex >= 'a' && hex <= 'f') {
        return hex - 'a' + 10;
    }
    return 0;
}

static void urldecode(char *dst, const char *src) {
    while (*src) {
        if ((*src == '%') && (src[1]) && (src[2])) {
            *dst++ = hex2char(src[1]) * 16 + hex2char(src[2]);
            src += 3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

static esp_err_t index_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)index_html_start, index_html_end - index_html_start);
    return ESP_OK;
}

static esp_err_t relays_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)relays_html_start, relays_html_end - relays_html_start);
    return ESP_OK;
}

static esp_err_t toggle_handler(httpd_req_t *req) {
    char buf[10];
    memset(buf, 0x00, sizeof(buf));
    size_t len = httpd_req_get_url_query_len(req) + 1;
    int relay_num = -1;

    if (len > 1) {
        char *query_buf = new char[len];
        memset(query_buf, 0x00, len);
        if (httpd_req_get_url_query_str(req, query_buf, len) == ESP_OK) {
            if (httpd_query_key_value(query_buf, "relay", buf, sizeof(buf)) == ESP_OK) {
                relay_num = atoi(buf);
            }
        }
        delete[](query_buf);
    }

    if (relay_num >= 1 && relay_num <= 8) {
        gpio_num_t relay_pin = relay_pins[relay_num - 1];
        int level = gpio_get_level(relay_pin);
        gpio_set_level(relay_pin, !level);

        json_gen_str_t gen;
        char json_response[64];
        memset(json_response, 0x00, sizeof(json_response));
        json_gen_str_start(&gen, json_response, sizeof(json_response), NULL, NULL);
        json_gen_start_object(&gen);
        json_gen_obj_set_bool(&gen, "state", !level);
        json_gen_end_object(&gen);

        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, json_response, strlen(json_response));

        // ESP_LOGI(LocalTAG, "Relay %d toggled to %d", relay_num, !level);
        // ESP_LOGI(LocalTAG, "json_response in Toggle Handler: %s", json_response);
        return ESP_OK;
    }

    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid Relay Number");
    return ESP_FAIL;
}

static esp_err_t status_handler(httpd_req_t *req) {
    json_gen_str_t gen;
    char json_response[256];
    memset(json_response, 0x00, sizeof(json_response));
    json_gen_str_start(&gen, json_response, sizeof(json_response), NULL,NULL);
    json_gen_start_object(&gen);
    for (int i = 1; i <= 8; i++) {
        gpio_num_t relay_pin = relay_pins[i - 1];
        char relay_key[10];
        snprintf(relay_key, sizeof(relay_key), "relay%d", i);
        json_gen_obj_set_bool(&gen, relay_key, gpio_get_level(relay_pin));
    }
    json_gen_end_object(&gen);
    // ESP_LOGI(LocalTAG, "json_response in Status Handler: %s", json_response);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_response, strlen(json_response));

    return ESP_OK;
}

static void wifi_init_softap(void) {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .ssid_len = static_cast<uint8_t>(strlen(EXAMPLE_ESP_WIFI_SSID)),
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .max_connection = EXAMPLE_MAX_STA_CONN
        },
    };

    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(LocalTAG, "wifi_init_softap finished. SSID:%s password:%s",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
}

static esp_err_t connect_handler(httpd_req_t *req) {
    char content[128];
    size_t len = httpd_req_get_url_query_len(req) + 1;
    if (len > 1) {
        char *buf = new char[len];
        if (httpd_req_get_url_query_str(req, buf, len) == ESP_OK) {
            char ssid[32];
            char password[64];
            char decoded_ssid[32];
            char decoded_password[64];
            if (httpd_query_key_value(buf, "ssid", ssid, sizeof(ssid)) == ESP_OK &&
                httpd_query_key_value(buf, "password", password, sizeof(password)) == ESP_OK) {
                
                urldecode(decoded_ssid, ssid);
                urldecode(decoded_password, password);

                nvs_handle_t nvs_handle;
                nvs_open("storage", NVS_READWRITE, &nvs_handle);
                nvs_set_str(nvs_handle, "ssid", decoded_ssid);
                nvs_set_str(nvs_handle, "password", decoded_password);
                nvs_commit(nvs_handle);
                nvs_close(nvs_handle);
                sprintf(content, "SSID: %s, Password: %s", decoded_ssid, decoded_password);
                httpd_resp_send(req, content, HTTPD_RESP_USE_STRLEN);
                delete[](buf);
                return ESP_OK;
            }
        }
        delete[](buf);
    }
    httpd_resp_send(req, "Invalid Parameters", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

httpd_uri_t connect_uri = {
    .uri      = "/connect",
    .method   = HTTP_GET,
    .handler  = connect_handler,
    .user_ctx = NULL
};

static void start_webserver(bool serve_relays_page) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.task_priority = tskIDLE_PRIORITY + 5; // Increase task priority
    config.stack_size = 8192; // Increase stack size

    httpd_uri_t index_uri = {
        .uri      = "/",
        .method   = HTTP_GET,
        .handler  = serve_relays_page ? relays_handler : index_handler,
        .user_ctx = NULL
    };

    httpd_uri_t relays_uri = {
        .uri      = "/relays",
        .method   = HTTP_GET,
        .handler  = relays_handler,
        .user_ctx = NULL
    };

    httpd_uri_t connect_uri = {
        .uri      = "/connect",
        .method   = HTTP_GET,
        .handler  = connect_handler,
        .user_ctx = NULL
    };

    httpd_uri_t toggle_uri = {
        .uri      = "/toggle",
        .method   = HTTP_GET,
        .handler  = toggle_handler,
        .user_ctx = NULL
    };

    httpd_uri_t status_uri = {
        .uri      = "/status",
        .method   = HTTP_GET,
        .handler  = status_handler,
        .user_ctx = NULL
    };

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &index_uri);
        httpd_register_uri_handler(server, &relays_uri);
        httpd_register_uri_handler(server, &connect_uri);
        httpd_register_uri_handler(server, &toggle_uri);
        httpd_register_uri_handler(server, &status_uri);
    }
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(LocalTAG, "Disconnected from WiFi, attempting to reconnect...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        // ESP_LOGI(LocalTAG, "Got IP: %s", esp_ip4addr_ntoa(&event->ip_info.ip));
    }
}

static void wifi_init_sta(const char *ssid, const char *pass) {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(LocalTAG, "Failed to initialize WiFi: %s", esp_err_to_name(ret));
        return;
    }

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {};
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password) - 1);

    ESP_LOGI(LocalTAG, "Connecting to SSID: %s", ssid);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(LocalTAG, "WiFi initialization complete.");
}

#endif /* MAIN_WIFI_SETUP_HPP_ */
