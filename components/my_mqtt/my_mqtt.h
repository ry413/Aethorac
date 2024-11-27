#ifndef MY_MQTT_H
#define MY_MQTT_H

#include "esp_err.h"

void mqtt_app_start();
void mqtt_app_stop(void);
void mqtt_app_cleanup(void);
void mqtt_publish_message(const char *message, int qos, int retain);

#endif // MY_MQTT_H