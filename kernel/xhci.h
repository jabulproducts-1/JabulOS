#ifndef XHCI_H
#define XHCI_H

#include "jabulos.h"

typedef struct {
    bool present;
    u8 bus;
    u8 device;
    u8 function;
    u64 mmio_base;
    u32 page_size;
    u32 max_slots;
} xhci_controller_t;

void xhci_initialize(void);
void xhci_poll(void);

#endif
