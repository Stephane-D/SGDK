/************************************************************************
 * \brief Simple COMM driver.
 *
 * \author Juan Antonio Ruiz (PaCHoN)
 * \date   2025
 * \defgroup COMM COMM
 * \brief
 *      COMM Implementation.
*************************************** */
#include "config.h"

#if (MODULE_MEGAWIFI == 1)

#include "ext/mw/comm.h"
#include "ext/mw/16c550.h"
#include "ext/mw/ssf_ed_x7.h"
#include "ext/mw/ssf_ed_pro.h"

typedef struct CommVTable {
    void (*init)(void);
    bool (*is_present)(void);
    u8 (*read_ready)(void);
    u8 (*read)(void);
    u8 (*write_ready)(void);
    void (*write)(u8 data);
    
    u16 (*get_buff_length)(void);
    u16 (*get_tx_fifo_length)(void);
} CommVTable;

static const CommVTable Everdrive_VTable
    = { ssf_ed_x7_init, ssf_ed_x7_is_present, ssf_ed_x7_read_ready,
          ssf_ed_x7_read, ssf_ed_x7_write_ready, ssf_ed_x7_write, 
          ssf_ed_x7_get_buff_length,
          ssf_ed_x7_get_tx_fifo_length };

static const CommVTable EverdrivePro_VTable
    = { ssf_ed_pro_init, ssf_ed_pro_is_present, ssf_ed_pro_read_ready,
          ssf_ed_pro_read, ssf_ed_pro_write_ready, ssf_ed_pro_write,
          ssf_ed_pro_get_buff_length,
          ssf_ed_pro_get_tx_fifo_length };
static const CommVTable MegaWifiCart_VTable
    = { uart_init, uart_is_present, uart_rx_ready,
          uart_getc, uart_tx_ready, uart_putc, 
          uart_get_buff_length,
          uart_get_tx_fifo_length };

static const CommVTable* commTypes[] = {
    &MegaWifiCart_VTable,
    &EverdrivePro_VTable,
    &Everdrive_VTable,
};

#define COMM_TYPES 3

static const CommVTable* activeCommType = NULL;

void comm_init(void) {
    for (u8 i = 0; i < COMM_TYPES; i++) {
        const CommVTable* type = commTypes[i];
        if (type->is_present()) {
            type->init();
            activeCommType = type;
            break;
        }
    }
}
bool comm_read_ready(void) {
    if (activeCommType != NULL) {
        return activeCommType->read_ready();
    }
    return false;
}

bool comm_write_ready(void) {
    if (activeCommType != NULL) {
        return activeCommType->write_ready();
    }
    return false;
}

u8 comm_read(void) {
    if (activeCommType != NULL) {
        return activeCommType->read();
    }
    return 0;
}

void comm_write(u8 data) {
    if (activeCommType != NULL) {
        activeCommType->write(data);
    }
}

u16 comm_get_buffer_length(void) {
    if (activeCommType != NULL) {
        return activeCommType->get_buff_length();
    }
    return 0;
}

u16 comm_get_tx_fifo_length(void) {
    if (activeCommType != NULL) {
        return activeCommType->get_tx_fifo_length();
    }
    return 0;
}

COMMMode comm_mode(void) {
    if (activeCommType == &Everdrive_VTable) {
        return Everdrive;
    } else if (activeCommType == &EverdrivePro_VTable) {
        return EverdrivePro;
    } else if (activeCommType == &MegaWifiCart_VTable) {
        return MegaWifiCart;
    }
    return Discovery;
}

#endif // MODULE_MEGAWIFI COMM IMPL
