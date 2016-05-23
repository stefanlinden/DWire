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


/**** PROTOTYPES AND CLASSES ****/

typedef struct {
    uint32_t module;
    uint8_t * rxBuffer;
    uint8_t * rxBufferIndex;
    uint8_t * txBuffer;
    uint8_t * txBufferIndex;
    uint8_t * txBufferSize;
} IRQParam;

void IRQHandler( IRQParam );


/**** GLOBAL VARIABLES ****/

// The buffers need to be declared globally, as the interrupts are too
#ifdef USING_EUSCI_B0

uint8_t * EUSCIB0_txBuffer = new uint8_t[TX_BUFFER_SIZE];
uint8_t EUSCIB0_txBufferIndex = 0;
uint8_t EUSCIB0_txBufferSize;

uint8_t * EUSCIB0_rxBuffer = new uint8_t[RX_BUFFER_SIZE];
uint8_t EUSCIB0_rxBufferIndex = 0;
#endif

#ifdef USING_EUSCI_B1

uint8_t * EUSCIB1_txBuffer = new uint8_t[TX_BUFFER_SIZE];
uint8_t EUSCIB1_txBufferIndex = 0;
uint8_t EUSCIB1_txBufferSize;

uint8_t * EUSCIB1_rxBuffer = new uint8_t[RX_BUFFER_SIZE];
uint8_t EUSCIB1_rxBufferIndex = 0;
#endif

#ifdef USING_EUSCI_B2

uint8_t * EUSCIB2_txBuffer = new uint8_t[TX_BUFFER_SIZE];
uint8_t EUSCIB2_txBufferIndex = 0;
uint8_t EUSCIB2_txBufferSize;

uint8_t * EUSCIB2_rxBuffer = new uint8_t[RX_BUFFER_SIZE];
uint8_t EUSCIB2_rxBufferIndex = 0;
#endif

#ifdef USING_EUSCI_B3

uint8_t * EUSCIB3_txBuffer = new uint8_t[TX_BUFFER_SIZE];
uint8_t EUSCIB3_txBufferIndex = 0;
uint8_t EUSCIB3_txBufferSize;

uint8_t * EUSCIB3_rxBuffer = new uint8_t[RX_BUFFER_SIZE];
uint8_t EUSCIB3_rxBufferIndex = 0;
#endif

// TODO find an efficient way of getting rid of the hashmap
static std::hash_map<uint32_t, DWire *> moduleMap;

/**** CONSTRUCTORS ****/

DWire::DWire( uint32_t module ) {

    this->module = module;

    // The receiver buffer and related variables
    rxReadIndex = 0;
    rxReadLength = 0;
    rxBuffer = new uint8_t[RX_BUFFER_SIZE];

    slaveAddress = 0;

    busRole = 0;

    switch ( module ) {
    case EUSCI_B0_BASE:
        pTxBuffer = EUSCIB0_txBuffer;
        pTxBufferIndex = &EUSCIB0_txBufferIndex;
        pTxBufferSize = &EUSCIB0_txBufferSize;
        break;
#ifdef USING_EUSCI_B1
        case EUSCI_B1_BASE:
        pTxBuffer = EUSCIB1_txBuffer;
        pTxBufferIndex = &EUSCIB1_txBufferIndex;
        pTxBufferSize = &EUSCIB1_txBufferSize;
        break;
#endif
#ifdef USING_EUSCI_B2
        case EUSCI_B2_BASE:
        pTxBuffer = EUSCIB2_txBuffer;
        pTxBufferIndex = &EUSCIB2_txBufferIndex;
        pTxBufferSize = &EUSCIB2_txBufferSize;
        break;
#endif
#ifdef USING_EUSCI_B3
        case EUSCI_B3_BASE:
        pTxBuffer = EUSCIB3_txBuffer;
        pTxBufferIndex = &EUSCIB3_txBufferIndex;
        pTxBufferSize = &EUSCIB3_txBufferSize;
        break;
#endif
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
    EUSCI_B_I2C_CLOCKSOURCE_SMCLK,                   // SMCLK Clock Source
            48000000,                                // SMCLK = 48MHz
            EUSCI_B_I2C_SET_DATA_RATE_400KBPS, // Desired I2C Clock of 400khz // TODO make configurable
            0,                                      // No byte counter threshold
            EUSCI_B_I2C_NO_AUTO_STOP                 // No Autostop
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
    (*pTxBufferIndex)++;
}

/**
 * End the transmission and transmit the tx buffer's contents over the bus
 */
void DWire::endTransmission( ) {

    // Wait until any ongoing (incoming) transmissions are finished
    while ( MAP_I2C_isBusBusy(module) == EUSCI_B_I2C_BUS_BUSY )
        ;

    // Send the start condition and initial byte
    (*pTxBufferSize) = *pTxBufferIndex;
    (*pTxBufferIndex)--;

    // Send the first byte, triggering the TX interrupt
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
 * The argument contains the number of bytes received
 */
void DWire::onReceive( void (*islHandle)( uint8_t ) ) {
    user_onReceive = islHandle;
}

/**** PRIVATE METHODS ****/

/**
 * Called to set the eUSCI module in 'master' mode
 */
void DWire::_initMaster( const eUSCI_I2C_MasterConfig * i2cConfig ) {

    // Initialise the pins
    // TODO make customisable, as these are only used by eUSCI B0
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

    // Enable and clear the interrupt flag
    MAP_I2C_clearInterruptFlag(module,
    EUSCI_B_I2C_TRANSMIT_INTERRUPT0 + EUSCI_B_I2C_NAK_INTERRUPT);

    // Enable master interrupts
    MAP_I2C_enableInterrupt(module,
    EUSCI_B_I2C_TRANSMIT_INTERRUPT0 + EUSCI_B_I2C_NAK_INTERRUPT);

    // Register the interrupts on the correct module
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
    // Init the pins
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1,
    GPIO_PIN6 + GPIO_PIN7, GPIO_PRIMARY_MODULE_FUNCTION);

    // initialise driverlib
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

/**
 * Internal process handling the rx buffers, and calling the user's interrupt handles
 */
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
void IRQHandler( IRQParam param ) {

    uint_fast16_t status;

    status = MAP_I2C_getEnabledInterruptStatus(param.module);
    MAP_I2C_clearInterruptFlag(param.module, status);

    /* RXIFG */
    // Triggered when data has been received
    if ( status & EUSCI_B_I2C_RECEIVE_INTERRUPT0 ) {

        param.rxBuffer[*param.rxBufferIndex] = MAP_I2C_slaveGetData(param.module);
        (*param.rxBufferIndex)++;
    }

    // As master: triggered when a byte has been transmitted
    // As slave: triggered on request (tbc) */
    if ( status & EUSCI_B_I2C_TRANSMIT_INTERRUPT0 ) {

        // If we've transmitted the last byte from the buffer, then send a stop
        if ( !(*param.txBufferIndex) ) {
            MAP_I2C_masterSendMultiByteStop(param.module);
        } else {

            // If we still have data left in the buffer, then transmit that
            MAP_I2C_masterSendMultiByteNext(param.module,
                    param.txBuffer[(*param.txBufferSize) - (*param.txBufferIndex)]);
            (*param.txBufferIndex)--;
        }

    }

    if ( status & EUSCI_B_I2C_NAK_INTERRUPT ) {
        MAP_I2C_masterSendStart(param.module);
        // TODO verify whether this is enough or we need to bring back the buffer by one item
    }

    /* STPIFG
     * Called when a STOP is received
     */
    if ( status & EUSCI_B_I2C_STOP_INTERRUPT ) {
        DWire * instance = moduleMap[param.module];
        if ( instance ) {
            instance->_handleReceive(param.rxBufferIndex, param.rxBuffer);
        }
    }
}

#ifdef USING_EUSCI_B0
/*
 * Handle everything on EUSCI_B0
 */
extern "C" {
void EUSCIB0_IRQHandler( void ) {
    IRQParam param;
    param.module = EUSCI_B0_BASE;
    param.rxBuffer = EUSCIB0_rxBuffer;
    param.rxBufferIndex = &EUSCIB0_rxBufferIndex;
    param.txBuffer = EUSCIB0_txBuffer;
    param.txBufferIndex = &EUSCIB0_txBufferIndex;
    param.txBufferSize = &EUSCIB0_txBufferSize;

    IRQHandler(param);
}
}

/* USING_EUSCI_B0 */
#endif

#ifdef USING_EUSCI_B1
/*
 * Handle everything on EUSCI_B1
 */
extern "C" {
void EUSCIB1_IRQHandler( void ) {
    IRQParam param;
    param.module = EUSCI_1_BASE;
    param.rxBuffer = EUSCIB1_rxBuffer;
    param.rxBufferIndex = &EUSCIB1_rxBufferIndex;
    param.txBuffer = EUSCIB1_txBuffer;
    param.txBufferIndex = &EUSCIB1_txBufferIndex;
    param.txBufferSize = &EUSCIB1_txBufferSize;

    IRQHandler(param);
}
}

/* USING_EUSCI_B1 */
#endif

#ifdef USING_EUSCI_B2
/*
 * Handle everything on EUSCI_B2
 */
extern "C" {
void EUSCIB2_IRQHandler( void ) {
    IRQParam param;
    param.module = EUSCI_B2_BASE;
    param.rxBuffer = EUSCIB2_rxBuffer;
    param.rxBufferIndex = &EUSCIB2_rxBufferIndex;
    param.txBuffer = EUSCIB2_txBuffer;
    param.txBufferIndex = &EUSCIB2_txBufferIndex;
    param.txBufferSize = &EUSCIB2_txBufferSize;

    IRQHandler(param);
}
}

/* USING_EUSCI_B2 */
#endif

#ifdef USING_EUSCI_B3
/*
 * Handle everything on EUSCI_B3
 */
extern "C" {
void EUSCIB3_IRQHandler( void ) {
    IRQParam param;
    param.module = EUSCI_B3_BASE;
    param.rxBuffer = EUSCIB3_rxBuffer;
    param.rxBufferIndex = &EUSCIB3_rxBufferIndex;
    param.txBuffer = EUSCIB3_txBuffer;
    param.txBufferIndex = &EUSCIB3_txBufferIndex;
    param.txBufferSize = &EUSCIB3_txBufferSize;

    IRQHandler(param);
}
}

/* USING_EUSCI_B0 */
#endif
