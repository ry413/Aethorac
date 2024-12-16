#ifndef MY_MQTT_H
#define MY_MQTT_H

#include "esp_err.h"
#include <string>

void mqtt_app_start();
void mqtt_app_stop(void);
void mqtt_app_cleanup(void);
void mqtt_publish_message(const std::string& message, int qos, int retain);

void report_states();
#endif // MY_MQTT_H