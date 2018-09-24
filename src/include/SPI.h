/**
 * @file SPI.h
 * 
 * @author
 * Angelo Elias Dalzotto (150633@upf.br)
 * GEPID - Grupo de Pesquisa em Cultura Digital (http://gepid.upf.br/)
 * Universidade de Passo Fundo (http://www.upf.br/)
 * 
 * @copyright
 * Copyright (C) 2018 by Angelo Elias Dalzotto
 * 
 * @brief This is a quick library for SPI in AVR microcontrollers.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SPI_H_
#define _SPI_H_

#include <avr/io.h>

class SPI {
public:
    static void init(volatile uint8_t *DDR_SS, volatile uint8_t *DDR_SCK, volatile uint8_t *DDR_MOSI, volatile uint8_t *DDR_MISO,
                     uint8_t PIN_SS, uint8_t PIN_SCK, uint8_t PIN_MOSI, uint8_t PIN_MISO, volatile uint8_t *PORT_SS);
    static void write(uint8_t data);
    static uint8_t read();
    static void set_speed();

private:
    static bool initialized;

};


#endif /* _SPI_H_ */