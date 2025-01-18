#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "MegaDeviceX7.h"
#define LOG_LOCAL_LEVEL CORE_DEBUG_LEVEL
#include "string.h"

MegaDeviceX7::MegaDeviceX7(MegaWiFi* mw){
    this->mw = mw;
}

int MegaDeviceX7::init(){
    if (pdPASS != xTaskCreate(txTask, "TXTASK", MW_TXTASK_STACK_LEN, this, MW_TXTASK_PRIO, NULL)) {
		ESP_LOGE(MW_TAG,"could not create TXTASK task!");
        ESP_LOGE(MW_TAG,"fatal error during initialization");
        return 1;
	}
    return 0;
}

void MegaDeviceX7::openDevice() {
    assert(dev_addr != 0);
    ESP_LOGD(MEGA_DEVICE_TAG, "Opening device at address %d", dev_addr);
    ESP_ERROR_CHECK(usb_host_device_open(client_hdl, dev_addr, &dev_hdl));
    actions &= ~ACTION_OPEN_DEV;
    actions |= ACTION_GET_DEV_INFO;
}

void MegaDeviceX7::getInfoDevice() {
  
    ESP_ERROR_CHECK(findDescriptors());    

    ESP_LOGD(MEGA_DEVICE_TAG, "Claim Iface %d",intf_desc->bInterfaceNumber);
    ESP_ERROR_CHECK(usb_host_interface_claim(client_hdl, dev_hdl, intf_desc->bInterfaceNumber, 0));
                    
    actions &= ~ACTION_GET_DEV_INFO;
    actions |= ACTION_TRANSFER_IN;
}

void MegaDeviceX7::showInfoExtraDevice() {

    assert(dev_hdl != NULL);
    usb_device_info_t dev_info;
    ESP_ERROR_CHECK(usb_host_device_info(dev_hdl, &dev_info));
    if (dev_info.str_desc_manufacturer) {
        ESP_LOGD(MEGA_DEVICE_TAG, "Getting Manufacturer string descriptor");
        usb_print_string_descriptor(dev_info.str_desc_manufacturer);
    }
    if (dev_info.str_desc_product) {
        ESP_LOGD(MEGA_DEVICE_TAG, "Getting Product string descriptor");
        usb_print_string_descriptor(dev_info.str_desc_product);
    }
    if (dev_info.str_desc_serial_num) {
        ESP_LOGD(MEGA_DEVICE_TAG, "Getting Serial Number string descriptor");
        usb_print_string_descriptor(dev_info.str_desc_serial_num);
    }
}

esp_err_t MegaDeviceX7::findDescriptors() {
    const usb_config_desc_t *config_desc;

    ESP_ERROR_CHECK(usb_host_get_active_config_descriptor(dev_hdl, &config_desc));

    bool interface_found = false;
    int desc_offset = 0;
    const usb_standard_desc_t *this_desc = (const usb_standard_desc_t *)config_desc;

    do {
        this_desc = usb_parse_next_descriptor_of_type(this_desc, config_desc->wTotalLength, USB_B_DESCRIPTOR_TYPE_INTERFACE, &desc_offset);

        if (this_desc == NULL)
            break; // Reached end of configuration descriptor

        const usb_intf_desc_t *intf_desc = (const usb_intf_desc_t *)this_desc;
        assert(intf_desc);

        bool foundEp = false;
        int temp_offset = desc_offset;
        for (int i = 0; i < 2; i++) {
            const usb_ep_desc_t *this_ep = usb_parse_endpoint_descriptor_by_index(intf_desc, i, config_desc->wTotalLength, &desc_offset);
            assert(this_ep);
            uint8_t ep_type = USB_BM_ATTRIBUTES_XFERTYPE_MASK & this_ep->bmAttributes;
            if (ep_type == USB_BM_ATTRIBUTES_XFER_BULK) {  // found bulk EP
                foundEp = true;
                if (USB_EP_DESC_GET_EP_DIR(this_ep) == 0) {  // found OUT EP
                    out_ep_usb = this_ep;
                    ESP_LOGD(MEGA_DEVICE_TAG, "found usb interface BULK-OUT ep address %d", this_ep->bEndpointAddress);
                } else {  // found IN EP
                    in_ep_usb = this_ep;
                    ESP_LOGD(MEGA_DEVICE_TAG, "found usb interface BULK-IN ep address %d", this_ep->bEndpointAddress);
                }
            }
            desc_offset = temp_offset;
        }

        if (foundEp) {
            this->intf_desc = intf_desc;
            interface_found = true;
        }
    } while (!interface_found);

    if (interface_found) {
        this->intf_desc = intf_desc;
        return ESP_OK;
    }
    return ESP_ERR_NOT_FOUND;
}

esp_err_t MegaDeviceX7::allocateOutTransfer(size_t out_buf_len) {

    usb_host_transfer_alloc(out_buf_len, 0, &out_xfer);
    out_xfer->callback = outTransferCallback;
    out_xfer->bEndpointAddress = out_ep_usb->bEndpointAddress;
    out_xfer->device_handle = dev_hdl;
    out_xfer->context = this;
    return ESP_OK;
}

esp_err_t MegaDeviceX7::allocateInTransfer() {
    usb_host_transfer_alloc(USB_EP_DESC_GET_MPS(in_ep_usb), 0, &in_xfer);
    in_xfer->callback = inTransferCallback;
    in_xfer->num_bytes = USB_EP_DESC_GET_MPS(in_ep_usb);
    in_xfer->bEndpointAddress = in_ep_usb->bEndpointAddress;
    in_xfer->device_handle = dev_hdl;
    in_xfer->context = this;
    return ESP_OK;
}

void MegaDeviceX7::startInTransfer(void* arg)
{
	cb_arg = arg;
    ESP_ERROR_CHECK(allocateInTransfer());
	ESP_ERROR_CHECK(usb_host_transfer_submit(in_xfer));
}

void MegaDeviceX7::startOutTransfer(void* arg)
{
    MegaDeviceX7TxMsg *data = (MegaDeviceX7TxMsg*)arg;
    ESP_ERROR_CHECK(allocateOutTransfer(data->size));
    out_xfer->context=this;
    memcpy((char *)out_xfer->data_buffer, (const char *)data->buffer, data->size);
    out_xfer->num_bytes =  data->size;
	// kick off OUT transfer
	//xSemaphoreGive((SemaphoreHandle_t)everdrive_dev->data.out_xfer->context);
    ESP_LOGI(MEGA_DEVICE_TAG,"OUTPUT[%d]: ",  data->size);
    ESP_LOG_BUFFER_HEX_LEVEL(MEGA_DEVICE_TAG, out_xfer->data_buffer,  data->size, ESP_LOG_INFO); 
    usb_host_transfer_submit(out_xfer);
}

void MegaDeviceX7::closeDevice() {
    assert(dev_hdl != nullptr);
    ESP_LOGD(MEGA_DEVICE_TAG, "Closing device at address %d", dev_addr);
    ESP_ERROR_CHECK(usb_host_device_close(client_hdl, dev_hdl));
    dev_hdl = nullptr;
    dev_addr = 0;
    actions &= ~ACTION_CLOSE_DEV;
    actions |= ACTION_EXIT;
}

uint8_t MegaDeviceX7::performActions() {
    if (actions == 0) {
        usb_host_client_handle_events(client_hdl, portMAX_DELAY);//
	    if(dev_addr != 0 && !actions)
            actions |= ACTION_TRANSFER_IN;
    } else {
        if (actions & ACTION_OPEN_DEV) {
            openDevice();
            showInfoExtraDevice();
        }
        if (actions & ACTION_GET_DEV_INFO) {
            getInfoDevice();
        }
        if (actions & ACTION_TRANSFER_IN) {
            mw->lsd->LsdRxBufFree();
            startInTransfer(this);
	        actions &= ~ACTION_TRANSFER_IN;
        }
        if (actions & ACTION_CLOSE_DEV) {
            closeDevice();
        }
        if (actions & ACTION_EXIT) {
            return 0U;
        }
    }
    return 1U;
}

void MegaDeviceX7::handleEvent(const usb_host_client_event_msg_t *event_msg, void *arg) {
    MegaDeviceX7* megaDeviceX7 = (MegaDeviceX7*)arg;
    switch (event_msg->event) {
    case USB_HOST_CLIENT_EVENT_NEW_DEV:
        if (megaDeviceX7->dev_addr == 0) {
            megaDeviceX7->dev_addr = event_msg->new_dev.address;
            megaDeviceX7->actions |= ACTION_OPEN_DEV;
        }
        break;
    case USB_HOST_CLIENT_EVENT_DEV_GONE:
        if (megaDeviceX7->dev_hdl != nullptr) {
            megaDeviceX7->actions = ACTION_CLOSE_DEV;
        }
        break;
    default:
        abort();
    }
}

void MegaDeviceX7::mega_device_task(void *arg)
{
    MegaDeviceX7TaskParam* mdx7TaskParam = (MegaDeviceX7TaskParam*)arg;
    SemaphoreHandle_t signaling_sem = mdx7TaskParam->sem;
    MegaDeviceX7* megaDevice = new MegaDeviceX7(mdx7TaskParam->mw);
    if(megaDevice->init()) return;

    //Wait until daemon task has installed USB Host Library
    xSemaphoreTake(signaling_sem, portMAX_DELAY);

    ESP_LOGD(MEGA_DEVICE_TAG, "Registering Client");
    usb_host_client_config_t client_config = {
        .is_synchronous = false,    //Synchronous clients currently not supported. Set this to false
        .max_num_event_msg = CLIENT_NUM_EVENT_MSG,
        .async = {
            .client_event_callback = MegaDeviceX7::handleEvent,
            .callback_arg = (void *) megaDevice,
        },
    };
    ESP_ERROR_CHECK(usb_host_client_register(&client_config, &megaDevice->client_hdl));

    while (megaDevice->performActions());

    ESP_LOGD(MEGA_DEVICE_TAG, "Deregistering Client");
    ESP_ERROR_CHECK(usb_host_client_deregister(megaDevice->client_hdl));

    //Wait to be deleted
    xSemaphoreGive(signaling_sem);
    vTaskSuspend(NULL);
}

void MegaDeviceX7::outTransferCallback(usb_transfer_t *transfer)
{
    //assert(transfer->context);
    ESP_LOGD(MEGA_DEVICE_TAG, "outTransferCallback: BULK OUT transfer completed");     
    MegaDeviceX7 *megaDevice = (MegaDeviceX7 *)transfer->context;
    ESP_ERROR_CHECK(usb_host_transfer_free(transfer));  
}

void MegaDeviceX7::inTransferCallback(usb_transfer_t *transfer)
{
    int totalBytes = transfer->actual_num_bytes;
    uint8_t* buffer = transfer->data_buffer;
    MegaDeviceX7 *megaDevice = (MegaDeviceX7 *)transfer->context;
    if(transfer->actual_num_bytes > 1 && transfer->data_buffer[0]==0x31 && (transfer->data_buffer[1]==0x60 || transfer->data_buffer[1]==0x00)){ //SKIP USB HEADER 0x31 0x60        
        totalBytes = totalBytes - 2;
        buffer = transfer->data_buffer + 2;
    }
    if(totalBytes > 0){    
        ESP_LOGI(MEGA_DEVICE_TAG,"INPUT[%d]:", totalBytes);
        ESP_LOG_BUFFER_HEX_LEVEL(MEGA_DEVICE_TAG, buffer, totalBytes, ESP_LOG_INFO); 
        megaDevice->mw->lsd->LsdRecv(buffer, totalBytes);
        ESP_LOGD(MEGA_DEVICE_TAG, "inTransferCallback: BULK IN transfer completed");             
    }
    ESP_ERROR_CHECK(usb_host_transfer_free(transfer));  
}

void MegaDeviceX7::txTask(void *pvParameters) {
	MegaDeviceX7 *megaDevice = (MegaDeviceX7 *)pvParameters;
	MegaDeviceX7TxMsg data;
	while(1) { 
        ESP_LOGD(MEGA_DEVICE_TAG, "Waiting TX transfer ...");
		if (xQueueReceive(megaDevice->mw->lsd->qTx, &data, portMAX_DELAY)) {            
			megaDevice->startOutTransfer(&data);
		}
	}
}