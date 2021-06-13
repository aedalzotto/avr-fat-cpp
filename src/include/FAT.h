/**
 * @file FAT.h
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

#ifndef _FAT_H_
#define _FAT_H_

#include "SDCard.h" // For now the only option
#include "FatStructs.h"

union cache_t {
           /** Used to access cached file data blocks. */
  uint8_t  data[512];
           /** Used to access cached FAT16 entries. */
  uint16_t fat16[256];
           /** Used to access cached FAT32 entries. */
  uint32_t fat32[128];
           /** Used to access cached directory entries. */
  dir_t    dir[16];
           /** Used to access a cached MasterBoot Record. */
  mbr_t    mbr;
           /** Used to access to a cached FAT boot sector. */
  fbs_t    fbs;
};

class FAT {
public:
    enum class Type {
        F12 = 12,
        F16 = 16,
        F32 = 32
    };

    FAT(SDCard *dev);
    bool mount();
    Type get_type();
    uint32_t get_cluster_count();
    uint8_t get_blocks_per_cluster();
    bool get_fat(uint32_t cluster, uint32_t *value);

    bool cache_raw_block(uint32_t block_no, uint8_t action);
    uint32_t get_root_entry_count();
    uint32_t get_root_start();
    bool get_chain_size(uint32_t cluster, uint32_t *size);
    uint8_t get_block(uint32_t position);
    uint32_t get_start_block(uint32_t cluster);
    uint32_t get_cache_block_no();

    bool read_data(uint32_t block, uint16_t offset, uint16_t count, uint8_t *buffer);
    uint8_t* get_buffer_data_ptr();
    dir_t* get_buffer_dir_ptr();

    bool flush_cache();
    bool is_eoc(uint32_t cluster);
    uint8_t get_cluster_size_shift();
    bool free_chain(uint32_t cluster);
    bool put_eoc(uint32_t cluster);
    bool alloc_contiguous(uint32_t count, uint32_t *current_cluster);
    bool cache_zero_block(uint32_t block_no);
    void set_cache_block_no(uint32_t block_no);

    bool write_block(uint32_t block, const uint8_t *dst);
    void set_cache_dirty();


    static uint8_t const CACHE_FOR_READ = 0;   // value for action argument in cacheRawBlock to indicate read from cache
    static uint8_t const CACHE_FOR_WRITE = 1;   // value for action argument in cacheRawBlock to indicate cache dirty


private:
    SDCard *dev;

    uint32_t cache_block_no;
    bool cache_dirty;
    cache_t buffer;
    uint32_t cache_mirror_block;

    uint8_t fat_count;
    uint8_t blocks_per_cluster;
    uint8_t cluster_size_shift;
    uint32_t blocks_per_fat;
    uint32_t fat_start_block;
    uint16_t root_dir_entry_cnt;
    uint32_t root_dir_start;
    uint32_t data_start_block;
    uint32_t cluster_count;
    Type fat_type;
    uint32_t alloc_search_start;

    bool put_fat(uint32_t cluster, uint32_t value);


};

#endif /* _FAT_H_ */
