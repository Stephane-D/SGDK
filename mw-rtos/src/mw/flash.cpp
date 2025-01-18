#include "mw/flash.h"

int Flash::flash_init(void)
{
	p = esp_partition_find_first(MW_DATA_PART_TYPE, MW_USER_PART_SUBTYPE, MW_USER_PART_LABEL);

	if (!p) {
		return 1;
	}

	return 0;
}

int Flash::flash_write(uint32_t addr, uint16_t len, const char *data)
{
	return esp_partition_write(p, addr, data, len);
}

int Flash::flash_read(uint32_t addr, uint16_t len, char *data)
{
	return esp_partition_read(p, addr, data, len);
}

int Flash::flash_erase(uint16_t sect)
{
	return esp_partition_erase_range(p, sect<<12, 1<<12);
}

