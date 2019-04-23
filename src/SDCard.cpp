/**
 * @file SDCard.cpp
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
 * This file has the low-level disk I/O with SD Cards connected to SPI.
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

#include <SDCard.h>

SDCard::SDCard(volatile uint8_t *PORT_CS, volatile uint8_t *DDR_CS, uint8_t PIN_CS)
{
    error = Error::OK;
    in_block = 0;
    offset = 0;
    status = 0;
    block = 0;
    partial_block_read = 0;

    this->PORT_CS = PORT_CS;
    this->DDR_CS = DDR_CS;
    this->PIN_CS = PIN_CS;

    *DDR_CS |= (1 << PIN_CS);
    deselect();
}

bool SDCard::init()
{
    Millis::init();
    error = Error::OK;
    in_block = partial_block_read = 0;

    uint32_t then = Millis::get();
    
    deselect();

#ifdef __AVR_ATmega2560__
    SPI::init(&DDRB, &DDRB, &DDRB, &DDRB, PB0, PB1, PB2, PB3, &PORTB);
#elif defined(__AVR_ATmega328P__)
    SPI::init(&DDRB, &DDRB, &DDRB, &DDRB, PB2, PB5, PB3, PB4, &PORTB);
#else
    #error "Not compatible. Please add this AVR to the SDCard.cpp init function"
#endif

    // 80 Dummy SPI clocks
    for(int i = 0; i < 10; i++)
        SPI::write(0xFF);
    
    // Selects chip
    select();

    // Put SD Card in idle mode
    while ((status = send_cmd(CMD0, 0)) != R1_IDLE_STATE) {
        uint16_t diff = Millis::get() - then;
        if (diff > SD_INIT_TIMEOUT) {
            error = Error::CMD0;
            deselect();
            return false;
        }
    }

    // Check SD type
    if(send_cmd(CMD8, 0x1AA) & R1_ILLEGAL_COMMAND){
        type = Type::SDv1;
    } else {
        // only need last byte of r7 response
        for (uint8_t i = 0; i < 4; i++)
            status = SPI::read();
    
        if (status != 0XAA) {
            error = Error::CMD8;
            deselect();
            return false;
        }

        type = Type::SDv2;
    }

    // initialize card and send host supports SDHC if SD2
    uint32_t arg = type == Type::SDv2 ? 0X40000000 : 0;

    while ((status = send_acmd(ACMD41, arg)) != R1_READY_STATE) {
        // Check for timeout
        uint16_t diff = Millis::get() - then;
        if (diff > SD_INIT_TIMEOUT) {
            error = Error::ACMD41;
            deselect();
            return false;
        }
    }

    // if SD2 read OCR register to check for SDHC card
    if (type == Type::SDv2) {
        if (send_cmd(CMD58, 0)) {
            error = Error::CMD58;
            deselect();
            return false;
        }
        if ((SPI::read() & 0XC0) == 0XC0) type = Type::SDHC;
        // discard rest of ocr - contains allowed voltage range
        for (uint8_t i = 0; i < 3; i++) SPI::read();
    }

    deselect();
    return true;
}

void SDCard::deselect()
{
    *PORT_CS |= (1 << PIN_CS);
}

void SDCard::select()
{
    *PORT_CS &= ~(1 << PIN_CS);
}

uint8_t SDCard::send_cmd(uint8_t cmd, uint32_t arg)
{
    end_read();

    select();

    wait_busy(300);

    SPI::write(cmd | 0x40);

    for(int8_t s = 24; s >= 0; s -= 8)
        SPI::write(arg >> s);

    uint8_t crc = 0xFF;
    if(cmd == CMD0) crc = 0x95;
    else if(cmd == CMD8) crc = 0x87;
    SPI::write(crc);

    for (uint8_t i = 0; ((status = SPI::read()) & 0X80) && i != 0XFF; i++);
    return status;
}

void SDCard::end_read()
{
    if(in_block){
        while(offset++ < 514) SPI::read();
        deselect();
        in_block = 0;
    }
}

bool SDCard::wait_busy(uint32_t milliseconds)
{
    uint32_t then = Millis::get();
    do {
        if(SPI::read() == 0xFF)
            return true;
    } while(Millis::get() - then < milliseconds);
    return false;
}

uint8_t SDCard::send_acmd(uint8_t cmd, uint32_t arg)
{
    send_cmd(CMD55, 0);
    return send_cmd(cmd, arg);
}

SDCard::Type SDCard::get_type()
{
    return type;
}

SDCard::Error SDCard::get_error()
{
    return error;
}

bool SDCard::write_block(uint32_t block_no, const uint8_t* src)
{
    // don't allow write to first block
    if (!block_no) {
        error = Error::WRITE_BLOCK_ZERO;
        deselect();
        return false;
    }

    // use address if not SDHC card
    if(type != Type::SDHC) block_no <<= 9;

    if(send_cmd(CMD24, block_no)){
        error = Error::CMD24;
        deselect();
        return false;
    }

    if(!write_data(DATA_START_BLOCK, src)){
        deselect();
        return false;
    }

    // wait for flash programming to complete
    if(!wait_busy(SD_WRITE_TIMEOUT)) {
        error = Error::WRITE_TIMEOUT;
        deselect();
        return false;
    }

    // response is r2 so get and check two bytes for nonzero
    if(send_cmd(CMD13, 0) || SPI::read()) {
        error = Error::WRITE_PROGRAMMING;
        deselect();
        return false;
    }
    
    deselect();
    return true;
}

bool SDCard::write_data(uint8_t token, const uint8_t* src)
{
    SPI::write(token);
    for (uint16_t i = 0; i < 512; i++) {
        SPI::write(src[i]);
    }

    SPI::write(0xff);  // dummy crc
    SPI::write(0xff);  // dummy crc

    status = SPI::read();
    if((status & DATA_RES_MASK) != DATA_RES_ACCEPTED){
        error = Error::WRITE;
        deselect();
        return false;
    }
    return true;
}

bool SDCard::read_block(uint32_t block, uint8_t *dst)
{
    return read_data(block, 0, 512, dst);
}

bool SDCard::read_data(uint32_t block, uint16_t offset, uint16_t count, uint8_t *dst)
{
    if(!count)
        return true;

    if((count + offset) > 512){
        deselect();
        return false;
    }

    if(!in_block || block != this->block || offset < this->offset){
        this->block = block;
            // use address if not SDHC card
        if(type != Type::SDHC) block <<= 9;
        if(send_cmd(CMD17, block)){
            error = Error::CMD17;
            deselect();
            return false;
        }
        if(!wait_start_block()) {
            deselect();
            return false;
        }
        this->offset = 0;
        in_block = 1;
    }

    // skip data before offset
    for (;this->offset < offset; this->offset++) {
        SPI::read();
    }
    // transfer data
    for (uint16_t i = 0; i < count; i++) {
        dst[i] = SPI::read();
    }

    this->offset += count;
    if (!partial_block_read || this->offset >= 512) {
        // read rest of data, checksum and set chip select high
        end_read();
    }
    return true;
}

bool SDCard::wait_start_block()
{
    uint32_t then = Millis::get();
    while((status = SPI::read()) == 0XFF){
        uint16_t diff = Millis::get() - then;
        if(diff > SD_READ_TIMEOUT){
            error = Error::READ_TIMEOUT;
            deselect();
            return false;
        }
    }
    if (status != DATA_START_BLOCK) {
        error = Error::READ;
        deselect();
        return false;
    }
    return true;
}


