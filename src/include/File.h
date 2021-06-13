/**
 * @file File.h
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
 * This file has the file stream implementation.
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

#ifndef _FILE_H_
#define _FILE_H_

#include <stdint.h>
#include <string.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <ctype.h>
#include "FAT.h"

class File {
public:

    enum class Type {
        // values for type_
        CLOSED = 0, /** This SdFile has not been opened. */
        NORMAL = 1, /** SdFile for a file */
        ROOT16 = 2, /** SdFile for a FAT16 root directory */
        ROOT32 = 3, /** SdFile for a FAT32 root directory */
        SUBDIR = 4, /** SdFile for a subdirectory */
        MIN_DIR = ROOT16 /** Test value for directory type */
    };
    enum Flags {
        // flags for timestamp
        LS_DATE = 1,    /** ls() flag to print modify date */
        LS_SIZE = 2,  /** ls() flag to print file size */
        LS_R = 4, /** ls() flag for recursive list of subdirectories */

        // use the gnu style oflag in open()
        O_READ = 0X01, /** open() oflag for reading */
        O_RDONLY = O_READ, /** open() oflag - same as O_READ */
        O_WRITE = 0X02,   /** open() oflag for write */
        O_WRONLY = O_WRITE,   /** open() oflag - same as O_WRITE */
        O_RDWR = (O_READ | O_WRITE),  /** open() oflag for reading and writing */
        O_ACCMODE = (O_READ | O_WRITE),   /** open() oflag mask for access modes */
        O_APPEND = 0X04,  /** The file offset shall be set to the end of the file prior to each write. */
        O_SYNC = 0X08,    /** synchronous writes - call sync() after each write */
        O_CREAT = 0X10,   /** create the file if nonexistent */
        O_EXCL = 0X20,    /** If O_CREAT and O_EXCL are set, open() shall fail if the file exists */
        O_TRUNC = 0X40,   /** truncate the file to zero length */

        // flags for timestamp
        T_ACCESS = 1, /** set the file's last access date */
        T_CREATE = 2, /** set the file's creation date and time */
        T_WRITE = 4,  /** Set the file's write date and time */

        // bits defined in flags_    
        F_OFLAG = (O_ACCMODE | O_APPEND | O_SYNC), // should be 0XF
        F_UNUSED = 0X30, // available bits
        F_FILE_UNBUFFERED_READ = 0X40,   // use unbuffered SD read
        F_FILE_DIR_DIRTY = 0X80 // sync of directory entry required
    };
    enum Options {
        LS_FILE = 0x01,
        LS_FOLDER = 0x02
    };

    File(FAT *fs);
    File(File f, const char *name);
    bool open_root();
    bool ls(char *buffer, uint8_t options);
    bool ls(char *buffer, uint8_t options, uint8_t index);
    bool is_dir();

    int16_t read(uint8_t *buffer, uint16_t size);
    int16_t read();

    bool is_open();
    bool is_file();

    bool open(File &dir, const char *filename, uint8_t oflag);
    bool close();
    bool sync();
    static bool make83name(const char *str, uint8_t *name);

    uint32_t get_current_position();
    uint32_t get_file_size();
    Type get_type();
    bool add_dir_cluster();
    uint32_t available();
    void rewind();

    int print(const char* format, ...);
    size_t write(const uint8_t *buffer, uint16_t size);

    bool rm();

private:
    FAT *fs;

    Type type;

    uint32_t first_cluster;
    uint32_t file_size;
    uint8_t flags;

    uint32_t current_cluster;
    uint32_t current_position;
    uint32_t dir_block;
    uint8_t dir_index;
 
    dir_t* read_dir_cache();
    uint8_t is_unbuffered_read();

    bool fill_name(dir_t* p, char* buffer, uint8_t options);    

    dir_t* cache_dir_entry(uint8_t action);
    bool open_cached_entry(uint8_t dir_index, uint8_t oflag);
    bool truncate(uint32_t length);
    bool seek_set(uint32_t pos);
    bool add_cluster();
    bool seek_end();

    /** Default date for file timestamps is 1 Jan 2000 */
    static uint16_t const FAT_DEFAULT_DATE = ((2000 - 1980) << 9) | (1 << 5) | 1;
    /** Default time for file timestamp is 1 am */
    static uint16_t const FAT_DEFAULT_TIME = (1 << 11);

};

#endif /* _FILE_H_ */
