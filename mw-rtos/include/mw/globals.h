/****************************************************************************
 * \brief MegaWiFi RTOS.
 * 
 * \author Juan Antonio (PaCHoN) 
 * \author Jesus Alonso (doragasu)
 * 
 * \date 01/2025
 ****************************************************************************/

#pragma once
/// Base physical address for the SPI flash chip
#define SPI_FLASH_BASE	0x40200000  // 
/// Compute the physical address for the specified SPI flash cip address
#define SPI_FLASH_ADDR(flash_addr)	(SPI_FLASH_BASE + (flash_addr))

/// Number of bits of a flash sector
#define MW_FLASH_SECT_BITS		12
/// Internal flash sector length y bytes
#define MW_FLASH_SECT_LEN		(1<<MW_FLASH_SECT_BITS)

/// Computes the sector length used by a byte length, rounding up to the
/// number of used sectors.
#define MW_FLASH_SECT_ROUND(length)	(((length + (MW_FLASH_SECT_LEN - 1)) \
			>>MW_FLASH_SECT_BITS)<<MW_FLASH_SECT_BITS)

/// MegaWiFi partition data type
#define MW_DATA_PART_TYPE		(esp_partition_type_t)0x40

/// User data partition subtype
#define MW_USER_PART_SUBTYPE		(esp_partition_subtype_t)0x00
/// User data partitionlabel
#define MW_USER_PART_LABEL		"user_data"

/// User config partition subtype
#define MW_CFG_PART_SUBTYPE		(esp_partition_subtype_t)0x01
/// User config partitionlabel
#define MW_CFG_PART_LABEL		"mw_cfg"

#define MW_CERT_PART_SUBTYPE		(esp_partition_subtype_t)0x02
#define MW_CERT_PART_LABEL		"cert"

/// Maximum length of a certificate in bytes
#define MW_CERT_MAXLEN		(8 * 1024 - 2 * sizeof(uint32_t))

