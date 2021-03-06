/* Rename this file to config.h */

#ifndef __CONFIG_H__
#define __CONFIG_H__

/* Debug mode enabling */
#define DEBUG
/////////////////////////

#define BUF_SIZE            512

#define ESP_BASE_NAME               "ESP-BASE"                  // name of base ESP, who send WiFi data to other ESP
#define ESP_AP_SSID			        "ESP-AP"
#define ESP_AP_PASS			        "123456789"
#define ESP_SERVER_IP		        192,168,100,1
#define ESP_SERVER_PORT		        9999
#define TIME_ZONE                   4
#define WIFI_RECONNECT_INTERVAL     10000                       // (ms)
#define WIFI_RECONNECT_TRIES        4                           // Max 14
#define SERVER_RECONNECT_INTERVAL   10000                       // (ms)
#define SERVER_RECONNECT_TRIES      30                          // Max 14


#endif