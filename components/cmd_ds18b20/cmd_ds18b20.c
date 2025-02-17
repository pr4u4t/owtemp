#include <stdio.h>
#include <string.h>
#include <math.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "cmd_ds18b20.h"
#include "ds18b20.h"
#include "onewire_bus.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define ONEWIRE_MAX_DS18B20 2
#define ONEWIRE_MAX_BUS 2
#define MEASUREMENT_INTERVAL 1000
#define CONFIG_NAMESPACE "ds18b20:bus"
#define LW_EMA_ALPHA 0.1
#define MEASUREMENT_QUEUE_SIZE 10
#define MEASUREMENT_TASK_STACK_SIZE 4096
#define PROCESS_TASK_STACK_SIZE 2048

static const char* TAG = "cmd_ds18b20";

typedef struct bus_hwnd_t bus_hwnd_t;
struct bus_hwnd_t {
    onewire_bus_handle_t bus;
    onewire_bus_config_t bus_config;
    onewire_bus_rmt_config_t rmt_config;
    ds18b20_device_handle_t ds18b20s[ONEWIRE_MAX_DS18B20];
    uint active_devices;
    int ds18b20_device_num;
};

typedef struct buses_t buses_t;
struct buses_t {
    bus_hwnd_t bus[ONEWIRE_MAX_BUS];
    int buses_num;
    SemaphoreHandle_t mutex;
    QueueHandle_t measurement_queue;
    float measurements[ONEWIRE_MAX_BUS][ONEWIRE_MAX_DS18B20];
    portMUX_TYPE spinLock;
    TaskHandle_t measure_handle;
};

typedef struct ds18b20_measurement_t ds18b20_measurement_t;
struct ds18b20_measurement_t {
    float temperature;
    int bus_id;
    int device_id;
};

typedef struct bus_config_t bus_config_t;
struct bus_config_t{
    int gpio_num;
    uint active_devices;
};

static buses_t buses = {
    .bus = {{0}},
    .buses_num = 0,
    .spinLock = portMUX_INITIALIZER_UNLOCKED
};

static inline void set_nth_bit(uint *flag, int n) {
    *flag |= (1 << n);
}

static inline void clear_nth_bit(uint *flag, int n) {
    *flag &= ~(1 << n);
}

static inline int is_nth_bit_set(uint flag, int n) {
    return (flag & (1 << n)) ? 1 : 0;
}

static int _bus_find(int gpio_num, int* busid) {
    for (size_t i = 0; i < buses.buses_num; i++) {
        if(buses.bus[i].bus_config.bus_gpio_num == gpio_num){
            *busid = i;
        }
    }

    return ESP_OK;
}

/*----------------------[ bus_scan ]----------------------*/

/** Arguments used by 'bus_scan' function */
static struct {
    struct arg_int *bus_id;
    struct arg_end *end;
} bus_scan_args;

static esp_err_t _bus_scan(int busid) {
    onewire_device_iter_handle_t iter = NULL;
    onewire_device_t next_onewire_device;
    esp_err_t err = ESP_OK;

    if(buses.buses_num == 0){
        ESP_LOGE(TAG, "No buses available");
        return ESP_FAIL;
    }

    if(buses.buses_num <= busid){
        ESP_LOGE(TAG, "Bus identified by id: %d not found", busid);
        return ESP_FAIL;
    }

    if(buses.bus[busid].ds18b20_device_num != 0){
        ESP_LOGE(TAG, "Bus identified by id: %d has associated devices", busid);
        return ESP_FAIL;
    }

    bus_hwnd_t* bus_hwnd = &buses.bus[busid];

    // create 1-wire device iterator, which is used for device search
    if((err = onewire_new_device_iter(bus_hwnd->bus, &iter)) != ESP_OK){
        ESP_LOGE(TAG, "Failed to create device iterator, error: %s", esp_err_to_name(err));
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Device iterator created, start searching...");
    int devices = 0;

    do {
        if ((err = onewire_device_iter_get_next(iter, &next_onewire_device)) == ESP_OK) {
            ds18b20_config_t ds_cfg = {};
            if (ds18b20_new_device(&next_onewire_device, &ds_cfg, &bus_hwnd->ds18b20s[devices]) == ESP_OK) {
                ESP_LOGI(TAG, "Found a DS18B20[%d], address: %016llX", devices, next_onewire_device.address);
                devices++;
            } else {
                ESP_LOGI(TAG, "Found an unknown device, address: %016llX, error: %s", next_onewire_device.address, esp_err_to_name(err));
            }
        }
    } while (err != ESP_ERR_NOT_FOUND && devices < ONEWIRE_MAX_DS18B20);

    if((err = onewire_del_device_iter(iter)) != ESP_OK){
        ESP_LOGE(TAG, "Failed to delete device iterator, error: %s", esp_err_to_name(err));
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Searching done, %d DS18B20 device(s) found", devices);

    //__atomic_store_n(&bus_hwnd->ds18b20_device_num, devices, __ATOMIC_SEQ_CST);
    bus_hwnd->ds18b20_device_num = devices;

    return ESP_OK;
}

static int bus_scan(int argc, char **argv) {
    int nerrors = arg_parse(argc, argv, (void **) &bus_scan_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, bus_scan_args.end, argv[0]);
        return 1;
    }

    const int busid = bus_scan_args.bus_id->ival[0];

    if(_bus_scan(busid) != ESP_OK){
        return ESP_FAIL;
    }

    for(size_t i = 0; i < buses.bus[busid].ds18b20_device_num; i++){
        ds18b20_device_handle_t hwnd = buses.bus[busid].ds18b20s[i];
        onewire_device_address_t addr;
        ds18b20_address(hwnd, &addr);
        printf("Bus[%d].DS18B20[%d] address: %016llX\r\n", busid, i, addr);
    }

    return ESP_OK;
}

static void bus_scan_register(void) {
    bus_scan_args.bus_id = arg_int1(NULL, "bus_id", "<int>", "Bus gpio number");
    bus_scan_args.end = arg_end(0);

    const esp_console_cmd_t bus_scan_cmd = {
        .command = "ds18b20_bus_scan",
        .help = "Scan bus identified byy id for DS18B20 devices, bus must be prior opened",
        .hint = NULL,
        .func = &bus_scan,
        .argtable = &bus_scan_args
    };

    ESP_ERROR_CHECK( esp_console_cmd_register(&bus_scan_cmd) );
}

/*----------------------[ bus_list ]----------------------*/

static int buses_list(int argc, char **argv) {
    if(buses.buses_num == 0){
        ESP_LOGI(TAG, "No buses available");
        printf("No buses available\r\n");
        return ESP_OK;
    }

    for (size_t i = 0; i < buses.buses_num; i++) {
        ESP_LOGI(TAG, "Bus[%d]: GPIO %d", i, buses.bus[i].bus_config.bus_gpio_num);
        printf("Bus[%d]: GPIO %d\r\n", i, buses.bus[i].bus_config.bus_gpio_num);
    }

    return ESP_OK;
}

static void buses_list_register(void) {
    const esp_console_cmd_t buses_list_cmd = {
        .command = "ds18b20_bus_list",
        .help = "List all 1-Wire bus",
        .hint = NULL,
        .func = &buses_list,
        .argtable = NULL
    };

    ESP_ERROR_CHECK( esp_console_cmd_register(&buses_list_cmd) );
}

/*----------------------[ bus_read ]----------------------*/
/** Arguments used by 'bus_read' function */
static struct {
    struct arg_int *gpio_num;
    struct arg_int *device_num;
    struct arg_end *end;
} bus_read_args;

static esp_err_t _bus_read(int busid, int devid, float* temperature, int* time) {
    TickType_t start = xTaskGetTickCount();
    if(buses.buses_num == 0){
        ESP_LOGE(TAG, "No buses available");
        return ESP_FAIL;
    }

    if(buses.buses_num <= busid){
        ESP_LOGE(TAG, "Bus identified by id: %d not found", busid);
        return ESP_FAIL;
    }

    if(devid >= buses.bus[busid].ds18b20_device_num){
        ESP_LOGE(TAG, "Device with number %d not found", devid);
        return ESP_ERR_NOT_FOUND;
    }

    ds18b20_device_handle_t hwnd = buses.bus[busid].ds18b20s[devid];

    xSemaphoreTake(buses.mutex, portMAX_DELAY);
    if(ds18b20_trigger_temperature_conversion(hwnd) != ESP_OK){
        ESP_LOGE(TAG, "Failed to trigger temperature conversion");
        goto FAIL;
    }

    if(ds18b20_get_temperature(hwnd, temperature) != ESP_OK){
        ESP_LOGE(TAG, "Failed to read temperature");
        goto FAIL;
    }
    xSemaphoreGive(buses.mutex);
    TickType_t end = xTaskGetTickCount();
    if(time != NULL){
        *time = (end - start) * portTICK_PERIOD_MS;
    }
    ESP_LOGD(TAG, "temperature read from Bus[%d].DS18B20[%d]: %.2fC, time: %ldms", busid, devid, *temperature, (end - start) * portTICK_PERIOD_MS);

    return ESP_OK;

FAIL:
    xSemaphoreGive(buses.mutex);
    return ESP_FAIL;
}

static int bus_read(int argc, char **argv) {
    int nerrors = arg_parse(argc, argv, (void **) &bus_read_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, bus_read_args.end, argv[0]);
        return 1;
    }

    const int busid = bus_read_args.gpio_num->ival[0];
    const int devid = bus_read_args.device_num->ival[0];
    float temperature = 0;
    int time = 0;

    taskENTER_CRITICAL(&buses.spinLock);
    temperature = buses.measurements[busid][devid];
    taskEXIT_CRITICAL(&buses.spinLock);

    printf("temperature read from Bus[%d].DS18B20[%d]: %.2fC, time: %dms\r\n", busid, devid, temperature, time);

    return ESP_OK;
}

static void bus_read_register(void) {
    bus_read_args.gpio_num = arg_int1(NULL, "bus_id", "<int>", "Bus gpio number");
    bus_read_args.device_num = arg_int1(NULL, "device_id", "<int>", "Device number");
    bus_read_args.end = arg_end(2);

    const esp_console_cmd_t bus_read_cmd = {
        .command = "ds18b20_read",
        .help = "Read temperature from DS18B20 device",
        .hint = NULL,
        .func = &bus_read,
        .argtable = &bus_read_args
    };

    ESP_ERROR_CHECK( esp_console_cmd_register(&bus_read_cmd) );
}

/*----------------------[ bus_read_direct ]----------------------*/
/** Arguments used by 'bus_read_direct' function */
static struct {
    struct arg_int *gpio_num;
    struct arg_int *device_num;
    struct arg_end *end;
} bus_read_direct_args;

static int bus_read_direct(int argc, char **argv) {
    int nerrors = arg_parse(argc, argv, (void **) &bus_read_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, bus_read_args.end, argv[0]);
        return 1;
    }

    const int busid = bus_read_args.gpio_num->ival[0];
    const int devid = bus_read_args.device_num->ival[0];
    float temperature = 0;
    int time = 0;

    if(_bus_read(busid, devid, &temperature, &time) != ESP_OK){
        ESP_LOGE(TAG, "Failed to read temperature");
        return ESP_FAIL;
    }

    printf("temperature read from Bus[%d].DS18B20[%d]: %.2fC, time: %dms\r\n", busid, devid, temperature, time);

    return ESP_OK;
}

static void bus_read_direct_register(void) {
    bus_read_direct_args.gpio_num = arg_int1(NULL, "bus_id", "<int>", "Bus gpio number");
    bus_read_direct_args.device_num = arg_int1(NULL, "device_id", "<int>", "Device number");
    bus_read_direct_args.end = arg_end(2);

    const esp_console_cmd_t bus_read_direct_cmd = {
        .command = "ds18b20_read_direct",
        .help = "Read direct temperature from DS18B20 device",
        .hint = NULL,
        .func = &bus_read_direct,
        .argtable = &bus_read_direct_args
    };

    ESP_ERROR_CHECK( esp_console_cmd_register(&bus_read_direct_cmd) );
}

/*----------------------[ bus_open ]----------------------*/

/** Arguments used by 'bus_open' function */
static struct {
    struct arg_int *gpio_num;
    struct arg_end *end;
} bus_open_args;

static int _bus_open(int gpio_num, int* ret_busid) {
    int busid = -1;
    esp_err_t err;

    if(buses.buses_num >= ONEWIRE_MAX_BUS){
        ESP_LOGE(TAG, "Exceed maximum number of buses");
        return ESP_FAIL;
    }    

    if(_bus_find(gpio_num, &busid) != ESP_OK){
        ESP_LOGE(TAG, "Failed to check if bus identified by GPIO %d already exists", gpio_num);
        return ESP_FAIL;
    }

    if(busid != -1){
        ESP_LOGE(TAG, "Bus identified by GPIO %d already exists", gpio_num);
        return ESP_FAIL;
    }

    bus_hwnd_t* bus_hwnd = &buses.bus[buses.buses_num];

    // install 1-wire bus
    bus_hwnd->bus = NULL;
    bus_hwnd->bus_config.bus_gpio_num = gpio_num;
    bus_hwnd->rmt_config.max_rx_bytes = 10; // 1byte ROM command + 8byte ROM number + 1byte device command
    bus_hwnd->ds18b20_device_num = 0;

    if((err = onewire_new_bus_rmt(&bus_hwnd->bus_config, &bus_hwnd->rmt_config, &bus_hwnd->bus)) != ESP_OK){
        ESP_LOGE(TAG, "Failed to install 1-Wire bus, error: %s", esp_err_to_name(err));
        return ESP_FAIL;
    }
 
    *ret_busid = buses.buses_num;
    buses.buses_num++;

    return ESP_OK;
}

static int bus_open(int argc, char **argv) {
    int nerrors = arg_parse(argc, argv, (void **) &bus_open_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, bus_open_args.end, argv[0]);
        return 1;
    }

    const int gpio_num = bus_open_args.gpio_num->ival[0];
    ESP_LOGD(TAG, "Trying to open bus with GPIO %d", gpio_num);
    int busid = -1;

    if(_bus_open(gpio_num, &busid) != ESP_OK){
        ESP_LOGE(TAG, "1-Wire bus initialization failed");
        return ESP_FAIL;
    }

    printf("Opened bus with GPIO %d as %d\r\n", gpio_num, busid);

    return ESP_OK;
}

static void bus_open_register(void) {
    bus_open_args.gpio_num = arg_int1(NULL, "bus_gpio", "<int>", "Bus gpio number");
    bus_open_args.end = arg_end(1);

    const esp_console_cmd_t bus_open_cmd = {
        .command = "ds18b20_bus_open",
        .help = "Open bus identified by GPIO number for DS18B20 devices",
        .hint = NULL,
        .func = &bus_open,
        .argtable = &bus_open_args
    };

    ESP_ERROR_CHECK( esp_console_cmd_register(&bus_open_cmd) );
}

/*------------------[ bus_list_devices ]------------------*/

/** Arguments used by 'bus_open' function */
static struct {
    struct arg_int *bus_id;
    struct arg_end *end;
} bus_list_devices_args;

static esp_err_t _bus_list_devices(int busid) {
    esp_err_t err;
    if(buses.buses_num == 0){
        ESP_LOGE(TAG, "No buses available");
        printf("No buses available\r\n");
        return ESP_FAIL;
    }

    if(buses.buses_num <= busid){
        ESP_LOGE(TAG, "Bus identified by id: %d not found", busid);
        printf("Bus identified by id: %d not found\r\n", busid);
        return ESP_FAIL;
    }

    for (int i = 0; i < buses.bus[busid].ds18b20_device_num; i++) {
        onewire_device_address_t addr;
        if((err = ds18b20_address(buses.bus[busid].ds18b20s[i], &addr)) != ESP_OK){
            ESP_LOGE(TAG, "Failed to get address of DS18B20[%d], error: %s", i, esp_err_to_name(err));
            continue;
        }
        ESP_LOGI(TAG, "DS18B20[%d]: address: %016llX, enabled: %d", i, addr, is_nth_bit_set(buses.bus[busid].active_devices, i));
        printf("DS18B20[%d]: address: %016llX, enabled: %d\r\n", i, addr, is_nth_bit_set(buses.bus[busid].active_devices, i));
    }

    return ESP_OK;
}

static int bus_list_devices(int argc, char **argv) {
    int nerrors = arg_parse(argc, argv, (void **) &bus_list_devices_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, bus_list_devices_args.end, argv[0]);
        return 1;
    }

    const int busid = bus_list_devices_args.bus_id->ival[0];

    return _bus_list_devices(busid);
}

static void bus_list_devices_register(void) {
    bus_list_devices_args.bus_id = arg_int1(NULL, "bus_id", "<int>", "Bus id");
    bus_list_devices_args.end = arg_end(1);

    const esp_console_cmd_t bus_open_cmd = {
        .command = "ds18b20_bus_list_devices",
        .help = "List devices on bus identified by id",
        .hint = NULL,
        .func = &bus_list_devices,
        .argtable = &bus_list_devices_args
    };

    ESP_ERROR_CHECK( esp_console_cmd_register(&bus_open_cmd) );
}

/*------------------[ buses_store_configuration ]------------------*/

static int buses_store_configuration(int argc, char **argv) {
    if(buses.buses_num == 0){
        ESP_LOGI(TAG, "No buses available");
        printf("No buses available\r\n");
        return ESP_OK;
    }
    nvs_handle_t config_handle;
    
    esp_err_t err = nvs_open(CONFIG_NAMESPACE, NVS_READWRITE, &config_handle);
    if (err != ESP_OK) {
        printf("Error opening NVS: %s\r\n", esp_err_to_name(err));
        return ESP_FAIL;
    }

    err = nvs_erase_all(config_handle);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Namespace '%s' erased successfully.", CONFIG_NAMESPACE);
    } else {
        ESP_LOGE(TAG, "Failed to erase namespace '%s'. Error: %s", CONFIG_NAMESPACE, esp_err_to_name(err));
    }

    // Commit changes
    err = nvs_commit(config_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit erase operation.");
    }

    for (size_t i = 0; i < buses.buses_num; i++) {
        ESP_LOGI(TAG, "Bus[%d]: GPIO %d", i, buses.bus[i].bus_config.bus_gpio_num);
        char key[16];
        snprintf(key, sizeof(key), "bus%d", i);
        bus_config_t config = {
            .gpio_num = buses.bus[i].bus_config.bus_gpio_num,
            .active_devices = buses.bus[i].active_devices
        };
        if(nvs_set_blob(config_handle, key, &config, sizeof(bus_config_t)) != ESP_OK){
            ESP_LOGE(TAG, "Failed to save %s", key); 
        }
    }

    err = nvs_commit(config_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS changes!");
    }

    nvs_close(config_handle);

    return ESP_OK;
}

static void buses_store_configuration_register(void) {
    const esp_console_cmd_t buses_store_configuration_cmd = {
        .command = "ds18b20_buses_save",
        .help = "Save buses configuration",
        .hint = NULL,
        .func = &buses_store_configuration,
        .argtable = NULL
    };

    ESP_ERROR_CHECK( esp_console_cmd_register(&buses_store_configuration_cmd) );
}

/*------------------[ bus_set_device ]------------------*/

/** Arguments used by 'bus_set_device' function */
static struct {
    struct arg_int *bus_id;
    struct arg_int *dev_id;
    struct arg_str *enable;
    struct arg_end *end;
} bus_set_args;

static esp_err_t _bus_set_device(int busid, int devid, bool enable) {
    if(buses.buses_num == 0){
        ESP_LOGE(TAG, "No buses available");
        return ESP_FAIL;
    }

    if(buses.buses_num <= busid){
        ESP_LOGE(TAG, "Bus identified by id: %d not found", busid);
        return ESP_FAIL;
    }

    if(devid >= buses.bus[busid].ds18b20_device_num){
        ESP_LOGE(TAG, "Device with number %d not found", devid);
        return ESP_ERR_NOT_FOUND;
    }

    if(enable){
        set_nth_bit(&buses.bus[busid].active_devices, devid);
    } else {
        clear_nth_bit(&buses.bus[busid].active_devices, devid);
    }

    return ESP_OK;
}

static int bus_set_device(int argc, char **argv) {
    int nerrors = arg_parse(argc, argv, (void **) &bus_set_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, bus_set_args.end, argv[0]);
        return 1;
    }

    const int busid = bus_set_args.bus_id->ival[0];
    const int devid = bus_set_args.dev_id->ival[0];
    const bool is_enabled = (strcmp(bus_set_args.enable->sval[0], "true") == 0);

    if(_bus_set_device(busid, devid, is_enabled) != ESP_OK){
        return ESP_FAIL;
    }

    printf("Bus[%d].DS18B20[%d] %s\r\n", busid, devid, is_enabled ? "enabled" : "disabled");

    return ESP_OK;
}

static void bus_set_register(void) {
    bus_set_args.bus_id = arg_int1(NULL, "bus_id", "<int>", "Bus id");
    bus_set_args.dev_id = arg_int1(NULL, "device_id", "<int>", "Device id");
    bus_set_args.enable = arg_str1(NULL, "enable", "<bool>", "Enable or disable device");
    bus_set_args.end = arg_end(1);

    const esp_console_cmd_t bus_set_cmd = {
        .command = "ds18b20_bus_device_set",
        .help = "Enable or disable particular DS18B20 device on bus",
        .hint = NULL,
        .func = &bus_set_device,
        .argtable = &bus_set_args
    };

    ESP_ERROR_CHECK( esp_console_cmd_register(&bus_set_cmd) );
}

void ds18b20_register(void) {
    bus_open_register();
    bus_scan_register();
    buses_list_register();
    bus_read_register();
    bus_list_devices_register();
    buses_store_configuration_register();
    bus_set_register();
    bus_read_direct_register();
}

static void ds18b20_measure_task(void *pvParameters) {
    for(;;) {
        const int buses_num = buses.buses_num;
        int interval = MEASUREMENT_INTERVAL;
        for (size_t i = 0; i < buses_num; i++) {
            const int devices = buses.bus[i].ds18b20_device_num;
            for (size_t j = 0; j < devices; j++) {
                int time = 0;
                if(is_nth_bit_set(buses.bus[i].active_devices, j) == 0){
                    continue;
                }

                ds18b20_measurement_t measurement = {
                    .temperature = 0,
                    .bus_id = i,
                    .device_id = j
                };

                if(_bus_read(i, j, &measurement.temperature, &time) != ESP_OK){
                    ESP_LOGE(TAG, "Failed to read temperature from Bus[%d].DS18B20[%d]", i, j);
                }

                interval -= time;
                TickType_t start = xTaskGetTickCount();
                if (xQueueSend(buses.measurement_queue, &measurement, pdMS_TO_TICKS(1000)) != pdPASS) {
                    ESP_LOGE(TAG, "Queue full! Failed to send measurment.");
                }

                //ulTaskNotifyTakeIndexed( 0, pdTRUE, pdMS_TO_TICKS(50) ); 
                TickType_t end = xTaskGetTickCount();
                interval -= (end - start) * portTICK_PERIOD_MS;
            }
        }

        if(interval > 0){
            vTaskDelay(pdMS_TO_TICKS(interval));
        }
    }
}

static void ds18b20_process_task(void *pvParameters){
    const float ALPHA = LW_EMA_ALPHA;
    
    for(;;){
        ds18b20_measurement_t received_measurement;
        if (xQueueReceive(buses.measurement_queue, &received_measurement, portMAX_DELAY)) {

            const float last = buses.measurements[received_measurement.bus_id][received_measurement.device_id];
            taskENTER_CRITICAL(&buses.spinLock);
            if(isnanf(last) == 0){
                buses.measurements[received_measurement.bus_id][received_measurement.device_id] = (ALPHA * received_measurement.temperature) + ((1 - ALPHA) * last);
            } else {
                buses.measurements[received_measurement.bus_id][received_measurement.device_id] = received_measurement.temperature;
            }
            taskEXIT_CRITICAL(&buses.spinLock);
        }
    }
}

esp_err_t ds18b20_init(void) {
    buses.mutex = xSemaphoreCreateRecursiveMutex();
    if(buses.mutex == NULL){
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_FAIL;
    }

    nvs_handle_t config_handle;
    esp_err_t err = nvs_open(CONFIG_NAMESPACE, NVS_READWRITE, &config_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS: %s", esp_err_to_name(err));
        return ESP_FAIL;
    }

    for (size_t i = 0; i < ONEWIRE_MAX_BUS; i++) {
        char key[16];
        snprintf(key, sizeof(key), "bus%d", i);
        bus_config_t config;
        int busid = -1;
        size_t len = sizeof(bus_config_t); 
        esp_err_t err = nvs_get_blob(config_handle, key, &config, &len);
        if( err == ESP_OK){
            ESP_LOGI(TAG, "Restoring bus identified by gpio: %d", config.gpio_num);
            _bus_open(config.gpio_num, &busid); //check err
            _bus_scan(busid); //check err
            buses.bus[busid].active_devices = config.active_devices;
            ESP_LOGI(TAG, "Restored bus identified by gpio: %d as %d", config.gpio_num, busid);
        } else {
            ESP_LOGD(TAG, "Bus: %s not found, error: %s", key, esp_err_to_name(err));
        }
    }

    buses.measurement_queue = xQueueCreate(MEASUREMENT_QUEUE_SIZE, sizeof(ds18b20_measurement_t));
    if (buses.measurement_queue == NULL) {
        ESP_LOGE(TAG, "Queue creation failed");
        return ESP_FAIL;
    }

    for(size_t i = 0; i < ONEWIRE_MAX_BUS; i++){
        for(size_t j = 0; j < ONEWIRE_MAX_DS18B20; j++){
            buses.measurements[i][j] = NAN;
        }
    }

    xTaskCreate(ds18b20_measure_task, "MeasureTask", MEASUREMENT_TASK_STACK_SIZE, NULL, 1, &buses.measure_handle);
    xTaskCreate(ds18b20_process_task, "ProcessTask", PROCESS_TASK_STACK_SIZE, NULL, 1, NULL);

    return ESP_OK;
}

esp_err_t ds18b20_bus_count(int *count){
    *count = buses.buses_num;
    return ESP_OK;
}

esp_err_t ds18b20_bus_devices_count(int busid, int* count){
    if(buses.buses_num <= busid){
        return ESP_ERR_NOT_FOUND;
    }

    *count = buses.bus[busid].ds18b20_device_num;
    return ESP_OK;
}

esp_err_t ds18b20_bus_read(int busid, int devid, float* temperature){
    return _bus_read(busid, devid, temperature, NULL);
}