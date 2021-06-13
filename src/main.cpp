/**
 * @file main.cpp
 * 
 * @author
 * Angelo Elias Dalzotto (150633@upf.br)
 * GEPID - Grupo de Pesquisa em Cultura Digital (http://gepid.upf.br/)
 * Universidade de Passo Fundo (http://www.upf.br/)
 * 
 * @copyright
 * Copyright (C) 2018 by Angelo Elias Dalzotto
 * 
 * @brief This is a pure AVR port of the Arduino Sd2Card Library.
 * This file has the example for the library.
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

#include <avr/io.h>
#include "serial.h"
#include "SDCard.h"
#include "FAT.h"
#include "File.h"
#include "SPI.h"

SDCard disk(&PORTB, &DDRB, PB2); // Third parameter is the CS pin. (In this case PB2 is digital pin 10 in Arduino UNO)
FAT fs(&disk);
File root(&fs);
File file(&fs);

void handle_error()
{
    switch(disk.get_error()){
    case SDCard::Error::CMD0:
        printf("timeout error for command CMD0\n");
        break;
    case SDCard::Error::CMD8:
        printf("CMD8 was not accepted - not a valid SD card\n");
        break;
    case SDCard::Error::ACMD41:
        printf("card's ACMD41 initialization process timeout\n");
        break;
    case SDCard::Error::CMD58:
        printf("card returned an error response for CMD58 (read OCR)\n");
        break;
    case SDCard::Error::CMD24:
        printf("card returned an error response for CMD24 (write block)\n");
        break;
    default:
        printf("Unknown error. Code %x\n", (uint8_t)disk.get_error());
        break;
    }
}

int main()
{
    uart_init();
    printf("Initializing SD card...\n");
    if(disk.init()){
        printf("Card connected!\n");
    } else {
        printf("Card initialization failed.\n");
        handle_error();
    }
    SPI::set_speed(); // Max speed for now

    printf("\nMounting FAT Filesystem...\n");
    if(fs.mount()){
        printf("Filesystem mounted!\n");
    } else {
        printf("Mount error.\n");
        handle_error();
    }

    printf("\nOpening filesystem root...\n");
    if(root.open_root()){
        printf("Root is open\n");
    } else {
        printf("Unable to open root\n");
        handle_error();
    }

    printf("\nOpening file for write\n");

    if(file.open(root, "TEST.TXT", File::O_CREAT | File::O_WRITE)){
        printf("TEST.txt opened\n");
        if(file.rm()){
            printf("Ensuring file new\n");
            if(file.open(root, "TEST.TXT", File::O_CREAT | File::O_WRITE))
                printf("New test.txt opened\n");
            else
                return 0;
        } else
            return 0;
        printf("Writing to file\n");
        char buffer[127];
        sprintf(buffer, "Teste abcdefghijklmnopqrstuvwxyz %u\n", 1);
        if(file.write((const uint8_t*)buffer, strlen(buffer)) != strlen(buffer)){
            printf("Write error\n");
            handle_error();
        } else {
            file.close();
            printf("Done\n");
        }
    } else {
        printf("Unable to open file to write\n");
        handle_error();
    }
    file.close();

    printf("\nOpening for read\n");
    if(file.open(root, "TEST.TXT", File::O_RDONLY)){
        printf("TEST.txt opened\n");
        while(file.available())
            printf("%c", file.read());
    } else {
        printf("Unable to open file to write\n");
        handle_error();
    }
    file.close();
    
    return 0;
}
