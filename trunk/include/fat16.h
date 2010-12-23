/*
 * File:   fat16.h
 * Author: KRIK
 */

#ifndef _FAT16_H
#define	_FAT16_H


#ifdef FAT16_SUPPORT

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

/*
00h	�������� �������� + NOP	3
03h	OEM ��������	8
0Bh	���������� ������ � ������� �� ������� �����	2
0Dh	���������� �������� � ��������	1
0Eh	���������� ����������������� ��������	2
10h	���������� ����� FAT, ��� �������, 2	1
11h	���������� ��������� ������� � �������� ��������: 512 ��� FAT16	2
13h	���������� �������� � ����� ��������� ��������	2
15h	��� ��������: F8 ��� ������� ������; F0 ��� ������.	1
16h	�������� �� FAT	2
18h	�������� �� �������	2
1Ah	����� �������	2
1Ch	����������������� ������� � ������ �������� �����	4
20h	����� ����� �������� � �������	4
24h	����� ���������. ������ ������ ���� ����� ����������� 80h, ������ 81h � �.�.	2
26h	����������� ����������� ��������� (29h)	1
27h	�������� ����� �������� ������	4
2Bh	������������ �������� ������	11
36h	�������� FAT (FAT16)	8
3Eh	����������� ���	448
1FEh	��������� (55h AAh)
 * */

//52 byte size

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

#endif  /* FAT16_SUPPORT */


#endif	/* _FAT16_H */
