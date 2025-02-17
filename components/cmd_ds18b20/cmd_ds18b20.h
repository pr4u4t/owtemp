#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Registers the DS18B20 temperature sensor command.
 *
 * This function initializes and registers the command for the DS18B20 temperature sensor,
 * allowing it to be used within the application.
 */
void ds18b20_register(void);

/**
 * @brief Initialize the DS18B20 temperature sensor.
 *
 * This function sets up the necessary configurations to communicate with the DS18B20 temperature sensor,
 * it also reads configuration from NVS to restore the previous configuration.
 *
 * @return
 *     - ESP_OK: Success
 *     - ESP_FAIL: Failed to initialize the sensor
 */
esp_err_t ds18b20_init(void);

/**
 * @brief Get the count of DS18B20 devices on the bus.
 *
 * This function counts the number of DS18B20 temperature sensors connected to the 1-Wire bus.
 *
 * @param[out] count Pointer to an integer where the count of devices will be stored.
 *
 * @return
 *     - ESP_OK: Successfully counted the devices on the bus.
 *     - ESP_ERR_INVALID_ARG: The count pointer is NULL.
 *     - ESP_FAIL: Failed to count the devices on the bus.
 */
esp_err_t ds18b20_bus_count(int *count);

/**
 * @brief Get the count of DS18B20 devices on the specified bus.
 *
 * This function counts the number of DS18B20 temperature sensors connected to the specified bus.
 *
 * @param[in] busid The ID of the bus to scan for devices.
 * @param[out] count Pointer to an integer where the device count will be stored.
 *
 * @return
 *     - ESP_OK: Successfully counted the devices.
 *     - ESP_ERR_INVALID_ARG: Invalid arguments.
 *     - ESP_FAIL: Failed to count the devices.
 */
esp_err_t ds18b20_bus_devices_count(int busid, int* count);

/**
 * @brief Reads the temperature from a DS18B20 sensor on the specified bus.
 *
 * @param busid The ID of the bus to which the DS18B20 sensor is connected.
 * @param devid The ID of the DS18B20 sensor on the bus.
 * @param temperature Pointer to a float where the read temperature will be stored.
 *
 * @return
 *     - ESP_OK: Success
 *     - ESP_ERR_INVALID_ARG: Invalid arguments
 *     - ESP_FAIL: Failed to read temperature
 */
esp_err_t ds18b20_bus_read(int busid, int devid, float* temperature);

#ifdef __cplusplus
}
#endif