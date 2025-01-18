#pragma once
#include <Arduino.h>
#include <esp_partition.h>
#include "globals.h"

class Flash {
public:

	Flash(){};

	const esp_partition_t *p = NULL;

	int flash_init(void);
	int flash_write(uint32_t addr, uint16_t len, const char *data);
	int flash_read(uint32_t addr, uint16_t len, char *data);
	int flash_erase(uint16_t sect);

private:
};