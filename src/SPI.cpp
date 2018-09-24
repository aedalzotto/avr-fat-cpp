/**
 * @file SPI.cpp
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

#include <SPI.h>

bool SPI::initialized = false;

void SPI::init(volatile uint8_t *DDR_SS, volatile uint8_t *DDR_SCK, volatile uint8_t *DDR_MOSI, volatile uint8_t *DDR_MISO,
               uint8_t PIN_SS, uint8_t PIN_SCK, uint8_t PIN_MOSI, uint8_t PIN_MISO, volatile uint8_t *PORT_SS)
{
    if(initialized)
        return;

    *DDR_SS |= (1 << PIN_SS); // Sets high hardware CS even if not used.
    *PORT_SS |= (1 << PIN_SS);
    *DDR_MISO &= ~(1 << PIN_MISO);
    *DDR_SCK |= (1 << PIN_SCK);
    *DDR_MOSI |= (1 << PIN_MOSI);

    // Enable SPI, Master, clock rate f_osc/128
    SPCR |= (1 << SPE) | (1 << MSTR) | (1 << SPR1) | (1 << SPR0);
    // clear double speed
    SPSR &= ~(1 << SPI2X);

    initialized = true;
}

void SPI::set_speed()
{
    SPCR &= ~((1 << SPR1) | (1 << SPR0));
    SPSR |= (1 << SPI2X);
}

void SPI::write(uint8_t data)
{
    SPDR = data;
    loop_until_bit_is_set(SPSR, SPIF);
}

uint8_t SPI::read()
{
    write(0xFF);
    return SPDR;
}