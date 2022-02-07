/**
 *  \file fat16.h
 *  \brief FAT16 file system support
 *  \author Krikzz
 *  \date XX/20XX
 *
 * This unit provides basic FAT16 file system support through everdrive flash cart hardware
 */

#ifndef _FAT16_H
#define _FAT16_H


#if (MODULE_FAT16 != 0)

#define FAT16_DIR_SIZE 32
#define FAT16_TYPE_FILE  0x20
#define FAT16_TYPE_DIR  0x10


typedef struct {
    u8 pointer[3]; //0
    u8 oem_name[8]; //3
    u16 byte_per_sector; //11
    u8 sector_per_cluster; //13
    u16 reserved_sectors; //14
    u8 fat_copys; //16
    u16 root_size; //17
    u16 small_partition_sectros; //19
    u8 drive_type; //21
    u16 sectors_per_fat; //22
    u16 sectors_per_track; //24
    u16 heads; //26
    u32 first_reserved_sectors; //28
    u32 total_partition_sectors; //32
    u16 drive_number; //36
    u8 extendet_boot_signature; //38
    u32 serial_number; //39
    u8 drive_name[11]; //43
    u8 fat_name[8]; //54
} Fat16PBR;


typedef struct {
    u8 name[14];
    u8 long_name[38];
    u8 flags;
    u16 entry;
    u32 size;
    u32 rec_addr;
} Fat16Record;

typedef struct {
    Fat16Record records[FAT16_DIR_SIZE];
    u16 entry;
    volatile u16 size;
} Fat16Dir;

typedef struct {
    Fat16Record *record;
    u8 sectror_buff[512];
    u16 cluster;
    u32 pos;
    u32 addr_buff;
    u8 sector;
} Fat16File;


extern Fat16PBR fat16_pbr;


u8 fat16Init();
u8 fat16OpenDir(u16 entry, Fat16Dir *dir);
u8 fat16OpenFile(Fat16Record *rec, Fat16File *file);
u8 fat16ReadNextSector(Fat16File *file);
u8 fat16WriteNextSector(Fat16File *file);
u8 fat16DeleteRecord(Fat16Record *rec);
u8 fat16CreateRecord(Fat16Record *rec, Fat16Dir *dir);
u8 fat16SkipSectors(Fat16File *file, u16 num);

#endif  /* MODULE_FAT16 */


#endif  /* _FAT16_H */
