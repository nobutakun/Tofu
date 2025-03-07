/**
 * @file hal.h
 * @brief Hardware Abstraction Layer interface
 */

#ifndef HAL_H
#define HAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <stdio.h>

// Network interface types
typedef int32_t network_handle_t;
typedef int32_t network_status_t;

#define NETWORK_HANDLE_INVALID (-1)
#define NETWORK_STATUS_OK 0
#define NETWORK_STATUS_ERROR (-1)
#define NETWORK_STATUS_TIMEOUT (-2)
#define NETWORK_STATUS_NOT_CONNECTED (-3)

// Network function prototypes
network_status_t hal_network_connect(const char *host, uint16_t port, 
                                   network_handle_t *handle);
network_status_t hal_network_disconnect(network_handle_t handle);
network_status_t hal_network_send(network_handle_t handle, 
                                const uint8_t *data, size_t length);
network_status_t hal_network_receive(network_handle_t handle,
                                   uint8_t *buffer, size_t length,
                                   size_t *received);
network_status_t hal_network_set_timeout(network_handle_t handle,
                                       uint32_t timeout_ms);

// System time functions
uint64_t hal_get_time_ms(void);
void hal_delay_ms(uint32_t ms);

// GPIO functions
void hal_gpio_init(void);
void hal_gpio_set(uint8_t pin, bool value);
bool hal_gpio_get(uint8_t pin);

// UART functions
void hal_uart_init(uint32_t baud_rate);
void hal_uart_send(const uint8_t *data, size_t length);
size_t hal_uart_receive(uint8_t *buffer, size_t length);

// I2C functions
bool hal_i2c_init(void);
bool hal_i2c_write(uint8_t addr, const uint8_t *data, size_t length);
bool hal_i2c_read(uint8_t addr, uint8_t *buffer, size_t length);

// SPI functions
void hal_spi_init(void);
void hal_spi_transfer(const uint8_t *tx_data, uint8_t *rx_data, size_t length);

// Flash memory functions
bool hal_flash_read(uint32_t addr, uint8_t *buffer, size_t length);
bool hal_flash_write(uint32_t addr, const uint8_t *data, size_t length);
bool hal_flash_erase(uint32_t addr, size_t length);

// File operations
int hal_file_open(const char *path, const char *mode, FILE **file);
int hal_file_close(FILE *file);
int hal_file_read(FILE *file, void *buffer, size_t size, size_t count, size_t *read);
int hal_file_write(FILE *file, const void *buffer, size_t size, size_t count, size_t *written);
int hal_file_copy(const char *src, const char *dest);
int hal_file_delete(const char *path);
bool hal_file_exists(const char *path);
int hal_file_rename(const char *old_path, const char *new_path);

// Directory management
int hal_dir_create(const char *path);
int hal_dir_delete(const char *path);
bool hal_dir_exists(const char *path);
int hal_list_dir(const char *path, char ***entries, size_t *count);
void hal_free_dir_list(char **entries, size_t count);

// Seek modes
#define HAL_SEEK_SET 0
#define HAL_SEEK_CUR 1
#define HAL_SEEK_END 2

// File seek operation
int hal_file_seek(FILE *file, long offset, int whence);

// File system error codes
#define HAL_FS_OK 0
#define HAL_FS_ERROR_CREATE -1
#define HAL_FS_ERROR_DELETE -2
#define HAL_FS_ERROR_ACCESS -3
#define HAL_FS_ERROR_EXISTS -4
#define HAL_FS_ERROR_NOTFOUND -5
#define HAL_FS_ERROR_READ -6
#define HAL_FS_ERROR_WRITE -7
#define HAL_FS_ERROR_INVALID -8

#endif // HAL_H
