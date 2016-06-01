#include "DWire.h"

DWire wire( EUSCI_B1_BASE );
uint8_t i;

void setup()
{
  // Initialise the serial (COM) connection
  //Serial.begin(9600);
  delay(100);
  //Serial.println("Ready as slave...");

  // Start the module as a slave on address 0x48
  wire.begin(0x42);
  wire.onReceive(handleReceive);
  wire.onRequest(handleRequest);
}

void loop()
{
  // There's no real function here
  delay(100);
}

/**
 * Receive interrupt handler
 * This interrupt is triggered by DWire when a full frame has been received
 * (i.e. after receiving a STOP)
 */
void handleReceive( uint8_t numBytes ) {

  //Serial.print("Got a message: ");

  // Get the rx buffer's contents from the DWire object
  for ( int i = 0; i < numBytes; i++ ) {
    uint8_t byte = wire.read( );

    // Print the contents of the received byte
    //Serial.print(byte);
  }

    // End the line in preparation of the next receive event
   // Serial.println( );
}

/**
 * Request interrupt handler
 * This request is called on a read request from a master node.
 *
 */
void handleRequest( void ) {
    //Serial.println("Sending six bytes.");
    wire.write(0x41);
    wire.write(0x42);
    wire.write(0x43);
    wire.write(0x44);
    wire.write(0x45);
    wire.write(0x46);
}

