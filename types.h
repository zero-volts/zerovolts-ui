#ifndef TYPES_H
#define TYPES_H
#ifdef __cplusplus
extern "C" {
#endif

#define UNKNOWN_NAME "Unknown"
#define BT_ALLOWD_MAX_DEVICES 20
#define BT_MAX_SERVICES 20
#define BT_MAX_CHARS_PER_SERVICE 16
#define BT_UUID_STR_LEN 37

typedef struct {
    char name[32];
    char mac[18];
    int rssi;
    char manufacturer[32];
    char appearance[32];
    char service[32];
    int connectable;
    int addr_type;
} device_t;

typedef struct {
    int char_index;
    char uuid[BT_UUID_STR_LEN];
    unsigned int props;
    unsigned int handle;
} bt_characteristic_t;

typedef struct {
    int svc_index;
    char uuid[BT_UUID_STR_LEN];
    bt_characteristic_t chars[BT_MAX_CHARS_PER_SERVICE];
    int chars_count;
} bt_service_t;

typedef enum {
    UI_IDLE = 0,
    UI_LOADING,
    UI_DONE
} ui_status_t;

typedef enum {
    BT_CONN_IDLE = 0,
    BT_CONN_CONNECTING,
    BT_CONN_CONNECTED,
    BT_CONN_DISCOVERING,
    BT_CONN_READY,
    BT_CONN_FAILED,
    BT_CONN_LOST,
    BT_CONN_DISCONNECTED
} bt_conn_status_t;

#ifdef __cplusplus
}
#endif

#endif /* TYPES_H */