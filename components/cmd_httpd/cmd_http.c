#include <stdio.h>
#include <string.h>
#include <math.h>
#include <esp_http_server.h>
#include <cJSON.h>
#include <esp_log.h>

#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#include "freertos/FreeRTOS.h"
#include "../cmd_ds18b20/cmd_ds18b20.h"

static httpd_handle_t server = NULL;

/* An HTTP_ANY handler */
static esp_err_t status_handler(httpd_req_t *req){
    /* Send response with body set as the
     * string passed in user context*/
    const char* resp_str = "{ \"status\": \"ok\" }";
    httpd_resp_set_hdr(req, "Content-Type", "application/json");
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t status = {
    .uri       = "/status",
    .method    = HTTP_ANY,
    .handler   = status_handler,
    .user_ctx  = NULL
};

static esp_err_t sensors_handler(httpd_req_t *req){
    /* Send response with body set as the
     * string passed in user context*/
    //const char* resp_str = "{ \"status\": \"ok\" }"; //
    ESP_LOGI(__func__, "Sensors handler");
    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    ds18b20_measurements_to_json(buffer, sizeof(buffer));
    ESP_LOGI(__func__, "Buffer: %s", buffer);
    httpd_resp_set_hdr(req, "Content-Type", "application/json");
    ESP_LOGI(__func__, "Sending response");
    httpd_resp_send(req, buffer, HTTPD_RESP_USE_STRLEN);
    
    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t sensors = {
    .uri       = "/sensors",
    .method    = HTTP_ANY,
    .handler   = sensors_handler,
    .user_ctx  = NULL
};

static httpd_handle_t start_webserver(void){
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    //config.max_uri_handlers = 8;
    
    // Start the httpd server
    ESP_LOGI(__func__, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(__func__, "Registering URI handlers");
        httpd_register_uri_handler(server, &status);
        httpd_register_uri_handler(server, &sensors);
        return server;
    }

    ESP_LOGI(__func__, "Error starting server!");
    return NULL;
}

static int httpd_server_start(int argc, char **argv){
    ESP_LOGI(__func__, "Starting HTTP server");
    if((server = start_webserver()) == NULL){
        ESP_LOGE(__func__, "Failed to start server");
        return 1;
    }

    return 0;
}

static int httpd_server_stop(int argc, char **argv){
    ESP_LOGI(__func__, "Stopping HTTP server");
    // Stop the httpd server
    if(httpd_stop(server) != ESP_OK){
        ESP_LOGE(__func__, "Failed to stop server");
        return 1;
    }

    return 0;
}

static void register_httpd_start(void){
    const esp_console_cmd_t httpd_start_cmd = {
        .command = "httpd_start",
        .help = "Start HTTP server",
        .hint = NULL,
        .func = &httpd_server_start,
        .argtable = NULL
    };

    ESP_ERROR_CHECK( esp_console_cmd_register(&httpd_start_cmd) );
}

static void register_httpd_stop(void){
    const esp_console_cmd_t httpd_stop_cmd = {
        .command = "httpd_stop",
        .help = "Stop HTTP server",
        .hint = NULL,
        .func = &httpd_server_stop,
        .argtable = NULL
    };

    ESP_ERROR_CHECK( esp_console_cmd_register(&httpd_stop_cmd) );
}

void httpd_register(void){
    register_httpd_start();
    register_httpd_stop();
}