/*
 * Copyright (c) 2016 by Stefan van der Linden <spvdlinden@gmail.com>
 *
 * DWire: a library to provide full hardware-driven I2C functionality
 * to the TI MSP432 family of microcontrollers. It is possible to use
 * this library in Energia (the Arduino port for MSP microcontrollers)
 * or in other toolchains.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License 
 * version 3, both as published by the Free Software Foundation.
 *
 */

#ifndef INCLUDE_MODULEMAP_H_
#define INCLUDE_MODULEMAP_H_

#ifndef NULL
#define NULL 0
#endif

#include  "DWire.h"


/**
 * Define the main type
 */
class ModuleNode {
public:
    uint32_t module;
    DWire * instance;
    struct ModuleNode * next;
};

/**
 * Register the specified module
 */
void registerModule( DWire * );

/**
 * Unregister the specified module
 */
void unregisterModule( DWire * );

/**
 * Get the specified module identifier's container object
 */
ModuleNode * getModuleNode( uint_fast32_t );

/**
 * Get the DWire instance corresponding to the specified identifier
 */
DWire * getInstance( uint_fast32_t );

extern ModuleNode * moduleMap;

#endif /* INCLUDE_MODULEMAP_H_ */
