/*
 * modulemap.h
 *
 *  Created on: 30 May 2016
 *      Author: stefa_000
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
