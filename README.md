# DWire (Delft Wire)
A simple library for handling I2C on the MSP432, trying to be similar to Energia's Wire library (originally from Arduino) but providing more advanced features.

## Introduction and Features

Although powerful, the Wire library included in [Energia](http://energia.nu/) (an Arduino port for TI microcontrollers) has been found to have certain limitations in its use. Delft Wire, or DWire for short (named after the Delft University of Technology), provides Energia developers looking for more advanced functionality when using the I2C bus standard in their projects.

Major features include:
- Full hardware-driven I2C (via the eUSCI modules): providing standard mode, full speed mode and fast mode.
- It is possible to select other eUSCI modules. 
- Ability to use multiple eUSCI modules at the same time.
- Full slave support: it is possible to run the microcontroller as a slave.
- Nearly identical interface as Wire's interface.
- Repeated starts are supported, both as Master and Slave.

## Installation

The library can directly be used in Energia. Simply clone the repository or download the zip file, placing the root directory of the repository in your Energia user folder's 'libraries' folder. E.g. in Windows, this is typically found in **C:\Documents\Energia\libraries**. This library uses `driverlib`, which should come with the standard Energia installation. Nevertheless, make sure this library is accessible to the compiler.

DWire should be able to compile with all generic toolchains for the MSP432. For the moment, make sure the `EUSCIBx_IRQHandler` interrupt handler is registered in the main interrupt vector. For example, when using Code Composer Studio, this may be done in the auto-generated `startup_msp432p401r_ccs.c` file in the main project folder. Make sure the main `driverlib` folder is included in the compiler's include path and that the library is linked to correctly.
