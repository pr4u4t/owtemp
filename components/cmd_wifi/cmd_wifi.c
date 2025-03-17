/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* Console example — WiFi commands

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "cmd_wifi.h"

#define JOIN_TIMEOUT_MS (10000)

typedef struct wifi_state_t wifi_state_t;
 struct wifi_state_t{
    EventGroupHandle_t event_group;
    int CONNECTED_BIT;
    int join_timeout_ms;
};

static wifi_state_t wifi_state = {
    .event_group = NULL,
    .CONNECTED_BIT = BIT0,
    .join_timeout_ms = JOIN_TIMEOUT_MS
};

static inline char* wifi_get_auth_mode(int authmode) {
    switch (authmode) {
    case WIFI_AUTH_OPEN:
        return "WIFI_AUTH_OPEN";
    case WIFI_AUTH_OWE:
        return "WIFI_AUTH_OWE";
    case WIFI_AUTH_WEP:
        return "WIFI_AUTH_WEP";
    case WIFI_AUTH_WPA_PSK:
        return "WIFI_AUTH_WPA_PSK";
    case WIFI_AUTH_WPA2_PSK:
        return "WIFI_AUTH_WPA2_PSK";
    case WIFI_AUTH_WPA_WPA2_PSK:
        return "WIFI_AUTH_WPA_WPA2_PSK";
    case WIFI_AUTH_ENTERPRISE:
        return "WIFI_AUTH_ENTERPRISE";
    case WIFI_AUTH_WPA3_PSK:
        return "WIFI_AUTH_WPA3_PSK";
    case WIFI_AUTH_WPA2_WPA3_PSK:
        return "WIFI_AUTH_WPA2_WPA3_PSK";
    case WIFI_AUTH_WPA3_ENTERPRISE:
        return "WIFI_AUTH_WPA3_ENTERPRISE";
    case WIFI_AUTH_WPA2_WPA3_ENTERPRISE:
        return "WIFI_AUTH_WPA2_WPA3_ENTERPRISE";
    case WIFI_AUTH_WPA3_ENT_192:
        return "WIFI_AUTH_WPA3_ENT_192";
    default:
        return "WIFI_AUTH_UNKNOWN";
    }
}

static inline char* wifi_get_cipher(int pairwise_cipher){
    switch (pairwise_cipher) {
    case WIFI_CIPHER_TYPE_NONE:
        return "WIFI_CIPHER_TYPE_NONE";
    case WIFI_CIPHER_TYPE_WEP40:
        return "WIFI_CIPHER_TYPE_WEP40";
    case WIFI_CIPHER_TYPE_WEP104:
        return "WIFI_CIPHER_TYPE_WEP104";
    case WIFI_CIPHER_TYPE_TKIP:
        return "WIFI_CIPHER_TYPE_TKIP";
    case WIFI_CIPHER_TYPE_CCMP:
        return "WIFI_CIPHER_TYPE_CCMP";
    case WIFI_CIPHER_TYPE_TKIP_CCMP:
        return "WIFI_CIPHER_TYPE_TKIP_CCMP";
    case WIFI_CIPHER_TYPE_AES_CMAC128:
        return "WIFI_CIPHER_TYPE_AES_CMAC128";
    case WIFI_CIPHER_TYPE_SMS4:
        return "WIFI_CIPHER_TYPE_SMS4";
    case WIFI_CIPHER_TYPE_GCMP:
        return "WIFI_CIPHER_TYPE_GCMP";
    case WIFI_CIPHER_TYPE_GCMP256:
        return "WIFI_CIPHER_TYPE_GCMP256";
    default:
        return "WIFI_CIPHER_TYPE_UNKNOWN";
    }
}

static inline char* wifi_get_group_cipher(int group_cipher){
    switch (group_cipher) {
    case WIFI_CIPHER_TYPE_NONE:
        return "WIFI_CIPHER_TYPE_NONE";
    case WIFI_CIPHER_TYPE_WEP40:
        return "WIFI_CIPHER_TYPE_WEP40";
    case WIFI_CIPHER_TYPE_WEP104:
        return "WIFI_CIPHER_TYPE_WEP104";
    case WIFI_CIPHER_TYPE_TKIP:
        return "WIFI_CIPHER_TYPE_TKIP";
    case WIFI_CIPHER_TYPE_CCMP:
        return "WIFI_CIPHER_TYPE_CCMP";
    case WIFI_CIPHER_TYPE_TKIP_CCMP:
        return "WIFI_CIPHER_TYPE_TKIP_CCMP";
    case WIFI_CIPHER_TYPE_SMS4:
        return "WIFI_CIPHER_TYPE_SMS4";
    case WIFI_CIPHER_TYPE_GCMP:
        return "WIFI_CIPHER_TYPE_GCMP";
    case WIFI_CIPHER_TYPE_GCMP256:
        return "WIFI_CIPHER_TYPE_GCMP256";
    default:
        return "WIFI_CIPHER_TYPE_UNKNOWN";
    }
}

static inline void wifi_print_aps(void){
    wifi_ap_record_t *ap_list = NULL;
    uint16_t ap_num = 0;
    ESP_ERROR_CHECK( esp_wifi_scan_get_ap_num(&ap_num) );
    ap_list = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * ap_num);
    ESP_ERROR_CHECK( esp_wifi_scan_get_ap_records(&ap_num, ap_list) );
    for (int i = 0; i < ap_num; i++) {
        printf(
            "SSID: %s, RSSI: %d, channel: %d, authmod: %s, cipher: %s, cipher_group: %s\n", 
            (char *)ap_list[i].ssid, 
            ap_list[i].rssi, 
            ap_list[i].primary, 
            wifi_get_auth_mode(ap_list[i].authmode), 
            wifi_get_cipher(ap_list[i].pairwise_cipher), 
            wifi_get_group_cipher(ap_list[i].group_cipher)
        );
    }
    free(ap_list);
}

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data){
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(__func__, "Disconnected from AP");
        int bits = xEventGroupWaitBits(wifi_state.event_group, wifi_state.CONNECTED_BIT,
            pdFALSE, pdTRUE, 0);

        if((bits & wifi_state.CONNECTED_BIT) == 0){
            return;
        }

        esp_wifi_connect();
        xEventGroupClearBits(wifi_state.event_group, wifi_state.CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI(__func__, "Connected to AP");
        xEventGroupSetBits(wifi_state.event_group, wifi_state.CONNECTED_BIT);
    } else if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE){
        ESP_LOGI(__func__, "Scan done");
        wifi_print_aps();
    }
}

static void initialise_wifi(void){
    esp_log_level_set("wifi", ESP_LOG_WARN);
    static bool initialized = false;
    if (initialized) {
        return;
    }
    ESP_ERROR_CHECK(esp_netif_init());
    wifi_state.event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
    assert(ap_netif);
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &event_handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_SCAN_DONE, &event_handler, NULL) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_NULL) );
    ESP_ERROR_CHECK( esp_wifi_start() );
    initialized = true;
}

//------------------------------------------------------------------------------------------------------------

static bool wifi_join(const char *ssid, const char *pass, int timeout_ms){
    initialise_wifi();
    wifi_config_t wifi_config = { 0 };
    strlcpy((char *) wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    if (pass) {
        strlcpy((char *) wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));
    }

    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    esp_wifi_connect();

    int bits = xEventGroupWaitBits(wifi_state.event_group, wifi_state.CONNECTED_BIT,
                                   pdFALSE, pdTRUE, timeout_ms / portTICK_PERIOD_MS);
    return (bits & wifi_state.CONNECTED_BIT) != 0;
}

/** Arguments used by 'join' function */
typedef struct join_args_t join_args_t;
struct join_args_t {
    struct arg_int *timeout;
    struct arg_str *ssid;
    struct arg_str *password;
    struct arg_end *end;
};
static struct join_args_t join_args;

static int connect(int argc, char **argv){
    int nerrors = arg_parse(argc, argv, (void **) &join_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, join_args.end, argv[0]);
        return 1;
    }
    ESP_LOGI(__func__, "Connecting to '%s'",
             join_args.ssid->sval[0]);

    /* set default value*/
    if (join_args.timeout->count == 0) {
        join_args.timeout->ival[0] = JOIN_TIMEOUT_MS;
    }

    bool connected = wifi_join(join_args.ssid->sval[0],
                               join_args.password->sval[0],
                               join_args.timeout->ival[0]);
    if (!connected) {
        ESP_LOGW(__func__, "Connection timed out");
        return 1;
    }
    ESP_LOGI(__func__, "Connected");
    return 0;
}

static int disconnect(int argc, char **argv){
    xEventGroupClearBits(wifi_state.event_group, wifi_state.CONNECTED_BIT);
    ESP_ERROR_CHECK( esp_wifi_disconnect() );
    return 0;
}

static int ip_info(int argc, char **argv){
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &ip_info);
    printf("IP address: " IPSTR "\n", IP2STR(&ip_info.ip));
    printf("Netmask: " IPSTR "\n", IP2STR(&ip_info.netmask));
    printf("Gateway: " IPSTR "\n", IP2STR(&ip_info.gw));
    return 0;
}

static int wifi_scan(int argc, char **argv){
    wifi_scan_config_t scan_config = {
        .ssid = 0,
        .bssid = 0,
        .channel = 0,
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time.active.min = 120,
        .scan_time.active.max = 150,
    };
    initialise_wifi();
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_scan_start(&scan_config, false) );
    return 0;
}

static int wifi_stop_scan(int argc, char **argv){
    //initialise_wifi();
    ESP_ERROR_CHECK( esp_wifi_scan_stop() );
    return 0;
}

static int wifi_current_ap(int argc, char **argv){
    wifi_ap_record_t ap_info;
    esp_wifi_sta_get_ap_info(&ap_info);
    printf("SSID: %s, RSSI: %d, channel: %d, authmod: %s, cipher: %s, cipher_group: %s\n", 
        (char *)ap_info.ssid, 
        ap_info.rssi, 
        ap_info.primary, 
        wifi_get_auth_mode(ap_info.authmode), 
        wifi_get_cipher(ap_info.pairwise_cipher), 
        wifi_get_group_cipher(ap_info.group_cipher)
    );

    return 0;
}

static int wifi_init(int argc, char **argv){
    initialise_wifi();
    return 0;
}

static int wifi_deinit(int argc, char **argv){
    xEventGroupClearBits(wifi_state.event_group, wifi_state.CONNECTED_BIT);
    ESP_ERROR_CHECK( esp_wifi_disconnect() );
    ESP_ERROR_CHECK( esp_wifi_stop() );
    ESP_ERROR_CHECK( esp_wifi_deinit() );
    return 0;
}
//------------------------------------------------------------------------------------------------------------

static void register_init(void){
    const esp_console_cmd_t init_cmd = {
        .command = "wifi_init",
        .help = "Initialize WiFi",
        .hint = NULL,
        .func = &wifi_init,
        .argtable = NULL
    };

    ESP_ERROR_CHECK( esp_console_cmd_register(&init_cmd) );
}

static void register_deinit(void){
    const esp_console_cmd_t deinit_cmd = {
        .command = "wifi_deinit",
        .help = "Deinitialize WiFi",
        .hint = NULL,
        .func = &wifi_deinit,
        .argtable = NULL
    };

    ESP_ERROR_CHECK( esp_console_cmd_register(&deinit_cmd) );
}

static void register_connect(void){
    join_args.timeout = arg_int0(NULL, "timeout", "<t>", "Connection timeout, ms");
    join_args.ssid = arg_str1(NULL, NULL, "<ssid>", "SSID of AP");
    join_args.password = arg_str0(NULL, NULL, "<pass>", "PSK of AP");
    join_args.end = arg_end(2);

    const esp_console_cmd_t join_cmd = {
        .command = "wifi_connect",
        .help = "Join WiFi AP as a station",
        .hint = NULL,
        .func = &connect,
        .argtable = &join_args
    };

    ESP_ERROR_CHECK( esp_console_cmd_register(&join_cmd) );
}

static void register_disconnect(void){
    const esp_console_cmd_t disconnect_cmd = {
        .command = "wifi_disconnect",
        .help = "Disconnect from the AP",
        .hint = NULL,
        .func = &disconnect,
        .argtable = NULL
    };

    ESP_ERROR_CHECK( esp_console_cmd_register(&disconnect_cmd) );
}

static void register_ip_info(void){
    const esp_console_cmd_t ip_info_cmd = {
        .command = "wifi_ip_info",
        .help = "Get IP information",
        .hint = NULL,
        .func = &ip_info,
        .argtable = NULL
    };

    ESP_ERROR_CHECK( esp_console_cmd_register(&ip_info_cmd) );
}

static void register_wifi_scan_start(void){
    const esp_console_cmd_t wifi_scan_cmd = {
        .command = "wifi_scan_start",
        .help = "Start WiFi scan",
        .hint = NULL,
        .func = &wifi_scan,
        .argtable = NULL
    };

    ESP_ERROR_CHECK( esp_console_cmd_register(&wifi_scan_cmd) );
}

static void register_wifi_scan_stop(void){
    const esp_console_cmd_t wifi_stop_scan_cmd = {
        .command = "wifi_scan_stop",
        .help = "Stop WiFi scan",
        .hint = NULL,
        .func = &wifi_stop_scan,
        .argtable = NULL
    };

    ESP_ERROR_CHECK( esp_console_cmd_register(&wifi_stop_scan_cmd) );
}

static void register_wifi_current_ap(void){
    const esp_console_cmd_t wifi_current_ap_cmd = {
        .command = "wifi_current_ap",
        .help = "Get information of AP to which the device is associated with",
        .hint = NULL,
        .func = &wifi_current_ap,
        .argtable = NULL
    };

    ESP_ERROR_CHECK( esp_console_cmd_register(&wifi_current_ap_cmd) );
}

void register_wifi(void){
    register_connect();
    register_disconnect();
    register_ip_info();
    register_wifi_scan_start();
    register_wifi_scan_stop();
    register_wifi_current_ap();
    register_init();
    register_deinit();
}