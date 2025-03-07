/**
 * @file hal.c
 * @brief Hardware Abstraction Layer implementation
 */

#include "hal.h"
#include "firmware_config.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <windows.h>
#define mkdir(path, mode) _mkdir(path)
#define rmdir _rmdir
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

// Network operations implementation
network_status_t hal_network_connect(const char *host, uint16_t port,
                                   network_handle_t *handle) {
    // Platform-specific network connection code here
    return NETWORK_STATUS_OK;
}

network_status_t hal_network_disconnect(network_handle_t handle) {
    // Platform-specific network disconnect code here
    return NETWORK_STATUS_OK;
}

network_status_t hal_network_send(network_handle_t handle,
                                const uint8_t *data, size_t length) {
    // Platform-specific network send code here
    return NETWORK_STATUS_OK;
}

network_status_t hal_network_receive(network_handle_t handle,
                                   uint8_t *buffer, size_t length,
                                   size_t *received) {
    // Platform-specific network receive code here
    return NETWORK_STATUS_OK;
}

network_status_t hal_network_set_timeout(network_handle_t handle,
                                       uint32_t timeout_ms) {
    // Platform-specific timeout setting code here
    return NETWORK_STATUS_OK;
}

// System time functions
uint64_t hal_get_time_ms(void) {
    struct timespec ts;
    #ifndef CLOCK_MONOTONIC
    #define CLOCK_MONOTONIC 1
    #endif
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
}

void hal_delay_ms(uint32_t ms) {
    #ifdef _WIN32
    Sleep(ms);
    #else
    usleep(ms * 1000);
    #endif
}

// File operations
int hal_file_open(const char *path, const char *mode, FILE **file) {
    *file = fopen(path, mode);
    if (*file == NULL) {
        return HAL_FS_ERROR_ACCESS;
    }
    return HAL_FS_OK;
}

int hal_file_close(FILE *file) {
    if (fclose(file) != 0) {
        return HAL_FS_ERROR_INVALID;
    }
    return HAL_FS_OK;
}

int hal_file_read(FILE *file, void *buffer, size_t size, size_t count, 
                 size_t *read) {
    *read = fread(buffer, size, count, file);
    if (*read != count && !feof(file)) {
        return HAL_FS_ERROR_READ;
    }
    return HAL_FS_OK;
}

int hal_file_write(FILE *file, const void *buffer, size_t size, size_t count,
                  size_t *written) {
    *written = fwrite(buffer, size, count, file);
    if (*written != count) {
        return HAL_FS_ERROR_WRITE;
    }
    return HAL_FS_OK;
}

int hal_file_copy(const char *src, const char *dest) {
    FILE *fsrc = NULL, *fdest = NULL;
    char buffer[4096];
    size_t bytes_read, bytes_written;
    int status = HAL_FS_OK;

    if (hal_file_open(src, "rb", &fsrc) != HAL_FS_OK) {
        return HAL_FS_ERROR_ACCESS;
    }

    if (hal_file_open(dest, "wb", &fdest) != HAL_FS_OK) {
        fclose(fsrc);
        return HAL_FS_ERROR_CREATE;
    }

    while (!feof(fsrc)) {
        if (hal_file_read(fsrc, buffer, 1, sizeof(buffer), &bytes_read) != HAL_FS_OK) {
            status = HAL_FS_ERROR_READ;
            break;
        }

        if (bytes_read > 0) {
            if (hal_file_write(fdest, buffer, 1, bytes_read, &bytes_written) != HAL_FS_OK) {
                status = HAL_FS_ERROR_WRITE;
                break;
            }
        }
    }

    fclose(fsrc);
    fclose(fdest);
    return status;
}

int hal_file_delete(const char *path) {
    if (remove(path) != 0) {
        return HAL_FS_ERROR_DELETE;
    }
    return HAL_FS_OK;
}

bool hal_file_exists(const char *path) {
    FILE *f = fopen(path, "r");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}

int hal_file_rename(const char *old_path, const char *new_path) {
    if (rename(old_path, new_path) != 0) {
        return HAL_FS_ERROR_INVALID;
    }
    return HAL_FS_OK;
}

// Directory operations
int hal_dir_create(const char *path) {
    #ifdef _WIN32
    if (_mkdir(path) != 0) {
    #else
    if (mkdir(path, 0755) != 0) {
    #endif
        return HAL_FS_ERROR_CREATE;
    }
    return HAL_FS_OK;
}

int hal_dir_delete(const char *path) {
    #ifdef _WIN32
    if (_rmdir(path) != 0) {
    #else
    if (rmdir(path) != 0) {
    #endif
        return HAL_FS_ERROR_DELETE;
    }
    return HAL_FS_OK;
}

bool hal_dir_exists(const char *path) {
    struct stat st ; 
    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    return false;
}

int hal_list_dir(const char *path, char ***entries, size_t *count) {
    *count = 0;
    *entries = NULL;

    #ifdef _WIN32
    WIN32_FIND_DATA find_data;
    HANDLE find_handle;
    char search_path[MAX_PATH];
    snprintf(search_path, sizeof(search_path), "%s\\*", path);

    find_handle = FindFirstFile(search_path, &find_data);
    if (find_handle == INVALID_HANDLE_VALUE) {
        return HAL_FS_ERROR_ACCESS;
    }
    #else
    DIR *dir = opendir(path);
    if (!dir) {
        return HAL_FS_ERROR_ACCESS;
    }
    #endif

    // Count entries first
    size_t capacity = 16;
    char **list = malloc(capacity * sizeof(char*));
    if (!list) {
        #ifdef _WIN32
        FindClose(find_handle);
        #else
        closedir(dir);
        #endif
        return HAL_FS_ERROR_CREATE;
    }

    #ifdef _WIN32
    do {
        if (*count >= capacity) {
            capacity *= 2;
            char **new_list = realloc(list, capacity * sizeof(char*));
            if (!new_list) {
                for (size_t i = 0; i < *count; i++) {
                    free(list[i]);
                }
                free(list);
                FindClose(find_handle);
                return HAL_FS_ERROR_CREATE;
            }
            list = new_list;
        }
        list[*count] = strdup(find_data.cFileName);
        (*count)++;
    } while (FindNextFile(find_handle, &find_data));
    FindClose(find_handle);
    #else
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (*count >= capacity) {
            capacity *= 2;
            char **new_list = realloc(list, capacity * sizeof(char*));
            if (!new_list) {
                for (size_t i = 0; i < *count; i++) {
                    free(list[i]);
                }
                free(list);
                closedir(dir);
                return HAL_FS_ERROR_CREATE;
            }
            list = new_list;
        }
        list[*count] = strdup(entry->d_name);
        (*count)++;
    }
    closedir(dir);
    #endif

    *entries = list;
    return HAL_FS_OK;
}

void hal_free_dir_list(char **entries, size_t count) {
    if (!entries) return;
    for (size_t i = 0; i < count; i++) {
        free(entries[i]);
    }
    free(entries);
}
