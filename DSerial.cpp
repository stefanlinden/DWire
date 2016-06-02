/*
 * Copyright (c) 2016 by Stefan van der Linden <spvdlinden@gmail.com>
 *
 * DSerial: Serial library to provide Energia-like UART functionality
 * to non-Energia projects
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License 
 * version 3, both as published by the Free Software Foundation.
 *
 */

/**** INCLUDES ****/
#include<string.h>

#include "DSerial.h"

/**** GLOBAL VARIABLES ****/

/**** CONSTRUCTORS ****/
DSerial::DSerial( void ) {
    // Nothing
}

/**** PUBLIC METHODS ****/
void DSerial::begin( void ) {
    // Customise this to give module, baud rate, etc...
    /* Halting WDT  */
    MAP_WDT_A_holdTimer( );

    /* Selecting P1.2 and P1.3 in UART mode */
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1,
    GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);

    /* Setting DCO to 48MHz */
    CS_setDCOCenteredFrequency(CS_DCO_FREQUENCY_48);

    /* Configuring UART Module */
    MAP_UART_initModule(EUSCI_A0_BASE, &uartConfig);

    /* Enable UART module */
    MAP_UART_enableModule(EUSCI_A0_BASE);

}

/**
 * Transmit a single byte over the UART
 */
void DSerial::print( uint_fast8_t byte ) {
    MAP_UART_transmitData(EUSCI_A0_BASE, byte);
}

/**
 * Print a string over the UART
 */
void DSerial::print( const char * text ) {

    for ( int ii = 0; ii < strlen(text); ii++ ) {
        print(text[ii]);
    }
}

/**
 * Transmit a carriage return
 */
void DSerial::println( void ) {
    MAP_UART_transmitData(EUSCI_A0_BASE, '\r');
    MAP_UART_transmitData(EUSCI_A0_BASE, '\n');
}

/**
 * Transmit a single byte and end with a newline
 */
void DSerial::println( uint_fast8_t byte ) {
    // The same as print, but add a carriage return after the message
    print(byte);
    println( );
}

/**
 * Print text and end with a newline
 */
void DSerial::println( const char * text ) {
    // The same as print, but add a carriage return after the message
    print(text);
    println( );
}

/**** PRIVATE METHODS ****/
