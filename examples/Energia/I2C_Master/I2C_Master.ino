#include "DWire.h"

DWire wire( EUSCI_B0_BASE );
uint8_t i;

void setup( ) {
    // put your setup code here, to run once:
    wire.begin( );
}

void loop( ) {
    // put your main code here, to run repeatedly:
    // Start a frame
    wire.beginTransmission(0x42);

    // Write three bytes
    wire.write(i);
    wire.write(i + 1);
    wire.write(i + 2);
    wire.write(i + 3);
    i++;

    // Mark the frame as finished. This causes DWire to transmit the buffer's contents
    wire.endTransmission( );

    delay(200);

    // Do a request
    wire.requestFrom(0x42, 4);
    delay(400);
}
