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

void IRQHandler( uint32_t module, uint8_t *, uint8_t *, std::vector<uint8_t> *,
        uint8_t *, uint8_t * );

/**** GLOBAL VARIABLES ****/

// The buffers need to be declared globally, as the interrupts are too
#ifdef USING_EUSCI_B0

static uint8_t * EUSCIB0_txBuffer = new uint8_t[TX_BUFFER_SIZE];
static uint8_t EUSCIB0_txBufferIndex = 0;
static uint8_t EUSCIB0_txBufferSize;

static uint8_t * EUSCIB0_rxBuffer = new uint8_t[RX_BUFFER_SIZE];
static uint8_t EUSCIB0_rxBufferIndex = 0;
#endif

#ifdef USING_EUSCI_B1

static uint8_t * EUSCIB1_txBuffer = new uint8_t[TX_BUFFER_SIZE];
static uint8_t EUSCIB1_txBufferIndex = 0;
static uint8_t EUSCIB1_txBufferSize;

static uint8_t * EUSCIB1_rxBuffer = new uint8_t[RX_BUFFER_SIZE];
static uint8_t EUSCIB1_rxBufferIndex = 0;
#endif

#ifdef USING_EUSCI_B2

static uint8_t * EUSCIB2_txBuffer = new uint8_t[TX_BUFFER_SIZE];
static uint8_t EUSCIB2_txBufferIndex = 0;
static uint8_t EUSCIB2_txBufferSize;

static uint8_t * EUSCIB2_rxBuffer = new uint8_t[RX_BUFFER_SIZE];
static uint8_t EUSCIB2_rxBufferIndex = 0;
#endif

#ifdef USING_EUSCI_B3

static uint8_t * EUSCIB3_txBuffer = new uint8_t[TX_BUFFER_SIZE];
static uint8_t EUSCIB3_txBufferIndex = 0;
static uint8_t EUSCIB3_txBufferSize;

static uint8_t * EUSCIB3_rxBuffer = new uint8_t[RX_BUFFER_SIZE];
static uint8_t EUSCIB3_rxBufferIndex = 0;
#endif

static std::hash_map<uint32_t, DWire *> moduleMap;

/**** CONSTRUCTORS ****/

DWire::DWire( uint32_t module ) {

    this->module = module;

    rxReadIndex = 0;
    rxReadLength = 0;
    rxBuffer = new uint8_t[RX_BUFFER_SIZE];

    slaveAddress = 0;

    status = BUS_STATUS_NONE;
    busRole = BUS_ROLE_SLAVE;

    switch ( module ) {
    case EUSCI_B0_BASE:
        pTxBuffer = EUSCIB0_txBuffer;
        pTxBufferIndex = &EUSCIB0_txBufferIndex;
        pTxBufferSize = &EUSCIB0_txBufferSize;
        break;
    case EUSCI_B1_BASE:
        //intModule = INT_EUSCIB1;
        break;
    case EUSCI_B2_BASE:
        //intModule = INT_EUSCIB2;
        break;
    case EUSCI_B3_BASE:
        //intModule = INT_EUSCIB3;
        break;
    default:
        return;
    }

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

}

/**
 * Write a single byte to the tx buffer
 */
void DWire::write( uint8_t dataByte ) {
    pTxBuffer[*pTxBufferIndex] = dataByte;
    //uint8_t * buffer = new uint8_t[pTxBuffer->size()];
    //std::copy(pTxBuffer->begin(), pTxBuffer->end(),buffer);
    (*pTxBufferIndex)++;
}

/**
 * End the transmission and transmit the tx buffer's contents over the bus
 */
void DWire::endTransmission( ) {

    // Wait until any ongoing (incoming) transmissions are finished
    while ( MAP_I2C_isBusBusy(module) == EUSCI_B_I2C_BUS_BUSY )
        ;

    status = BUS_STATUS_TX;

    // Send the start condition and initial byte
    (*pTxBufferSize) = *pTxBufferIndex;
    (*pTxBufferIndex)--;
    MAP_I2C_masterSendMultiByteStart(module, pTxBuffer[0]);

}

/**
 * Reads a single byte from the rx buffer
 */
uint8_t DWire::read( void ) {

    // Wait if there is nothing to read
    while ( rxReadIndex == 0 && rxReadLength == 0 )
        ;

    uint8_t byte = rxBuffer[rxReadIndex];
    rxReadIndex++;

    // Check whether this was the last byte. If so, reset.
    if ( rxReadIndex == rxReadLength ) {
        rxReadIndex = 0;
        rxReadLength = 0;
    }
    return byte;
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
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1,
    GPIO_PIN6 + GPIO_PIN7, GPIO_PRIMARY_MODULE_FUNCTION);

    // Initializing I2C Master to SMCLK at 400kbs with no autostop
    MAP_I2C_initMaster(module, i2cConfig);

    // Specify slave address
    MAP_I2C_setSlaveAddress(module, 0);

    // Set Master in receive mode
    MAP_I2C_setMode(module, EUSCI_B_I2C_TRANSMIT_MODE);

    // Enable I2C Module to start operations
    MAP_I2C_enableModule(module);

    // Update the status
    status = BUS_STATUS_RDY;

    // Enable and clear the interrupt flag
    MAP_I2C_clearInterruptFlag(module,
    EUSCI_B_I2C_TRANSMIT_INTERRUPT0 + EUSCI_B_I2C_NAK_INTERRUPT);

    // Enable master Receive interrupt
    MAP_I2C_enableInterrupt(module,
    EUSCI_B_I2C_TRANSMIT_INTERRUPT0 + EUSCI_B_I2C_NAK_INTERRUPT);

    // Register the correct module for the interrupt
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
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1,
    GPIO_PIN6 + GPIO_PIN7, GPIO_PRIMARY_MODULE_FUNCTION);

    MAP_I2C_initSlave(module, slaveAddress, EUSCI_B_I2C_OWN_ADDRESS_OFFSET0,
    EUSCI_B_I2C_OWN_ADDRESS_ENABLE);

    // Set in receive mode
    MAP_I2C_setMode(module, EUSCI_B_I2C_RECEIVE_MODE);

    // Enable the module
    MAP_I2C_enableModule(module);

    // Enable the module and enable interrupts
    MAP_I2C_enableModule(module);
    MAP_I2C_clearInterruptFlag(module,
    EUSCI_B_I2C_RECEIVE_INTERRUPT0 | EUSCI_B_I2C_STOP_INTERRUPT);
    MAP_I2C_enableInterrupt(module,
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

void DWire::_handleReceive( uint8_t * rxBufferIndex, uint8_t * rxBuffer ) {
    // No need to do anything if there is no handler registered
    if ( !user_onReceive )
        return;

    // Check whether the user application is still reading out the local buffer.
    // This needs to be tested to make sure it doesn't give any problems.
    if ( rxReadIndex != 0 && rxReadLength != 0 )
        return;

    // Copy the main buffer into a local buffer
    rxReadLength = *rxBufferIndex;
    rxReadIndex = 0;

    for ( int i = 0; i < rxReadLength; i++ )
        this->rxBuffer[i] = rxBuffer[i];

    // Reset the main buffer
    (*rxBufferIndex) = 0;

    user_onReceive(rxReadLength);
}

/**** ISR/IRQ Handles ****/

/**
 * The main (global) interrupt  handler
 */
void IRQHandler( uint32_t module, uint8_t * rxBuffer, uint8_t * rxBufferIndex,
        uint8_t * txBuffer, uint8_t * txBufferIndex, uint8_t * txBufferSize ) {

    uint_fast16_t status;

    status = MAP_I2C_getEnabledInterruptStatus(module);
    MAP_I2C_clearInterruptFlag(module, status);

    /* RXIFG */
    // Triggered when data has been received
    if ( status & EUSCI_B_I2C_RECEIVE_INTERRUPT0 ) {

        rxBuffer[*rxBufferIndex] = MAP_I2C_slaveGetData(module);
        (*rxBufferIndex)++;
    }

    // As master: triggered when a byte has been transmitted
    // As slave: triggered on request (tbc) */
    if ( status & EUSCI_B_I2C_TRANSMIT_INTERRUPT0 ) {

        // If we've transmitted the last byte from the buffer, then send a stop
        if ( !(*txBufferIndex) ) {
            MAP_I2C_masterSendMultiByteStop(module);
        } else {

            // If we still have data left in the buffer, then transmit that
            MAP_I2C_masterSendMultiByteNext(module,
                    txBuffer[(*txBufferSize) - (*txBufferIndex)]);
            (*txBufferIndex)--;
        }

    }

    if ( status & EUSCI_B_I2C_NAK_INTERRUPT ) {
        MAP_I2C_masterSendStart(module);
        // TODO verify whether this is enough or we need to bring back the buffer by one item
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
extern "C" {
void EUSCIB0_IRQHandler( void ) {
    IRQHandler(EUSCI_B0_BASE, EUSCIB0_rxBuffer, &EUSCIB0_rxBufferIndex,
            EUSCIB0_txBuffer, &EUSCIB0_txBufferIndex, &EUSCIB0_txBufferSize);
}
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
