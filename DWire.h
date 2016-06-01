/*
 * DWire.h
 *
 *  Created on: 16 May 2016
 *      Author: Stefan van der Linden
 *
 * DWire (Delft-Wire)
 * Provides a simple library for handling I2C, trying to be similar to Arduino's Wire library
 * Many of the variable names and methods are kept identical to the original (Two)Wire.
 *
 */

#ifndef DWIRE_DWIRE_H_
#define DWIRE_DWIRE_H_


#define USING_EUSCI_B0
#define USING_EUSCI_B1
#define USING_EUSCI_B2
#define USING_EUSCI_B3


// Similar for the roles
#define BUS_ROLE_MASTER 0
#define BUS_ROLE_SLAVE 1

// Default buffer size in bytes
#define TX_BUFFER_SIZE 32
#define RX_BUFFER_SIZE 32

/* Driverlib */
#ifdef ENERGIA
#include "driverlib/driverlib.h"
#else
#include "driverlib.h"
#endif

/* Device specific includes */
#include "inc/dwire_pins.h"

#ifdef ENERGIA
extern "C" {
#ifdef USING_EUSCI_B0
extern void EUSCIB0_IRQHandler( void );
#endif

#ifdef USING_EUSCI_B1
extern void EUSCIB1_IRQHandler( void );
#endif

#ifdef USING_EUSCI_B2
extern void EUSCIB2_IRQHandler( void );
#endif

#ifdef USING_EUSCI_B3
extern void EUSCIB3_IRQHandler( void );
#endif
}
#endif

/* Main class definition */
class DWire {
private:

    volatile uint8_t * pTxBufferIndex;
    uint8_t * pTxBuffer;
    volatile uint8_t * pTxBufferSize;

    volatile uint8_t rxReadIndex;
    volatile uint8_t rxReadLength;

    uint8_t rxLocalBuffer[RX_BUFFER_SIZE];

    uint8_t * pRxBuffer;
    uint8_t * pRxBufferIndex;
    uint8_t * pRxBufferSize;

    volatile bool requestDone;

    uint8_t slaveAddress;

    uint8_t busRole;

    uint32_t intModule;

    uint_fast8_t modulePort;
    uint_fast16_t modulePins;

    void (*user_onRequest)( void );
    void (*user_onReceive)( uint8_t );

    void _initMaster( const eUSCI_I2C_MasterConfig * );
    void _initSlave( void );
    void _setSlaveAddress( uint_fast8_t );

public:

    uint_fast32_t module;

    /* Constructors */
    DWire( uint32_t );
    ~DWire( );

    /* MASTER specific */
    void begin( void );

    void beginTransmission( uint_fast8_t );
    void write( uint8_t );
    void endTransmission( void );

    uint8_t * requestFrom( uint_fast8_t, uint_fast8_t );

    /* SLAVE specific */
    void begin( uint8_t );

    uint8_t read( void );

    void onRequest( void (*)( void ) );
    void onReceive( void (*)( uint8_t ) );

    /* Miscellaneous */
    bool isMaster( void );

    /* Internal */
    void _handleReceive( uint8_t * );
    void _handleRequestSlave( void );
    void _finishRequest( void );
};


#endif /* DWIRE_DWIRE_H_ */
