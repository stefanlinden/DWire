/*
 * DSerial.h
 *
 *  Created on: 18 May 2016
 *      Author: stefa_000
 */

#ifndef DWIRE_DSERIAL_H_
#define DWIRE_DSERIAL_H_

/* DRIVERLIB */
#include "driverlib.h"

/* UART Configuration Parameter. These are the configuration parameters to
 * make the eUSCI A UART module to operate with a 9600 baud rate. These
 * values were calculated using the online calculator that TI provides
 * at:
 * http://software-dl.ti.com/msp430/msp430_public_sw/mcu/msp430/MSP430BaudRateConverter/index.html
 */

const eUSCI_UART_Config uartConfig =
{
        EUSCI_A_UART_CLOCKSOURCE_SMCLK,          // SMCLK Clock Source
        156,                                     // BRDIV = 78
        4,                                       // UCxBRF = 2
        0,                                       // UCxBRS = 0
        EUSCI_A_UART_NO_PARITY,                  // No Parity
        EUSCI_A_UART_LSB_FIRST,                  // LSB First
        EUSCI_A_UART_ONE_STOP_BIT,               // One stop bit
        EUSCI_A_UART_MODE,                       // UART mode
        EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION  // Oversampling
};

class DSerial {
private:

public:
    DSerial( void );
    void begin( void );
    void print( uint_fast8_t );
    void print( const char * );
    void println( const char * );
};

#endif /* DWIRE_DSERIAL_H_ */
