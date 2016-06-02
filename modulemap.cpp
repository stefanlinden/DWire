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

#include "modulemap.h"

ModuleNode * moduleMap = NULL;

/**
 * Register this module in the static moduleMap
 */
void registerModule( DWire * instance ) {
    // Check whether this module has already been registered
    if ( getModuleNode(instance->module) != NULL ) {
        return;
    }

    ModuleNode ** head = &moduleMap;

    ModuleNode * newModule = new ModuleNode( );
    newModule->instance = instance;
    newModule->module = instance->module;

    if ( moduleMap == NULL ) {
        newModule->next = NULL;
        *head = newModule;
    } else {
        newModule->next = *head;
        *head = newModule;
    }
}

/**
 * Unregister this module from the module map
 */
void unregisterModule( DWire * instance ) {

    ModuleNode * current = moduleMap;

    while ( current->next != NULL ) {
        if ( (current->next)->instance == instance ) {
            ModuleNode * newNext = (current->next)->next;
            // Free memory
            delete (current->next);
            current->next = newNext;
            return;
        }
    }
}

/**
 * Get the specified module identifier's container object
 */
ModuleNode * getModuleNode( uint_fast32_t module ) {

    if ( moduleMap == NULL )
        return NULL;

    ModuleNode * current = moduleMap;
    while ( current != NULL ) {
        if ( current->module == module )
            return current;

        current = current->next;
    }
    return NULL;
}

/**
 * Get a reference to the specified module. If it does not exist, then return NULL
 */
DWire * getInstance( uint_fast32_t module ) {

    ModuleNode * node = getModuleNode(module);
    return node->instance;
}
