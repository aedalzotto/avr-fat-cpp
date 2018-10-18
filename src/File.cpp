/**
 * @file File.cpp
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

#include <File.h>

File::File(FAT *fs) : fs(fs)
{
    type = Type::CLOSED;
}

bool File::open_root()
{
    if(is_open())
        return false;

    if(fs->get_type() == FAT::Type::F16){
        type = Type::ROOT16;
        first_cluster = 0;
        file_size = 32 * fs->get_root_entry_count();
    } else if(fs->get_type() == FAT::Type::F32){
        type = Type::ROOT32;
        first_cluster = fs->get_root_start();
        if(!fs->get_chain_size(first_cluster, &file_size))
            return false;
    } else {
        // volume is not initialized or FAT12
        return false;
    }

    // read only
    flags = Flags::O_READ;

    // set to start of file
    current_cluster = 0;
    current_position = 0;

    // root has no directory entry
    dir_block = 0;
    dir_index = 0;
    return true;
}

bool File::ls(char *buffer, uint8_t options)
{
    dir_t* p = nullptr;
    buffer[0]=0;

    if(!(options & (LS_FILE | LS_FOLDER)))
        return false;

    while((p = read_dir_cache())){
        // done if past last used entry
        if(p->name[0] == DIR_NAME_FREE)
            return false; // Or true?
        
        if (p->name[0] == DIR_NAME_DELETED || p->name[0] == '.' || p->name[0] == 0x80)
            continue;
        
        if(DIR_IS_SUBDIR(p) && !(options & LS_FOLDER))
            continue;
        
        if(DIR_IS_FILE(p) && !(options & LS_FILE))
            continue;
        
        break;
    }

    return p ? fill_name(p, buffer, options) : false;
}

bool File::ls(char *buffer, uint8_t options, uint8_t index)
{
    dir_t* p = nullptr;
    buffer[0]=0;

    if(!(options & (LS_FILE | LS_FOLDER)))
        return false;

    rewind();
    for(int8_t i = 0; i <= index; i++){
        p = read_dir_cache();
        if(!p)
            return false;

        // done if past last used entry
        if(p->name[0] == DIR_NAME_FREE)
            return false; // Or true?
        
        if (p->name[0] == DIR_NAME_DELETED || p->name[0] == '.' || p->name[0] == 0x80){
            i--;
            continue;
        }
        if(DIR_IS_SUBDIR(p) && !(options & LS_FOLDER)){
            i--;
            continue;
        }
        if(DIR_IS_FILE(p) && !(options & LS_FILE)){
            i--;
            continue;
        }
    }

    return fill_name(p, buffer, options);
}

bool File::fill_name(dir_t* p, char* buffer, uint8_t options)
{    
    uint8_t w = 0;
    for(uint8_t i = 0; i < 11; i++){
        if(p->name[i] == ' ')
            continue;

        if(i == 8)
            buffer[w++] = '.';
        buffer[w++] = p->name[i];
    }
    if(DIR_IS_SUBDIR(p))
        buffer[w++] = '/';
    buffer[w] = 0;
    return true;
}


void File::rewind()
{
    current_position = current_cluster = 0;
}

dir_t* File::read_dir_cache()
{
    // error if not directory
    if(!is_dir())
        return nullptr;

    // index of entry in cache
    uint8_t i = (current_position >> 5) & 0XF;

    // use read to locate and cache block
    if(read() < 0)
        return nullptr;

    // advance to next entry
    current_position += 31;

    // return pointer to entry
    return (fs->get_buffer_dir_ptr() + i);
}

bool File::is_dir()
{
    return (uint8_t)type >= (uint8_t)File::Type::MIN_DIR;
}

int16_t File::read()
{
    uint8_t b;
    return read(&b, 1) == 1 ? b : -1;
}

int16_t File::read(uint8_t *buffer, uint16_t size)
{
    // error if not open or write only
    if(!is_open() || !(flags & O_READ))
        return -1;

    // max bytes left in file
    if(size > (file_size - current_position)) size = file_size - current_position;

    // amount left to read
    uint16_t toRead = size;
    while (toRead > 0) {
        uint32_t block;  // raw device block number
        uint16_t offset = current_position & 0X1FF;  // offset in block
        if (type == Type::ROOT16) {
            block = fs->get_root_start() + (current_position >> 9);
        } else {
            uint8_t blockOfCluster = fs->get_block(current_position);
            if (offset == 0 && blockOfCluster == 0) {
                // start of new cluster
                if (current_position == 0) {
                    // use first cluster in file
                    current_cluster = first_cluster;
                } else {
                    // get next cluster from FAT
                    if (!fs->get_fat(current_cluster, &current_cluster))
                        return -1;
                }
            }
            block = fs->get_start_block(current_cluster) + blockOfCluster;
        }
        uint16_t n = toRead;

        // amount to be read from current block
        if (n > (512 - offset)) n = 512 - offset;

        // no buffering needed if n == 512 or user requests no buffering
        if ((is_unbuffered_read() || n == 512) &&
            block != fs->get_cache_block_no()) {
            if (!fs->read_data(block, offset, n, buffer))
                return -1;
            buffer += n;
        } else {
            // read block to cache and copy data to caller
            if (!fs->cache_raw_block(block, FAT::CACHE_FOR_READ))
                return -1;
            
            uint8_t* src = fs->get_buffer_data_ptr() + offset;
            uint8_t* end = src + n;
            while (src != end) *buffer++ = *src++;
        }
        current_position += n;
        toRead -= n;
    }
    return size;
}

uint8_t File::is_unbuffered_read()
{
    return flags & Flags::F_FILE_UNBUFFERED_READ;
}

bool File::is_open()
{
    return ((uint8_t)type != (uint8_t)Type::CLOSED);
}

bool File::close()
{
    if(!sync())
        return false;
    type = Type::CLOSED;
    return true;
}

bool File::sync()
{
    if(!is_open())
        return false;

    if(flags & Flags::F_FILE_DIR_DIRTY){
        dir_t* d = cache_dir_entry(FAT::CACHE_FOR_WRITE);
        if (!d)
            return false;

        // do not set filesize for dir files
        if (!is_dir()) d->fileSize = file_size;

        // update first cluster fields
        d->firstClusterLow = first_cluster & 0XFFFF;
        d->firstClusterHigh = first_cluster >> 16;

        // clear directory dirty
        flags &= ~F_FILE_DIR_DIRTY;
    }
    return fs->flush_cache();
}

dir_t* File::cache_dir_entry(uint8_t action)
{
    if(!fs->cache_raw_block(dir_block, action))
        return nullptr;
    return fs->get_buffer_dir_ptr() + dir_index;
}

bool File::open(File &dir, const char *filename, uint8_t oflag)
{
    uint8_t dname[11];
    dir_t* p;

    // error if already open
    if (is_open())
        return false;

    if (!make83name(filename, dname)) 
        return false;
    
    dir.rewind();

    // bool for empty entry found
    bool emptyFound = false;

    // search for file
    while (dir.get_current_position() < dir.get_file_size()) {
        uint8_t index = 0XF & (dir.get_current_position() >> 5);
        p = dir.read_dir_cache();
        if (!p)
            return false;

        if (p->name[0] == DIR_NAME_FREE || p->name[0] == DIR_NAME_DELETED) {
            // remember first empty slot
            if (!emptyFound) {
                emptyFound = true;
                dir_index = index;
                dir_block = fs->get_cache_block_no();
            }
            // done if no entries follow
            if (p->name[0] == DIR_NAME_FREE)
                break;
        } else if (!memcmp(dname, p->name, 11)) {
            // don't open existing file if O_CREAT and O_EXCL
            if ((oflag & (O_CREAT | O_EXCL)) == (O_CREAT | O_EXCL))
                return false;

            // open found file
            return open_cached_entry(0XF & index, oflag);
        }
    }

    // only create file if O_CREAT and O_WRITE
    if ((oflag & (O_CREAT | O_WRITE)) != (O_CREAT | O_WRITE))
        return false;

    // cache found slot or add cluster if end of file
    if (emptyFound) {
        p = cache_dir_entry(FAT::CACHE_FOR_WRITE);

        if (!p)
            return false;
    } else {
        if (dir.get_type() == Type::ROOT16)
            return false;

        // add and zero cluster for dirFile - first cluster is in cache for write
        if (!dir.add_dir_cluster())
            return false;

        // use first entry in cluster
        dir_index = 0;
        p = fs->get_buffer_dir_ptr();
    }

    // initialize as empty file
    memset(p, 0, sizeof(dir_t));
    memcpy(p->name, dname, 11);

    // set timestamps
    // use default date/time
    p->creationDate = FAT_DEFAULT_DATE;
    p->creationTime = FAT_DEFAULT_TIME;
    
    p->lastAccessDate = p->creationDate;
    p->lastWriteDate = p->creationDate;
    p->lastWriteTime = p->creationTime;

    // force write of entry to SD
    if (!fs->flush_cache())
        return false;

    // open entry in cache
    return open_cached_entry(dir_index, oflag);
}

bool File::add_dir_cluster()
{
    if(!add_cluster())
        return false;

    // zero data in cluster insure first cluster is in cache
    uint32_t block = fs->get_start_block(current_cluster);
    for (uint8_t i = fs->get_blocks_per_cluster(); i != 0; i--) {
        if (!fs->cache_zero_block(block + i - 1))
            return false;
    }
    // Increase directory file size by cluster size
    file_size += 512UL << fs->get_cluster_size_shift();
    return true;
}

bool File::add_cluster()
{
    if(!fs->alloc_contiguous(1, &current_cluster))
        return false;

    // if first cluster of file link to directory entry
    if (first_cluster == 0) {
        first_cluster = current_cluster;
        flags |= F_FILE_DIR_DIRTY;
    }
    return true;
}

bool File::make83name(const char *str, uint8_t *name)
{
    uint8_t c;
    uint8_t n = 7;  // max index for part before dot
    uint8_t i = 0;
    // blank fill name and extension
    while (i < 11) name[i++] = ' ';
    i = 0;
    while ((c = *str++) != '\0') {
        if (c == '.') {
            if (n == 10)
                return false;  // only one dot allowed
            n = 10;  // max index for full 8.3 name
            i = 8;   // place for extension
        } else {
            // illegal FAT characters
            uint8_t b;
            PGM_P p = PSTR("|<>^+=?/[];,*\"\\");
            while ((b = pgm_read_byte(p++)))
                if (b == c)
                    return false;
            // check size and only allow ASCII printable characters
            if (i > n || c < 0X21 || c > 0X7E)
                return false;

            // only upper case allowed in 8.3 names - convert lower to upper
            name[i++] = c < 'a' || c > 'z' ?  c : c + ('A' - 'a');
        }
    }
    // must have a file name, extension is optional
    return name[0] != ' ';
}

uint32_t File::get_current_position()
{
    return current_position;
}

uint32_t File::get_file_size()
{
    return file_size;
}


bool File::open_cached_entry(uint8_t dir_index, uint8_t oflag)
{
    // location of entry in cache
    dir_t* p = fs->get_buffer_dir_ptr() + dir_index;

    // write or truncate is an error for a directory or read-only file
    if (p->attributes & (DIR_ATT_READ_ONLY | DIR_ATT_DIRECTORY))
        if (oflag & (O_WRITE | O_TRUNC))
            return false;
  
    // remember location of directory entry on SD
    this->dir_index = dir_index;
    dir_block = fs->get_cache_block_no();

    // copy first cluster number for directory fields
    first_cluster = (uint32_t)p->firstClusterHigh << 16;
    first_cluster |= p->firstClusterLow;

    // make sure it is a normal file or subdirectory
    if (DIR_IS_FILE(p)) {
        file_size = p->fileSize;
        type = Type::NORMAL;
    } else if (DIR_IS_SUBDIR(p)) {
        if (!fs->get_chain_size(first_cluster, &file_size))
            return false;
        type = Type::SUBDIR;
    } else {
        return false;
    }
    // save open flags for read/write
    flags = oflag & (O_ACCMODE | O_SYNC | O_APPEND);

    // set to start of file
    current_cluster = 0;
    current_position = 0;

    // truncate file to zero length if requested
    if (oflag & O_TRUNC)
        return truncate(0);

    return true;
}


/////////

bool File::truncate(uint32_t length)
{
    // error if not a normal file or read-only
    if (!is_file() || !(flags & O_WRITE))
        return false;

    // error if length is greater than current size
    if (length > file_size)
        return false;

    // fileSize and length are zero - nothing to do
    if (file_size == 0)
        return true;

    // remember position for seek after truncation
    uint32_t newPos = current_position > length ? length : current_position;

    // position to last cluster in truncated file
    if (!seek_set(length))
        return false;

    if (!length) {
        // free all clusters
        if (!fs->free_chain(first_cluster))
            return false;
        first_cluster = 0;
    } else {
        uint32_t toFree;
        if (!fs->get_fat(current_cluster, &toFree))
            return false;

        if (!fs->is_eoc(toFree)) {
            // free extra clusters
            if (!fs->free_chain(toFree))
                return false;

        // current cluster is end of chain
        if (!fs->put_eoc(current_cluster))
            return false;
        }
    }
    file_size = length;

    // need to update directory entry
    flags |= F_FILE_DIR_DIRTY;

    if (!sync())
        return false;

    // set file to correct position
    return seek_set(newPos);
}

bool File::is_file()
{
    return type == Type::NORMAL;
}

bool File::seek_set(uint32_t pos)
{
    // error if file not open or seek past end of file
    if (!is_open() || pos > file_size)
        return false;

    if (type == Type::ROOT16) {
        current_position = pos;
        return true;
    }
    if (pos == 0) {
        // set position to start of file
        current_cluster = 0;
        current_position = 0;
        return true;
    }
    // calculate cluster index for cur and new position
    uint32_t nCur = (current_position - 1) >> (fs->get_cluster_size_shift() + 9);
    uint32_t nNew = (pos - 1) >> (fs->get_cluster_size_shift() + 9);

    if (nNew < nCur || current_position == 0) {
        // must follow chain from first cluster
        current_cluster = first_cluster;
    } else {
        // advance from curPosition
        nNew -= nCur;
    }
    while (nNew--) {
        if (!fs->get_fat(current_cluster, &current_cluster))
            return false;
    }
    current_position = pos;
    return true;
}

File::Type File::get_type()
{
    return type;
}

size_t File::write(const uint8_t *buffer, uint16_t size)
{
    const uint8_t *src = buffer;
    uint16_t written;

    if(!is_file() || !(flags & O_WRITE))
        return 0;
    
    if((flags & O_APPEND) && current_position != file_size){
        if(!seek_end())
            return 0;
    }

    for(written = 0; written < size; written++){
        uint8_t boc = fs->get_block(current_position);
        uint16_t w_offset = current_position & 0x1FF;

        if(!boc && !w_offset){
            // Start of a new cluster
            if(!current_cluster){
                if(!first_cluster){
                    // allocate first cluster of file
                    if(!add_cluster())
                        return written;
                } else {
                    current_cluster = first_cluster;
                }
            } else {
                uint32_t next;
                if(!fs->get_fat(current_cluster, &next))
                    return written;
                
                if(fs->is_eoc(next)){
                    // add cluster if at end of chain
                    if(!add_cluster())
                        return written;
                } else {
                    current_cluster = next;
                }
            }
        }

        // max space in block
        uint16_t n = 512 - w_offset;

        // lesser of space and amount to write
        if(n > (size - written)) n = (size - written);

        // block for data write
        uint32_t block = fs->get_start_block(current_cluster) + boc;
        if(n == 512){
            // full block - don't need to use cache
            // invalidade cache if block is in cache
            if(fs->get_cache_block_no() == block)
                fs->set_cache_block_no(0XFFFFFFFF);
            
            if(!fs->write_block(block, src))
                return written;

            src += 512;
        } else {
            if(!w_offset && current_position >= file_size){
                // start of new block don't need to read into cache
                if(!fs->flush_cache())
                    return written;

                fs->set_cache_block_no(block);
                fs->set_cache_dirty();
            } else {
                // rewrite part of block
                if(!fs->cache_raw_block(block, FAT::CACHE_FOR_WRITE))
                    return written;
            }
            uint8_t *dst = fs->get_buffer_data_ptr() + w_offset;
            uint8_t *end = dst + n;
            while(dst != end) *dst++ = *src++;
        }
        written += n;
        current_position += n;
    }

    if(current_position > file_size){
        // update file_size and insure sync will update dir entry
        file_size = current_position;
        flags |= Flags::F_FILE_DIR_DIRTY;
    }

    if(flags & O_SYNC){
        if(!sync())
            return 0;
    }
    return written-1;
}

bool File::seek_end()
{
    return seek_set(file_size);
}


uint32_t File::available()
{
    if(!is_open())
        return 0;

    uint32_t n = file_size - current_position;

    return n > 0x7FFF ? 0x7FFF : n;
}

bool File::rm()
{
    // free any clusters - will fail if read-only or directory
    if(!truncate(0))
        return false;

    // cache directory entry
    dir_t* d = cache_dir_entry(FAT::CACHE_FOR_WRITE);
    if(!d)
        return false;

    // mark entry deleted
    d->name[0] = DIR_NAME_DELETED;

    // set this SdFile closed
    type = Type::CLOSED;

    // write entry to SD
    return fs->flush_cache();
}