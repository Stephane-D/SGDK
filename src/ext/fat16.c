#include "types.h"

#include "ext/fat16.h"
#include "ext/everdrive.h"


#if (MODULE_FAT16 != 0)

void fat16loadRecord(Fat16Record *record, u8 *data);
u8 fat16LoadFatTableSector(u32 sector_idx);
u8 fat16OpenRootDir(Fat16Dir *dir);
u8 fat16GetDirClustersNum(u16 *num, u32 entry);
u8 fat16GetFatTableRecord(u16 cluster, u16 *val);
u8 fat16SetFatTableRecord(u16 cluster, u16 val);
u8 fat16ApplyFatTableChange();

volatile u8 *sector_buff;

Fat16PBR fat16_pbr;

u8 fat16_buff[1024];
u32 fat16_fat_base;
u32 fat16_root_base;
u16 fat16_fat_table_buff[256];
u8 fat16_current_fat_table_sector;
u8 fat16_fat_table_buff_changed;
u32 fat16_data_start;
u16 cluster_size;


int fat16LoadPbr() {

    u8 *buff_ptr = fat16_buff;
    u8 i;
    //fat16_pbr = (Fat16PBR *) fat16_pbr_data;
    if (evd_mmcRdBlock(0, fat16_buff) != 0)return 1;


    for (i = 0; i < 3; i++)fat16_pbr.pointer[i] = *buff_ptr++;

    for (i = 0; i < 8; i++)fat16_pbr.oem_name[i] = *buff_ptr++;

    fat16_pbr.byte_per_sector = *buff_ptr++;
    fat16_pbr.byte_per_sector |= *buff_ptr++ << 8;

    fat16_pbr.sector_per_cluster = *buff_ptr++;

    fat16_pbr.reserved_sectors = *buff_ptr++;
    fat16_pbr.reserved_sectors |= *buff_ptr++ << 8;

    fat16_pbr.fat_copys = *buff_ptr++;

    fat16_pbr.root_size = *buff_ptr++;
    fat16_pbr.root_size |= *buff_ptr++ << 8;

    fat16_pbr.small_partition_sectros = *buff_ptr++;
    fat16_pbr.small_partition_sectros |= *buff_ptr++ << 8;

    fat16_pbr.drive_type = *buff_ptr++;

    fat16_pbr.sectors_per_fat = *buff_ptr++;
    fat16_pbr.sectors_per_fat |= *buff_ptr++ << 8;

    fat16_pbr.sectors_per_track = *buff_ptr++;
    fat16_pbr.sectors_per_track |= *buff_ptr++ << 8;

    fat16_pbr.heads = *buff_ptr++;
    fat16_pbr.heads |= *buff_ptr++ << 8;

    fat16_pbr.first_reserved_sectors = *buff_ptr++;
    fat16_pbr.first_reserved_sectors |= *buff_ptr++ << 8;
    fat16_pbr.first_reserved_sectors |= *buff_ptr++ << 16;
    fat16_pbr.first_reserved_sectors |= *buff_ptr++ << 24;

    fat16_pbr.total_partition_sectors = *buff_ptr++;
    fat16_pbr.total_partition_sectors |= *buff_ptr++ << 8;
    fat16_pbr.total_partition_sectors |= *buff_ptr++ << 16;
    fat16_pbr.total_partition_sectors |= *buff_ptr++ << 24;

    fat16_pbr.drive_number = *buff_ptr++;
    fat16_pbr.drive_number |= *buff_ptr++ << 8;

    fat16_pbr.extendet_boot_signature = *buff_ptr++;

    fat16_pbr.serial_number = *buff_ptr++;
    fat16_pbr.serial_number |= *buff_ptr++ << 8;
    fat16_pbr.serial_number |= *buff_ptr++ << 16;
    fat16_pbr.serial_number |= *buff_ptr++ << 24;

    for (i = 0; i < 11; i++)fat16_pbr.drive_name[i] = *buff_ptr++;
    for (i = 0; i < 8; i++)fat16_pbr.fat_name[i] = *buff_ptr++;


    return 0;
}

u8 fat16Init() {

    //u16 i;
    if (evd_mmcInit() != 0)return 1;
    if (fat16LoadPbr() != 0)return 2;

    fat16_fat_base = fat16_pbr.reserved_sectors * fat16_pbr.byte_per_sector;

    fat16_root_base = fat16_fat_base + fat16_pbr.sectors_per_fat * fat16_pbr.byte_per_sector * fat16_pbr.fat_copys;

    if (evd_mmcRdBlock(fat16_fat_base, (u8 *) fat16_fat_table_buff) != 0)return 3;
    fat16_current_fat_table_sector = 0;
    fat16_fat_table_buff_changed = 0;
    fat16_data_start = fat16_root_base + 16384;
    cluster_size = fat16_pbr.byte_per_sector * fat16_pbr.sector_per_cluster;


    return 0;
}

u8 fat16OpenDir(u16 entry, Fat16Dir *dir) {

    if (entry == 0)return fat16OpenRootDir(dir);
    u8 *buff = fat16_buff + 512;
    u32 addr;
    u16 i;
    u16 u;

    dir->size = 0;
    dir->entry = entry;
    for (;;) {

        addr = (entry - 2) * cluster_size + fat16_data_start;
        for (i = 0; i < cluster_size; i += 512) {

            if (evd_mmcRdBlock(addr + i - 512, fat16_buff) != 0)return 1;
            if (evd_mmcRdBlock(addr + i, fat16_buff + 512) != 0)return 2;

            for (u = 0; u < 512; u += 32) {

                if (buff[u] > 0x2f && buff[u] < 0x60 && (buff[u + 0x0b] & 0x30) != 0) {
                    dir->records[dir->size].rec_addr = addr + i + u;
                    fat16loadRecord(&dir->records[dir->size++], &buff[u]);
                }

                if (dir->size == FAT16_DIR_SIZE)return 0;
            }
        }

        //if (fat16GetNextFileCluster(&entry) != 0)return 2;
        if (fat16GetFatTableRecord(entry, &entry) != 0)return 3;
        if (entry == 0xffff)break;
    }


    return 0;
}

u8 fat16OpenRootDir(Fat16Dir *dir) {


    u32 addr = fat16_root_base;
    u16 i;
    u16 u;
    dir->size = 0;
    dir->entry = 0;
    u16 root_size = FAT16_DIR_SIZE > 512 ? 512 : FAT16_DIR_SIZE;
    u8 *buff = fat16_buff + 512;


    for (i = 0; i < 16384 && dir->size < root_size; i += 512) {

        if (evd_mmcRdBlock(addr + i - 512, fat16_buff) != 0)return 1;
        if (evd_mmcRdBlock(addr + i, fat16_buff + 512) != 0)return 2;

        for (u = 0; u < 512 && dir->size < root_size; u += 32) {
            if (buff[u] > 0x2f && buff[u] < 0x60 && (buff[u + 0x0b] & 0x30) != 0) {
                dir->records[dir->size].rec_addr = addr + i + u;
                fat16loadRecord(&dir->records[dir->size++], &buff[u]);
            }
        }
    }

    return 0;
}

u8 long_name_buff[53];

void loadLongName(Fat16Record *record, u8 *data) {

    u8 i = 0;
    data -= 32;
    if (data[0x0b] != 0x0f) {
        //record->long_name[0] = 0;
        for (i = 0; i < 14; i++)record->long_name[i] = record->name[i];
        return;
    }
    u8 *name = record->long_name;
    u8 name_len = 0;


    for (;;) {

        for (i = 1; i < 11; i += 2) {
            long_name_buff[name_len++] = data[i];
        }
        long_name_buff[name_len++] = data[14];
        for (i = 16; i < 26; i += 2) {
            long_name_buff[name_len++] = data[i];
        }
        //*name++ = data[28];
        //*name++ = data[30];
        long_name_buff[name_len++] = data[28];
        long_name_buff[name_len++] = data[30];

        if ((data[0] & 64) != 0)break;
        data -= 32;
        //name_len += 13;
        if (name_len + 1 >= sizeof (record->long_name)) {
            long_name_buff[name_len++] = data[i];
            break;
        }
    }

    i = 0;
    for (i = 0; i < sizeof (record->long_name); i++)*name++ = long_name_buff[i];


}

void fat16loadRecord(Fat16Record *record, u8 *data) {

    u8 i;
    for (i = 0; i < 11; i++) {
        record->name[i] = data[i];
    }
    record->name[i] = 0;

    loadLongName(record, data);

    record->flags = data[0x0b];
    record->entry = data[0x1a] | data[0x1b] << 8;
    record->size = data[0x1c] | data[0x1d] << 8 | data[0x1e] << 16 | data[0x1f] << 24;
}

u8 fat16OpenFile(Fat16Record *rec, Fat16File *file) {

    if ((rec->flags & FAT16_TYPE_DIR) != 0)return 1;
    if ((rec->flags & FAT16_TYPE_FILE) == 0)return 1;
    file->record = rec;
    file->pos = 0;
    file->cluster = rec->entry;
    file->sector = 0;
    file->addr_buff = (file->cluster - 2) * cluster_size + fat16_data_start;

    return 0;
}

u8 fat16SkipSectors(Fat16File *file, u16 num) {


    while (num--) {
        if (file->pos >= file->record->size)return 1;
        if (file->sector == fat16_pbr.sector_per_cluster) {
            //fat16GetNextFileCluster(&file->cluster);
            if (fat16GetFatTableRecord(file->cluster, &file->cluster) != 0)return 2;
            file->sector = 0;
            file->addr_buff = (file->cluster - 2) * cluster_size + fat16_data_start;
        }



        file->sector++;
        file->addr_buff += 512;

        if (file->pos <= file->record->size - 512) {
            file->pos += 512;
        } else {
            file->pos = file->record->size;
        }
    }


    return 0;
}

u8 fat16ReadNextSector(Fat16File *file) {


    if (file->pos >= file->record->size)return 1;
    if (file->sector == fat16_pbr.sector_per_cluster) {
        //fat16GetNextFileCluster(&file->cluster);
        if (fat16GetFatTableRecord(file->cluster, &file->cluster) != 0)return 2;
        file->sector = 0;
        file->addr_buff = (file->cluster - 2) * cluster_size + fat16_data_start;
    }


    if (evd_mmcRdBlock(file->addr_buff, file->sectror_buff) != 0)return 3;

    file->sector++;
    file->addr_buff += 512;

    if (file->pos <= file->record->size - 512) {
        file->pos += 512;
    } else {
        file->pos = file->record->size;
    }



    return 0;
}

u8 fat16DeleteRecord(Fat16Record *rec) {

    u16 cluster = rec->entry;
    u16 prev_cluster;
    u16 in_sector_addr;
    u32 addr;

    while (cluster != 0xffff && rec->size != 0) {

        prev_cluster = cluster;
        if (fat16GetFatTableRecord(cluster, &cluster) != 0)return 1;
        if (cluster == 0)return 2;
        if (fat16SetFatTableRecord(prev_cluster, 0) != 0)return 3;
    }

    if (fat16ApplyFatTableChange() != 0)return 4;

    if (evd_mmcRdBlock(rec->rec_addr / 512 * 512, fat16_buff) != 0)return 5;
    in_sector_addr = rec->rec_addr % 512;
    addr = rec->rec_addr / 512 * 512;
    fat16_buff[in_sector_addr] = 0xe5;



    for (;;) {

        if (addr + in_sector_addr == fat16_root_base) {
            if (evd_mmcWrBlock(fat16_root_base, fat16_buff) != 0)return 6;
            return 0;
        }

        if (in_sector_addr == 0) {
            if (evd_mmcWrBlock(addr, fat16_buff) != 0)return 7;
            in_sector_addr = 512;
            addr -= 512;
            if (evd_mmcRdBlock(addr, fat16_buff) != 0)return 8;
        }

        in_sector_addr -= 32;
        if (fat16_buff[in_sector_addr + 0x0b] == 0x0f && fat16_buff[in_sector_addr] != 0xe5) {
            fat16_buff[in_sector_addr] = 0xe5;
        } else {
            if (evd_mmcWrBlock(addr, fat16_buff) != 0)return 9;
            return 0;
        }
    }

    //if (mmcWrBlock(rec->rec_addr / 512 * 512, fat16_buff) != 0)return 6;

    return 10;
}

u8 fat16GetFatTableRecord(u16 cluster, u16 *val) {

    //u16 i;
    u8 req_sector = cluster >> 8;
    if (req_sector != fat16_current_fat_table_sector) {

        if (fat16_fat_table_buff_changed) {
            if (fat16ApplyFatTableChange() != 0)return 1;
        }

        if (evd_mmcRdBlock(fat16_fat_base + (req_sector << 9), (u8 *) fat16_fat_table_buff) != 0)return 2;
        //for(i = 0; i < 256;i++)fat16_fat_table_buff[i] = fat16_fat_cache[(req_sector << 9) + i];
        fat16_current_fat_table_sector = req_sector;
    }
    *val = fat16_fat_table_buff[cluster & 0xff];
    //*val = fat16_fat_cache[cluster];
    *val = *val >> 8 | *val << 8;
    return 0;
}

u8 fat16ApplyFatTableChange() {

    if (fat16_fat_table_buff_changed) {
        if (evd_mmcWrBlock(fat16_fat_base + (fat16_current_fat_table_sector << 9), (u8 *) fat16_fat_table_buff) != 0)return 1;
        if (evd_mmcWrBlock(fat16_fat_base + (fat16_current_fat_table_sector << 9) + (fat16_pbr.sectors_per_fat << 9), (u8 *) fat16_fat_table_buff) != 0)return 2;
        fat16_fat_table_buff_changed = 0;
    }

    return 0;
}

u8 fat16SetFatTableRecord(u16 cluster, u16 val) {

    u8 req_sector = cluster >> 8;

    if (req_sector != fat16_current_fat_table_sector) {

        if (fat16_fat_table_buff_changed) {
            if (fat16ApplyFatTableChange() != 0)return 1;
        }

        if (evd_mmcRdBlock(fat16_fat_base + (req_sector << 9), (u8 *) fat16_fat_table_buff) != 0)return 2;
        fat16_current_fat_table_sector = req_sector;
    }

    fat16_fat_table_buff[cluster & 0xff] = val >> 8 | val << 8;
    fat16_fat_table_buff_changed = 1;

    return 0;
}

u8 fat16GetNextFreeCluster(u16 *current_cluster, u8 take) {

    if (*current_cluster == 0xffff)return 1;
    u16 free_cluster = *current_cluster;
    u16 val;

    while (free_cluster < 0xffff) {
        //drawNum("cur cl: ", free_cluster, 0, 1, 22);
        free_cluster++;
        if (fat16GetFatTableRecord(free_cluster, &val) != 0)return 2;
        if (val != 0) continue;

        if (take) {
            if (*current_cluster == 0) {
                *current_cluster = free_cluster;
            }
            fat16SetFatTableRecord(*current_cluster, free_cluster);
            fat16SetFatTableRecord(free_cluster, 0xffff);
        }
        *current_cluster = free_cluster;

        return 0;
    }


    return 3;
}

u8 fat16GetFreeRecordEntry(Fat16Dir *dir, u32 *rec_addr) {


    u32 addr;
    u16 i;
    u16 u;


    if (dir->entry == 0) {

        for (addr = fat16_root_base; addr < 16384 + fat16_root_base; addr += 512) {

            if (evd_mmcRdBlock(addr, fat16_buff) != 0)return 1;
            for (i = 0; i < 512; i += 32) {
                if (fat16_buff[i] == 0 || fat16_buff[i] == 0xe5) {

                    *rec_addr = addr + i;
                    return 0;
                }
            }
        }

        return 2;
    }


    u16 cluster = dir->entry;
    u16 last_cluster;
    //VDP_drawText(APLAN, "entry1", 0, 1, cy++);
    while (cluster != 0xffff) {


        addr = fat16_data_start + (cluster - 2) * cluster_size;
        last_cluster = cluster;

        for (i = 0; i < cluster_size; i += 512) {

            if (evd_mmcRdBlock(addr, fat16_buff) != 0)return 3;
            for (u = 0; u < 512; u += 32) {

                if (fat16_buff[u] == 0 || fat16_buff[u] == 0xe5) {

                    *rec_addr = addr + u;
                    return 0;
                }

            }
            addr += 512;
        }

        if (fat16GetFatTableRecord(cluster, &cluster) != 0)return 4;
        if (cluster == 0)return 5;

    }
    //VDP_drawText(APLAN, "entry2", 0, 1, cy++);
    if (fat16GetNextFreeCluster(&last_cluster, 1) != 0)return 6;

    *rec_addr = fat16_data_start + (last_cluster - 2) * cluster_size;
    fat16ApplyFatTableChange();

    for (i = 0; i < 512; i++) {
        fat16_buff[i] = 0;
    }

    for (i = 0; i < cluster_size; i += 512) {
        if (evd_mmcWrBlock(*rec_addr + i, fat16_buff) != 0)return 3;
    }

    return 0;
}

u8 fat16CreateRecord(Fat16Record *rec, Fat16Dir *dir) {

    u16 i;
    u16 clusters_req = rec->size / cluster_size;
    if (clusters_req * cluster_size < rec->size)clusters_req += 1;
    u16 cluster = 0;
    for (i = 0; i < clusters_req; i++) {
        if (fat16GetNextFreeCluster(&cluster, 0) != 0)return 1;
    }



    if (fat16GetFreeRecordEntry(dir, &rec->rec_addr) != 0)return 2;

    rec->entry = 0;
    if (fat16GetNextFreeCluster(&rec->entry, 0) != 0)return 4;

    if (evd_mmcRdBlock(rec->rec_addr / 512 * 512, fat16_buff) != 0)return 3;




    for (i = 0; i < 32; i++)fat16_buff[rec->rec_addr % 512 + i] = 0;
    for (i = 0; i < 11; i++)fat16_buff[rec->rec_addr % 512 + i] = rec->name[i];
    fat16_buff[rec->rec_addr % 512 + 0x0b] = rec->flags;
    fat16_buff[rec->rec_addr % 512 + 31] = (u8) (rec->size >> 24);
    fat16_buff[rec->rec_addr % 512 + 30] = (u8) (rec->size >> 16);
    fat16_buff[rec->rec_addr % 512 + 29] = (u8) (rec->size >> 8);
    fat16_buff[rec->rec_addr % 512 + 28] = (u8) (rec->size >> 0);

    fat16_buff[rec->rec_addr % 512 + 27] = (u8) (rec->entry >> 8);
    fat16_buff[rec->rec_addr % 512 + 26] = (u8) (rec->entry >> 0);

    //drawNum("rec addr1: ", rec->rec_addr / 512, 0, 1, cy++);
    //drawNum("rec addr2: ", rec->rec_addr % 512, 0, 1, cy++);
    if (evd_mmcWrBlock(rec->rec_addr / 512 * 512, fat16_buff) != 0)return 5;


    cluster = 0;
    for (i = 0; i < clusters_req; i++) {
        if (fat16GetNextFreeCluster(&cluster, 1) != 0)return 6;
    }
    fat16ApplyFatTableChange();

    return 0;
}

u8 fat16WriteNextSector(Fat16File *file) {


    if (file->pos >= file->record->size)return 1;
    if (file->sector == fat16_pbr.sector_per_cluster) {

        if (fat16GetFatTableRecord(file->cluster, &file->cluster) != 0)return 2;
        file->sector = 0;
        file->addr_buff = (file->cluster - 2) * cluster_size + fat16_data_start;
    }


    if (evd_mmcWrBlock(file->addr_buff, file->sectror_buff) != 0)return 3;

    file->sector++;
    file->addr_buff += 512;

    if (file->pos <= file->record->size - 512) {
        file->pos += 512;
    } else {
        file->pos = file->record->size;
    }



    return 0;
}

u8 fat16SetNextReadSector(Fat16File *file, u32 *addr) {


    if (file->pos >= file->record->size)return 1;
    if (file->sector == fat16_pbr.sector_per_cluster) {
        //fat16GetNextFileCluster(&file->cluster);
        if (fat16GetFatTableRecord(file->cluster, &file->cluster) != 0)return 2;
        file->sector = 0;
    }

    *addr = (file->cluster - 2) * cluster_size + fat16_data_start + (file->sector << 9);

    file->sector++;
    if (file->pos <= file->record->size - 512) {
        file->pos += 512;
    } else {
        file->pos = file->record->size;
    }

    return 0;
}

#endif  /* MODULE_FAT16 */
