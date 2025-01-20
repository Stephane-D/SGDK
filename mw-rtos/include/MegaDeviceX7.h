/****************************************************************************
 * \brief MegaWiFi RTOS.
 * 
 * \author Juan Antonio (PaCHoN) 
 * \author Jesus Alonso (doragasu)
 * 
 * \date 01/2025
 ****************************************************************************/

#pragma once

#include "usb/usb_host.h"
#include "mw/MegaWiFi.h"

#define CONFIG_USB_TRANSFER_OUT_BUF_SIZE 64
#define CLIENT_NUM_EVENT_MSG        6

#define ACTION_OPEN_DEV             0x01
#define ACTION_GET_DEV_INFO         0x02
#define ACTION_TRANSFER_IN          0x04
#define ACTION_TRANSFER_OUT         0x08
#define ACTION_CLOSE_DEV            0x40
#define ACTION_EXIT                 0x80

#define MEGA_DEVICE_TAG     "MEGADEVICEX7"

class MegaDeviceX7 {
public:
    
    typedef struct {
        MegaWiFi* mw;
        SemaphoreHandle_t sem;
    }MegaDeviceX7TaskParam;

    MegaDeviceX7(MegaWiFi* mw);
    int init();

    uint8_t performActions();
    void openDevice();
    void getInfoDevice();
    
    void showInfoExtraDevice();
    esp_err_t findDescriptors();
    esp_err_t allocateInTransfer();
    esp_err_t allocateOutTransfer(size_t out_buf_len);

    void startInTransfer(void* arg);
    void startOutTransfer(void* arg);
    void closeDevice();

    static void mega_device_task(void *arg);
    static void handleEvent(const usb_host_client_event_msg_t *event_msg, void *arg);

    usb_host_client_handle_t client_hdl;
    uint8_t dev_addr = 0;
    usb_device_handle_t dev_hdl;
    uint32_t actions = 0;
    
    MegaWiFi* mw;

    const usb_ep_desc_t *in_ep_usb = NULL;
    const usb_ep_desc_t *out_ep_usb = NULL;

private:
    static void inTransferCallback(usb_transfer_t *transfer);
    static void outTransferCallback(usb_transfer_t *transfer);

    static void txTask(void *pvParameters);

    const usb_intf_desc_t *intf_desc;
    usb_transfer_t *in_xfer;
    usb_transfer_t *out_xfer;
    void *cb_arg;
    QueueHandle_t qTx;
    
};