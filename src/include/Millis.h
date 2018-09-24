/**
 * @file Millis.h
 * 
 * @author
 * Angelo Elias Dalzotto (150633@upf.br)
 * GEPID - Grupo de Pesquisa em Cultura Digital (http://gepid.upf.br/)
 * Universidade de Passo Fundo (http://www.upf.br/)
 * 
 * @copyright
 * Copyright (c) 2018 Angelo Elias Dalzotto & Gabriel Boni Vicari
 * 
 * @brief The Millis class manages the milliseconds timer.
 */

#ifndef _MILLIS_H_
#define _MILLIS_H_

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

/**
 * TIMER2 interrupt service.
 * It sums the milliseconds counter.
 */
extern "C" void TIMER2_COMPA_vect(void) __attribute__((signal));

/**
 * Millis class
 */
class Millis {
public:

    /**
     * @brief Timer initializer.
     * 
     * @details This class only use static methods and attributes.
     * This is the "static constructor".
     */
    static void init();

    /**
     * @brief Millisecond counter getter.
     * 
     * @returns Millisecond counter since the init() has been called.
     */
    static uint32_t get();
    
    /**
     * @brief Timer ISR declaration.
     * 
     * @details This is done to include the ISR in the class.
     * This is not to be called.
     */
    friend void TIMER2_COMPA_vect(void);

private:
    static bool initialized;
    static uint32_t counter;
};

#endif /* _MILLIS_H_ */