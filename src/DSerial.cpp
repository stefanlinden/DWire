/*
 * DSerial.cpp
 *
 *  Created on: 18 May 2016
 *      Author: Stefan van der Linden
 */

#include "DSerial.h"

/**** INCLUDES ****/
#include<string.h>

/**** GLOBAL VARIABLES ****/


/**** CONSTRUCTORS ****/
DSerial::DSerial( void ) {
    // Nothing
}

/**** PUBLIC METHODS ****/
void DSerial::begin( void ) {
    // Customise this to give module, baud rate, etc...
    /* Halting WDT  */
    MAP_WDT_A_holdTimer();

    /* Selecting P1.2 and P1.3 in UART mode */
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1,
            GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);

    /* Setting DCO to 12MHz */
    CS_setDCOCenteredFrequency(CS_DCO_FREQUENCY_48);

    /* Configuring UART Module */
    MAP_UART_initModule(EUSCI_A0_BASE, &uartConfig);

    /* Enable UART module */
    MAP_UART_enableModule(EUSCI_A0_BASE);

    /* Enabling interrupts */
    /*MAP_UART_enableInterrupt(EUSCI_A0_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);
    MAP_Interrupt_enableInterrupt(INT_EUSCIA0);
    MAP_Interrupt_enableSleepOnIsrExit();
    MAP_Interrupt_enableMaster();   */
}

void DSerial::print( const char * text ) {
    int ii;

    for( ii = 0; ii < strlen(text); ii++ ) {
        //MAP_UART_transmitData(EUSCI_A0_BASE, text[ii]);
        print(text[ii]);
    }
}

void DSerial::print( uint_fast8_t byte ) {
    MAP_UART_transmitData(EUSCI_A0_BASE, byte);
}

void DSerial::println( const char * text ) {
    // The same as print, but add a carriage return after the message
    print(text);
    MAP_UART_transmitData(EUSCI_A0_BASE, '\r');
    MAP_UART_transmitData(EUSCI_A0_BASE, '\n');
}

/**** PRIVATE METHODS ****/

