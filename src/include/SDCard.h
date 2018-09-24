/**
 * @file SDCard.h
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

#ifndef _SDCARD_H_
#define _SDCARD_H_

#include <stdint.h>
#include <Millis.h>
#include <SPI.h>

class SDCard {
public:
    enum class Type {
        SDv1 = 1,
        SDv2 = 2,
        SDHC = 3
    };
    enum class Error {
        OK = 0,
        CMD0 = 0X1,      /** timeout error for command CMD0 */
        CMD8 = 0X2,      /** CMD8 was not accepted - not a valid SD card*/
        CMD17 = 0X3,     /** card returned an error response for CMD17 (read block) */
        CMD24 = 0X4,     /** card returned an error response for CMD24 (write block) */
        CMD25 = 0X05,    /**  WRITE_MULTIPLE_BLOCKS command failed */
        CMD58 = 0X06,    /** card returned an error response for CMD58 (read OCR) */
        ACMD23 = 0X07,   /** SET_WR_BLK_ERASE_COUNT failed */
        ACMD41 = 0X08,   /** card's ACMD41 initialization process timeout */
        BAD_CSD = 0X09,  /** card returned a bad CSR version field */
        ERASE = 0X0A,    /** erase block group command failed */
        ERASE_SINGLE_BLOCK = 0X0B,    /** card not capable of single block erase */
        ERASE_TIMEOUT = 0X0C,         /** Erase sequence timed out */
        READ = 0X0D,         /** card returned an error token instead of read data */
        READ_REG = 0X0E,     /** read CID or CSD failed */
        READ_TIMEOUT = 0X0F, /** timeout while waiting for start of read data */
        STOP_TRAN = 0X10,    /** card did not accept STOP_TRAN_TOKEN */
        WRITE = 0X11,        /** card returned an error token as a response to a write operation */
        WRITE_BLOCK_ZERO = 0X12,  /** attempt to write protected block zero */
        WRITE_MULTIPLE = 0X13,    /** card did not go ready for a multiple block write */
        WRITE_PROGRAMMING = 0X14, /** card returned an error to a CMD13 status check after a write */
        WRITE_TIMEOUT = 0X15,     /** timeout occurred during write programming */
        SCK_RATE = 0X16,          /** incorrect rate selected */
    };

    SDCard(volatile uint8_t *port_cs, volatile uint8_t *ddr_cs, uint8_t pin_cs);
    bool init();
    Type get_type();
    Error get_error();

    bool write_block(uint32_t block_no, const uint8_t* src);
    bool read_block(uint32_t block, uint8_t *dst);

    bool read_data(uint32_t block, uint16_t offset, uint16_t count, uint8_t *dst);


private:
    volatile uint8_t *PORT_CS;
    volatile uint8_t *DDR_CS;
    uint8_t PIN_CS;
    Error error;
    uint8_t in_block;
    uint16_t offset;
    uint8_t status;
    Type type;
    uint32_t block;
    uint8_t partial_block_read;

    void deselect();
    void select();
    uint8_t send_cmd(uint8_t cmd, uint32_t arg);
    void end_read();
    bool wait_busy(uint32_t milliseconds);
    uint8_t send_acmd(uint8_t cmd, uint32_t arg);

    bool write_data(uint8_t token, const uint8_t* src);

    bool wait_start_block();

    
    static const uint16_t SD_INIT_TIMEOUT = 2000;
    static const uint16_t SD_WRITE_TIMEOUT = 600;
    static const uint16_t SD_READ_TIMEOUT = 300;


    // SD card commands
    static const uint8_t CMD0 = 0x00;   /** GO_IDLE_STATE - init card in spi mode if CS low */
    static const uint8_t CMD8 = 0x08;   /** SEND_IF_COND - verify SD Memory Card interface operating condition.*/
    static const uint8_t CMD9 = 0X09;   /** SEND_CSD - read the Card Specific Data (CSD register) */
    static const uint8_t CMD10 = 0X0A;  /** SEND_CID - read the card identification information (CID register) */    
    static const uint8_t CMD13 = 0X0D;  /** SEND_STATUS - read the card status register */
    static const uint8_t CMD17 = 0X11;  /** READ_BLOCK - read a single data block from the card */
    static const uint8_t CMD24 = 0X18;  /** WRITE_BLOCK - write a single data block to the card */
    static const uint8_t CMD25 = 0X19;  /** WRITE_MULTIPLE_BLOCK - write blocks of data until a STOP_TRANSMISSION */
    static const uint8_t CMD32 = 0X20;  /** ERASE_WR_BLK_START - sets the address of the first block to be erased */
    static const uint8_t CMD33 = 0X21;  /** ERASE_WR_BLK_END - sets the address of the last block of the continuous range to be erased*/
    static const uint8_t CMD38 = 0X26;  /** ERASE - erase all previously selected blocks */
    static const uint8_t CMD55 = 0X37;  /** APP_CMD - escape for application specific command */
    static const uint8_t CMD58 = 0X3A;  /** READ_OCR - read the OCR register of a card */
    static const uint8_t ACMD23 = 0X17; /** SET_WR_BLK_ERASE_COUNT - Set the number of write blocks to be pre-erased before writing */
    static const uint8_t ACMD41 = 0X29; /** SD_SEND_OP_COMD - Sends host capacity support information and activates the card's initialization process */
    
    static const uint8_t R1_READY_STATE = 0X00;         /** status for card in the ready state */
    static const uint8_t R1_IDLE_STATE = 0X01;          /** status for card in the idle state */
    static const uint8_t R1_ILLEGAL_COMMAND = 0X04;     /** status bit for illegal command */
    static const uint8_t DATA_START_BLOCK = 0XFE;       /** start data token for read or write single block*/
    static const uint8_t STOP_TRAN_TOKEN = 0XFD;        /** stop token for write multiple blocks*/
    static const uint8_t WRITE_MULTIPLE_TOKEN = 0XFC;   /** start data token for write multiple blocks*/
    static const uint8_t DATA_RES_MASK = 0X1F;          /** mask for data response tokens after a write block operation */
    static const uint8_t DATA_RES_ACCEPTED = 0X05;      /** write data accepted token */

};

#endif /* _SDCARD_H_ */