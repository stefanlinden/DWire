/*
 * BASIC_MASTER_SLAVE.cpp
 *
 *  Created on: 23 May 2016
 *      Author: Stefan van der Linden
 *
 * DWire (Delft-Wire)
 * Provides a simple library for handling I2C, trying to be similar to Arduino's Wire library
 * Many of the variable names and methods are kept identical to the original (Two)Wire.
 *
 * This example shows both the use of DWire as a master and a slave (set by #define'ing or #undef'ing I2C_MASTER).
 * Two different boards are required: one running as master transmitting data, and a second one running as a slave
 * receiving and copying the data to a serial line.
 *
 * Connected lines are: P1.6 (SDA) and P1.7 (SCL) with pull-up resistors to VCC
 *
 */

// Comment out/#undef the next line if the code is to be used as a slave setup
#define I2C_MASTER

/* Custom Includes */
#include "DWire.h"
#include "DSerial.h"

DWire* wire;
DSerial * serial;

uint_fast8_t i = 0;

void handleReceive( uint8_t );
void handleRequest( void );

int main( void ) {
    /* Disabling the Watchdog */
    MAP_WDT_A_holdTimer( );

    // Start the serial port
    serial = new DSerial( );
    serial->begin( );
    serial->println("Starting...");

    // Create a new instance of DWire on the EUSCI_B0_BASE module
    wire = new DWire( EUSCI_B0_BASE);

// The code for the master setup
#ifdef I2C_MASTER
    wire->begin( );
    serial->println("Ready!");

    // Loop through all ASCII values and write this to the slave

    while ( 1 ) {

        // Start a frame
        wire->beginTransmission(0x42);

        // Write three bytes
        wire->write(i);
        wire->write(i + 1);
        wire->write(i + 2);
        i++;
        serial->println("Sending message");

        // Mark the frame as finished. This causes DWire to transmit the buffer's contents
        wire->endTransmission( );

        // Wait a while until transmitting the next frame
        for ( int ii = 0; ii < 50000; ii++ )
            ;

        serial->println("Requesting data...");
        uint8_t * data = wire->requestFrom(0x42, 6);

        serial->print("Got: ");
        for ( int j = 0; j < 6; j++ ) {
            serial->print(data[j]);
        }
        serial->println( );

        for ( int ii = 0; ii < 5000000; ii++ )
            ;
    }

// slave code
#else
    // Start the module as a slave on address 0x48
    wire->begin(0x42);
    wire->onReceive(handleReceive);
    wire->onRequest(handleRequest);
    serial->println("Ready as slave.");

    // Loop, with interrupts running the different parts of the slave
    while ( 1 )
    ;
#endif
}

/**
 * Receive interrupt handler
 * This interrupt is triggered by DWire when a full frame has been received
 * (i.e. after receiving a STOP)
 */
void handleReceive( uint8_t numBytes ) {

    serial->print("Got a message: ");

    // Get the rx buffer's contents from the DWire object
    for ( int i = 0; i < numBytes; i++ ) {
        uint8_t byte = wire->read( );

        // Print the contents of the received byte
        serial->print(byte);
    }

    // End the line in preparation of the next receive event
    serial->println( );
}

/**
 * Request interrupt handler
 * This request is called on a read request from a master node.
 *
 */
void handleRequest( void ) {
    serial->println("Sending six bytes.");
    wire->write(0x41);
    wire->write(0x42);
    wire->write(0x43);
    wire->write(0x44);
    wire->write(0x45);
    wire->write(0x46);
}
