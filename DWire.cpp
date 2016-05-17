/*
 * DWire.cpp
 *
 *  Created on: 16 May 2016
 *      Author: Stefan van der Linden
 *
 * DWire (Delft-Wire)
 * Provides a simple library for handling I2C, trying to be similar to Arduino's Wire library
 * Many of the variable names and methods are kept identical to the original (Two)Wire.
 *
 */

/* Standard includes */
extern "C" {
#include <stdint.h>
#include <stdbool.h>
}

#include <hash_map>

#include "DWire.h"

/**** PROTOTYPES ****/

void IRQHandler( uint32_t, std::vector<uint8_t> *, uint8_t );

/**** GLOBAL VARIABLES ****/
#ifdef USING_EUSCI_B0
static std::vector<uint8_t> EUSCIB0_rxBuffer;
#endif
#ifdef USING_EUSCI_B1
static std::vector<uint8_t> EUSCIB1_rxBuffer;
#endif
#ifdef USING_EUSCI_B2
static std::vector<uint8_t> EUSCIB2_rxBuffer;
#endif
#ifdef USING_EUSCI_B3
static std::vector<uint8_t> EUSCIB3_rxBuffer;
#endif

static std::hash_map<uint32_t, DWire *> moduleMap;

uint8_t EUSCIB0_rxBufferIndex = 0;
uint8_t EUSCIB1_rxBufferIndex = 0;
uint8_t EUSCIB2_rxBufferIndex = 0;
uint8_t EUSCIB3_rxBufferIndex = 0;

/**** CONSTRUCTORS ****/

DWire::DWire( uint32_t module ) {

    this->module = module;

    txBufferIndex = 0;
    rxReadIndex = 0;
    rxReadLength = 0;

    slaveAddress = 0;

    status = BUS_STATUS_NONE;
    busRole = BUS_ROLE_SLAVE;

    void (*user_onRequest)( void );
    void (*user_onReceive)( uint8_t, uint8_t * );

    // Register this instance in the 'moduleMap'
    moduleMap[module] = this;
}

DWire::~DWire( ) {
    // Deregister from the moduleMap
    moduleMap[module] = 0;
}

/**** PUBLIC METHODS ****/

void DWire::begin( void ) {
    // Initialising the given module as a master
    busRole = BUS_ROLE_MASTER;

    const eUSCI_I2C_MasterConfig i2cConfig = {
    EUSCI_B_I2C_CLOCKSOURCE_SMCLK,          // SMCLK Clock Source
            48000000,                                // SMCLK = 3MHz
            EUSCI_B_I2C_SET_DATA_RATE_400KBPS, // Desired I2C Clock of 400khz // TODO make configurable
            0,                                      // No byte counter threshold
            EUSCI_B_I2C_NO_AUTO_STOP                // No Autostop
            };
    _initMaster(&i2cConfig);
}

void DWire::begin( uint8_t address ) {
    // Initialising the given module as a slave
    busRole = BUS_ROLE_SLAVE;
    slaveAddress = address;
    _initSlave( );
}

/**
 * Begin a transmission as a master
 */
void DWire::beginTransmission( uint_fast8_t slaveAddress ) {
    // Starting a transmission as a master to the slave at slaveAddress
    if ( busRole != BUS_ROLE_MASTER )
        return;

    // Wait in case a previous message is still being sent
    while ( MAP_I2C_masterIsStopSent(module) == EUSCI_B_I2C_SENDING_STOP )
        ;

    if ( slaveAddress != this->slaveAddress )
        _setSlaveAddress(slaveAddress);

    // Reset the buffer index
    txBufferIndex = 0;
}

/**
 * Write a single byte to the tx buffer
 */
void DWire::write( uint8_t dataByte ) {
    txBuffer.push_back(dataByte);
    txBufferIndex++;
}

/**
 * End the transmission and transmit the tx buffer's contents over the bus
 */
void DWire::endTransmission( ) {
    int ii;

    if ( txBufferIndex == 0 )
        return;

    // Wait until any ongoing (incoming) transmissions are finished
    while ( MAP_I2C_isBusBusy(module) == EUSCI_B_I2C_BUS_BUSY )
        ;

    status = BUS_STATUS_TX;

    // Send the start condition and initial byte
    MAP_I2C_masterSendMultiByteStart(module, txBuffer.at(0));

    for ( ii = 1; ii <= txBufferIndex; ii++ ) {
        MAP_I2C_masterSendMultiByteNext(module, txBuffer.at(ii));
    }

    MAP_I2C_masterSendMultiByteStop(module);

    status = BUS_STATUS_RDY;
}

/**
 * Reads a single byte from the rx buffer
 */
uint8_t DWire::read( void ) {
    if ( rxReadIndex == 0 && rxReadLength == 0 )
        return 0;

    // Read normally up to the last byte
    if ( rxReadIndex < rxReadLength - 1 ) {
        rxReadIndex++;
        return rxBuffer[rxReadIndex - 1];
    }

    if ( rxReadIndex == rxReadLength ) {
        // Read the last byte and then clear
        uint8_t value = rxBuffer[rxReadIndex];

        rxReadIndex = 0;
        rxReadLength = 0;
        return value;
    }
    return 0;
}

/**
 * Register the user's interrupt handler
 */
void DWire::onRequest( void (*islHandle)( void ) ) {
    user_onRequest = islHandle;
}

/**
 * Register the interrupt handler
 * The two arguments for the handle are:
 * 1. number of bytes received ( == RX_BUFFER_SIZE)
 * 2. the actual bytes
 */
void DWire::onReceive( void (*islHandle)( uint8_t ) ) {
    user_onReceive = islHandle;
}

/**** PRIVATE METHODS ****/

void DWire::_initMaster( const eUSCI_I2C_MasterConfig * i2cConfig ) {
    /* Initializing I2C Master to SMCLK at 400kbs with no autostop */
    MAP_I2C_initMaster(module, i2cConfig);

    /* Specify slave address */
    MAP_I2C_setSlaveAddress(module, 0);

    /* Set Master in receive mode */
    MAP_I2C_setMode(module, EUSCI_B_I2C_TRANSMIT_MODE);

    /* Enable I2C Module to start operations */
    MAP_I2C_enableModule(module);

    // Update the status
    status = BUS_STATUS_RDY;

    /* Enable and clear the interrupt flag */
    MAP_I2C_clearInterruptFlag(module, EUSCI_B_I2C_NAK_INTERRUPT);

    //Enable master Receive interrupt
    MAP_I2C_enableInterrupt(module, EUSCI_B_I2C_NAK_INTERRUPT);
    uint32_t intModule;
    switch ( module ) {
    case EUSCI_B0_BASE:
        intModule = INT_EUSCIB0;
        break;
    case EUSCI_B1_BASE:
        intModule = INT_EUSCIB1;
        break;
    case EUSCI_B2_BASE:
        intModule = INT_EUSCIB2;
        break;
    case EUSCI_B3_BASE:
        intModule = INT_EUSCIB3;
        break;
    default:
        return;
    }
    MAP_Interrupt_enableInterrupt(intModule);
}

void DWire::_initSlave( void ) {
    MAP_I2C_initSlave(module, slaveAddress, EUSCI_B_I2C_OWN_ADDRESS_OFFSET0,
    EUSCI_B_I2C_OWN_ADDRESS_ENABLE);

    /* Set in receive mode */
    MAP_I2C_setMode(module, EUSCI_B_I2C_RECEIVE_MODE);

    /* Enable the module */
    MAP_I2C_enableModule(module);

    // Update the status
    status = BUS_STATUS_RDY;

    /* Enable the module and enable interrupts */
    MAP_I2C_enableModule(EUSCI_B0_BASE);
    MAP_I2C_clearInterruptFlag(EUSCI_B0_BASE,
            EUSCI_B_I2C_RECEIVE_INTERRUPT0 | EUSCI_B_I2C_STOP_INTERRUPT);
    MAP_I2C_enableInterrupt(EUSCI_B0_BASE,
            EUSCI_B_I2C_RECEIVE_INTERRUPT0 | EUSCI_B_I2C_STOP_INTERRUPT);
    MAP_Interrupt_enableSleepOnIsrExit( );
    MAP_Interrupt_enableInterrupt(INT_EUSCIB0);
    MAP_Interrupt_enableMaster( );
}

/**
 * Re-set the slave address (the target address when master or the slave's address when slave)
 */
void DWire::_setSlaveAddress( uint_fast8_t newAddress ) {
    slaveAddress = newAddress;
    MAP_I2C_setSlaveAddress(module, newAddress);
}

/**
 * Handle a request ISL
 */
void DWire::_handleRequest( void ) {
    // If there is no handler registered, then there is nothing to do
    if ( !user_onRequest )
        return;
}

void DWire::_handleReceive( uint8_t * rxBufferIndex,
        std::vector<uint8_t> * rxBuffer ) {
    rxReadLength = *rxBufferIndex;
    rxReadIndex = 0;

    this->rxBuffer = new uint8_t[rxReadLength];
    std::copy(rxBuffer->begin(), rxBuffer->end(),this->rxBuffer);

    if( !user_onReceive )
        return;
    user_onReceive( rxReadLength );
}

/**** ISR/IQR Handles ****/

void IRQHandler( uint32_t module, std::vector<uint8_t> * rxBuffer,
        uint8_t * rxBufferIndex ) {

    uint_fast16_t status;

    status = MAP_I2C_getEnabledInterruptStatus(module);
    MAP_I2C_clearInterruptFlag(module, status);

    /* RXIFG */
    if ( status & EUSCI_B_I2C_RECEIVE_INTERRUPT0 ) {

        rxBuffer->push_back(MAP_I2C_slaveGetData(module));
        (*rxBufferIndex)++;
    }

    /* STPIFG
     * Called when a STOP is received
     */
    if ( status & EUSCI_B_I2C_STOP_INTERRUPT ) {
        DWire * instance = moduleMap[module];
        if ( instance ) {
            instance->_handleReceive(rxBufferIndex, rxBuffer);
        }
    }
}

#ifdef USING_EUSCI_B0
/*
 * Handle everything on EUSCI_B0
 */
void EUSCIB0_IRQHandler( void ) {
    IRQHandler(EUSCI_B0_BASE, &EUSCIB0_rxBuffer, &EUSCIB0_rxBufferIndex);
}

/* USING_EUSCI_B0 */
#endif

#ifdef USING_EUSCI_B1
/*
 * Handle everything on EUSCI_B0
 */
void EUSCIB1_IRQHandler( void ) {
    IRQHandler(EUSCI_B1_BASE, &EUSCIB1_rxBuffer, &EUSCIB1_rxBufferIndex);
}

/* USING_EUSCI_B1 */
#endif
