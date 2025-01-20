/****************************************************************************
 * \brief MegaWiFi RTOS.
 * 
 * \author Juan Antonio (PaCHoN) 
 * \author Jesus Alonso (doragasu)
 * 
 * \date 01/2025
 ****************************************************************************/

#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "usb/usb_host.h"
#include "esp_log.h"
#include "USBHost.h"


void USBHost::host_lib_daemon_task(void *arg){
    SemaphoreHandle_t signaling_sem = (SemaphoreHandle_t)arg;

    ESP_LOGI(TAG, "Installing USB Host Library");
    usb_host_config_t host_config = {
        .skip_phy_setup = false,
        .intr_flags = ESP_INTR_FLAG_LEVEL1
    };
    ESP_ERROR_CHECK(usb_host_install(&host_config));

    //Signal to the class driver task that the host library is installed
    xSemaphoreGive(signaling_sem);
    vTaskDelay(10); //Short delay to let client task spin up

    bool has_clients = true;
    bool has_devices = true;
    while (has_clients || has_devices) {
        uint32_t event_flags;
        ESP_ERROR_CHECK(usb_host_lib_handle_events(portMAX_DELAY, &event_flags));
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            has_clients = false;
        }
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE) {
            has_devices = false;
        }
    }
    ESP_LOGI(TAG, "No more clients and devices");

    //Uninstall the USB Host Library
    ESP_ERROR_CHECK(usb_host_uninstall());
    //Wait to be deleted
    xSemaphoreGive(signaling_sem);
    vTaskSuspend(NULL);
}