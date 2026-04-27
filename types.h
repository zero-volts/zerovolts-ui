#ifndef TYPES_H
#define TYPES_H
#ifdef __cplusplus
extern "C" {
#endif

#define UNKNOWN_NAME "Unknown"
#define BT_ALLOWD_MAX_DEVICES 20

typedef struct {
    char name[32];
    char mac[18];
    int rssi;
    char manufacturer[32];
    char appearance[32];
    char service[32];
    int connectable;
} device_t;

typedef enum {
    UI_IDLE = 0,
    UI_LOADING,
    UI_DONE
} ui_status_t;

#ifdef __cplusplus
}
#endif

#endif /* TYPES_H */