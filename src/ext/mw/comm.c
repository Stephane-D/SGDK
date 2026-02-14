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

#if (MEGAWIFI_IMPLEMENTATION & MEGAWIFI_IMPLEMENTATION_MW_CART)
#include "ext/mw/16c550.h"
#endif
#if (MEGAWIFI_IMPLEMENTATION & MEGAWIFI_IMPLEMENTATION_ED)
#include "ext/mw/ssf_ed_x7.h"
#include "ext/mw/ssf_ed_pro.h"
#endif
#if (MEGAWIFI_IMPLEMENTATION & MEGAWIFI_IMPLEMENTATION_CROSS)
// MEGAWIFI_IMPLEMENTATION_CROSS
#include "ext/serial/serial.h"
#endif

typedef struct CommVTable {
    void (*init)(void);
    bool (*is_present)(void);
    u8 (*read_ready)(void);
    u8 (*read)(void);
    u8 (*write_ready)(void);
    void (*write)(u8 data);
    
    u16 buff_length;
    u16 fifo_length;
    char* mode;
} CommVTable;

static const CommVTable commTypes[] = {
#if (MEGAWIFI_IMPLEMENTATION & MEGAWIFI_IMPLEMENTATION_ED)
    { ssf_ed_x7_init, ssf_ed_x7_is_present, ssf_ed_x7_read_ready,
          ssf_ed_x7_read, ssf_ed_x7_write_ready, ssf_ed_x7_write, 
          MW_EDX7_BUFLEN,
          MW_EDX7_TXFIFO_LEN, "Ex7" },

    { ssf_ed_pro_init, ssf_ed_pro_is_present, ssf_ed_pro_read_ready,
          ssf_ed_pro_read, ssf_ed_pro_write_ready, ssf_ed_pro_write,
          MW_EDPRO_BUFLEN,
          MW_EDPRO_TXFIFO_LEN, "EPr" },
#endif // MEGAWIFI_IMPLEMENTATION_ED
#if (MEGAWIFI_IMPLEMENTATION & MEGAWIFI_IMPLEMENTATION_MW_CART)
    { uart_init, uart_is_present, uart_rx_ready,
          uart_getc, uart_tx_ready, uart_putc, 
          UART_BUFLEN,
          UART_TX_FIFO_LEN, "MwC" },
#endif // MEGAWIFI_IMPLEMENTATION_MW_CART
#if (MEGAWIFI_IMPLEMENTATION & MEGAWIFI_IMPLEMENTATION_CROSS)
    { serial_init, serial_is_present, serial_read_ready,
          serial_read, serial_write_ready, serial_write, 
          SERIAL_BUFLEN,
          SERIAL_TXFIFO_LEN, "Pt2" },
#endif // MEGAWIFI_IMPLEMENTATION_CROSS
};

#define COMM_TYPES sizeof(commTypes) / sizeof(commTypes[0])

static const CommVTable* activeCommType = NULL;

void comm_init(void) {
    for (u8 i = 0; i < COMM_TYPES; i++) {
        const CommVTable* type = &commTypes[i];
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
        return activeCommType->buff_length;
    }
    return 0;
}

u16 comm_get_tx_fifo_length(void) {
    if (activeCommType != NULL) {
        return activeCommType->fifo_length;
    }
    return 0;
}

char* comm_mode(void) {
    if (activeCommType != NULL) {
        return activeCommType->mode;
    }
    return "Dis";
}

#endif // MODULE_MEGAWIFI COMM IMPL
