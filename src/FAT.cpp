/**
 * @file FAT.cpp
 * 
 * @author
 * Angelo Elias Dalzotto (150633@upf.br)
 * GEPID - Grupo de Pesquisa em Cultura Digital (http://gepid.upf.br/)
 * Universidade de Passo Fundo (http://www.upf.br/)
 * 
 * @copyright
 * Copyright (C) 2009 by William Greiman
 * 
 * @brief This is a pure AVR port of the Arduino Sd2Card Library.
 * This file has the FAT filesystem implementation.
 * 
 * This Library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Arduino Sd2Card Library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <FAT.h>

FAT::FAT(SDCard *dev)
{
    this->dev = dev;
    cache_block_no = 0XFFFFFFFF;
    cache_dirty = false;
    cache_mirror_block = 0;
    alloc_search_start = 2;
}

bool FAT::mount()
{
    uint32_t start_block = 0;
    uint8_t part = 1; // For now only first partition

    if (!cache_raw_block(start_block, CACHE_FOR_READ))
        return false;

    part_t* p = &buffer.mbr.part[part-1];
    if ((p->boot & 0X7F) !=0  ||
        p->totalSectors < 100 ||
        p->firstSector == 0) {
        // not a valid partition
        return false;
    }
    start_block = p->firstSector;

    if (!cache_raw_block(start_block, CACHE_FOR_READ))
        return false;

    bpb_t* bpb = &buffer.fbs.bpb;
    if (bpb->bytesPerSector != 512 ||
        bpb->fatCount == 0 ||
        bpb->reservedSectorCount == 0 ||
        bpb->sectorsPerCluster == 0){

        // not valid FAT volume
        return false;
    }

    fat_count = bpb->fatCount;
    blocks_per_cluster = bpb->sectorsPerCluster;

    // determine shift that is same as multiply by blocksPerCluster_
    cluster_size_shift = 0;
    while(blocks_per_cluster != (1 << cluster_size_shift)) {
        // error if not power of 2
        if (cluster_size_shift++ > 7)
            return false;
    }
    blocks_per_fat = bpb->sectorsPerFat16 ?
                     bpb->sectorsPerFat16 : bpb->sectorsPerFat32;

    fat_start_block = start_block + bpb->reservedSectorCount;

    // count for FAT16 zero for FAT32
    root_dir_entry_cnt = bpb->rootDirEntryCount;

    // directory start for FAT16 dataStart for FAT32
    root_dir_start = fat_start_block + bpb->fatCount * blocks_per_fat;

    // data start for FAT16 and FAT32
    data_start_block = root_dir_start + ((32 * bpb->rootDirEntryCount + 511)/512);

    // total blocks for FAT16 or FAT32
    uint32_t total_blocks = bpb->totalSectors16 ?
                           bpb->totalSectors16 : bpb->totalSectors32;
    // total data blocks
    cluster_count = total_blocks - (data_start_block - start_block);

    // divide by cluster size to get cluster count
    cluster_count >>= cluster_size_shift;

    // FAT type is determined by cluster count
    if(cluster_count < 4085) {
        fat_type = Type::F12;
    } else if (cluster_count < 65525) {
        fat_type = Type::F16;
    } else {
        root_dir_start = bpb->fat32RootCluster;
        fat_type = Type::F32;
    }

    return true;
}

bool FAT::cache_raw_block(uint32_t block_no, uint8_t action)
{
    if(cache_block_no != block_no){
        if(!flush_cache())
            return false;
        if(!dev->read_block(block_no, buffer.data))
            return false;
        cache_block_no = block_no;
    }
    cache_dirty |= action;
    return true;
}

FAT::Type FAT::get_type()
{
    return fat_type;
}

uint32_t FAT::get_cluster_count()
{
    return cluster_count;
}

uint8_t FAT::get_blocks_per_cluster()
{
    return blocks_per_cluster;
}

/* Pt 1 end */

uint32_t FAT::get_root_entry_count()
{
    return root_dir_entry_cnt;
}

uint32_t FAT::get_root_start()
{
    return root_dir_start;
}

bool FAT::get_chain_size(uint32_t cluster, uint32_t *size)
{
    uint32_t sz = 0;
    do {
        if(!get_fat(cluster, &cluster))
            return false;

        sz += 512UL << cluster_size_shift;
    } while(!is_eoc(cluster));
    *size = sz;
    return true;
}

bool FAT::get_fat(uint32_t cluster, uint32_t *value)
{
    if (cluster > (cluster_count + 1))
        return false;

    uint32_t lba = fat_start_block;
    lba += fat_type == Type::F16 ? cluster >> 8 : cluster >> 7;

    if (lba != cache_block_no) {
        if (!cache_raw_block(lba, CACHE_FOR_READ))
            return false;
    }

    if (fat_type == Type::F16)
        *value = buffer.fat16[cluster & 0XFF];
    else
        *value = buffer.fat32[cluster & 0X7F] & FAT32MASK;
  
    return true;
}

bool FAT::is_eoc(uint32_t cluster)
{
    return cluster >= (fat_type == Type::F16 ? FAT16EOC_MIN : FAT32EOC_MIN);
}


///////
 
uint8_t FAT::get_block(uint32_t position)
{
    return (position >> 9) & (blocks_per_cluster - 1);
}

uint32_t FAT::get_start_block(uint32_t cluster)
{
    return data_start_block + ((cluster - 2) << cluster_size_shift);
}

uint32_t FAT::get_cache_block_no()
{
    return cache_block_no;
}

bool FAT::read_data(uint32_t block, uint16_t offset, uint16_t count, uint8_t *buffer)
{
    return dev->read_data(block, offset, count, buffer);
}

uint8_t* FAT::get_buffer_data_ptr()
{
    return buffer.data;
}

dir_t* FAT::get_buffer_dir_ptr()
{
    return buffer.dir;
}

bool FAT::flush_cache()
{
    if(cache_dirty){
        if (!dev->write_block(cache_block_no, buffer.data)) 
            return false;

        // mirror FAT tables
        if (cache_mirror_block) {
            if (!dev->write_block(cache_mirror_block, buffer.data)) 
                return false;
            
            cache_mirror_block = 0;
        }
        cache_dirty = false;
    }
    return true;
}

uint8_t FAT::get_cluster_size_shift()
{
    return cluster_size_shift;
}

bool FAT::free_chain(uint32_t cluster)
{
    alloc_search_start = 2;

    do {
        uint32_t next;
        if(!get_fat(cluster, &next))
            return false;

        if(!put_fat(cluster, 0))
            return false;

        cluster = next;
    } while(!is_eoc(cluster));

    return true;
}

bool FAT::put_fat(uint32_t cluster, uint32_t value)
{
    // error if reserved cluster
    if (cluster < 2)
        return false;

    // error if not in FAT
    if (cluster > (cluster_count + 1))
        return false;

    // calculate block address for entry
    uint32_t lba = fat_start_block;
    lba += fat_type == Type::F16 ? cluster >> 8 : cluster >> 7;

    if (lba != cache_block_no) {
        if (!cache_raw_block(lba, CACHE_FOR_READ))
            return false;
    }
    // store entry
    if (fat_type == Type::F16) {
        buffer.fat16[cluster & 0XFF] = value;
    } else {
        buffer.fat32[cluster & 0X7F] = value;
    }
    set_cache_dirty();

    // mirror second FAT
    if (fat_count > 1) cache_mirror_block = lba + blocks_per_fat;
    return true;
}

void FAT::set_cache_dirty()
{
    cache_dirty |= CACHE_FOR_WRITE;
}

bool FAT::put_eoc(uint32_t cluster)
{
    return put_fat(cluster, 0x0FFFFFFF);
}

bool FAT::alloc_contiguous(uint32_t count, uint32_t *current_cluster)
{
    // start of group
    uint32_t bgnCluster;

    // flag to save place to start next search
    uint8_t setStart;

    // set search start cluster
    if (*current_cluster) {
        // try to make file contiguous
        bgnCluster = *current_cluster + 1;

        // don't save new start location
        setStart = false;
    } else {
        // start at likely place for free cluster
        bgnCluster = alloc_search_start;

        // save next search start if one cluster
        setStart = 1 == count;
    }
    // end of group
    uint32_t endCluster = bgnCluster;

    // last cluster of FAT
    uint32_t fatEnd = cluster_count + 1;

    // search the FAT for free clusters
    for (uint32_t n = 0;; n++, endCluster++) {
        // can't find space checked all clusters
        if (n >= cluster_count)
            return false;

        // past end - start from beginning of FAT
        if (endCluster > fatEnd) {
            bgnCluster = endCluster = 2;
        }
        uint32_t f;
        if (!get_fat(endCluster, &f))
            return false;

        if (f != 0) {
            // cluster in use try next cluster as bgnCluster
            bgnCluster = endCluster + 1;
        } else if ((endCluster - bgnCluster + 1) == count) {
            // done - found space
            break;
        }
    }
    // mark end of chain
    if (!put_eoc(endCluster))
        return false;

    // link clusters
    while (endCluster > bgnCluster) {
        if (!put_fat(endCluster - 1, endCluster))
            return false;
        endCluster--;
    }
    if (*current_cluster != 0) {
        // connect chains
        if (!put_fat(*current_cluster, bgnCluster))
            return false;
    }
    // return first cluster number to caller
    *current_cluster = bgnCluster;

    // remember possible next free cluster
    if (setStart) alloc_search_start = bgnCluster + 1;

    return true;
}

bool FAT::cache_zero_block(uint32_t block_no)
{
    if (!flush_cache())
        return false;

    // loop take less flash than memset(cacheBuffer_.data, 0, 512);
    for (uint16_t i = 0; i < 512; i++) {
        buffer.data[i] = 0;
    }
    cache_block_no = block_no;
    set_cache_dirty();
    return true;
}

void FAT::set_cache_block_no(uint32_t block_no)
{
    cache_block_no = block_no;
}

bool FAT::write_block(uint32_t block, const uint8_t *dst)
{
    return dev->write_block(block, dst);
}