#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_intr_alloc.h"
#include "USBHost.h"
#include "MegaDeviceX7.h"
#include "mw/MegaWiFi.h"

#define DAEMON_TASK_PRIORITY    2
#define CLASS_TASK_PRIORITY     3


void setup() {
    esp_log_level_set("*", ESP_LOG_DEBUG); 
    mw->MwInit();
	ESP_LOGD(MEGA_DEVICE_TAG, "Init done!");
}

void loop() {    
    SemaphoreHandle_t signaling_sem = xSemaphoreCreateBinary();

    MegaDeviceX7::MegaDeviceX7TaskParam mdx7TaskParam = {
        mw, signaling_sem
    };

    TaskHandle_t daemon_task_hdl;
    TaskHandle_t class_driver_task_hdl;
    //Create daemon task
    xTaskCreatePinnedToCore(USBHost::host_lib_daemon_task,
                            "USBHost",
                            4096,
                            (void *)signaling_sem,
                            DAEMON_TASK_PRIORITY,
                            &daemon_task_hdl,
                            0);
    //Create the class driver task
    xTaskCreatePinnedToCore(MegaDeviceX7::mega_device_task,
                            "MegaDeviceX7",
                            4096,
                            (void *)&mdx7TaskParam,
                            CLASS_TASK_PRIORITY,
                            &class_driver_task_hdl,
                            1);

    vTaskDelay(10);     //Add a short delay to let the tasks run

    //Wait for the tasks to complete
    for (int i = 0; i < 2; i++) {
        xSemaphoreTake(signaling_sem, portMAX_DELAY);
    }

    //Delete the tasks
    vTaskDelete(class_driver_task_hdl);
    vTaskDelete(daemon_task_hdl);
}