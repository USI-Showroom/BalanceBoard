/*
 Name:		nRF_BT_Server.ino
 Created:	12/21/2017 2:46:01 AM
 Author:	Rami Baddour
*/

/* The main use of the nRF_BT server is to have one hub point, whenever any data is received on
	one of the interfaces, it is propagated to the other ones.
	
   Please note that the nRF units cannot mutually talk, in other words, if data is received from
	one nRF unit, it will not be broadcasted to the other nRF units, just to the BT and the Serial
	interfaces.

*/
// Include necessary libraries
// the nRF library we used is the one found here: https://github.com/TMRh20/RF24
#include <SoftwareSerial.h>
#include "RF24.h"
#include "printf.h"


/****************** User Config ***************************/
// Set this radio as radio number 1 or 2   (for relay nodes)
int radioNumber = 1;

// select if the node is the server node or not
bool isServer = true;

//define on which pin the relay is connected 
int pinRelay = A0;
// Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 9, 10
RF24 radio(9,10);
// Define the addresses of the nRF24l01 nodes, the first 4 bytes should be equal
byte addresses[] = {
	0xF0F0F0F0E0,	// Broadcast address. all nodes should listen to it
	0xF0F0F0F0E1,	// All remaining addresses are per node
	0xF0F0F0F0E2,	//  They all share the same first 32 bits. 
	0xF0F0F0F0E3,	//	Only the least significant byte should be unique	
	0xF0F0F0F0E4,
	0xF0F0F0F0E5,
	0xF0F0F0F0E6,
	0xF0F0F0F0E7,
	0xF0F0F0F0E8
};
// Define a Software Serial port where the HC-05 Bluetooth is connected
SoftwareSerial mySerial(4,5);

/**********************************************************/

void setup() {
  // put your setup code here, to run once:
  printf_begin();
  mySerial.begin(38400);
  Serial.begin(38400);
  pinMode(pinRelay, OUTPUT);
  
  // Setup the radio
  /*
  We tried to select a set of best configurations that should have good effect on the signal quality
  and range of the nRF units.*/
  radio.begin();
  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(118);
  radio.setCRCLength(RF24_CRC_8);
  radio.setRetries(15,15);
  radio.setAutoAck (1);
  radio.setPayloadSize(1 + sizeof(char));

  radio.printDetails();

  // By default, the Server node will send command to all nodes.
  // We can change the address ofthe writing pipe according to the 
  // received command later.
  if(isServer){
    radio.openWritingPipe(addresses[0]);
  }else{
    radio.openReadingPipe(1,addresses[0]);
    radio.openReadingPipe(2,addresses[radioNumber]);
  }
  
  // Start the radio listening for data
  radio.startListening();
}

void loop() {
  char myChar = ' ';
  /**** Code for the Server node *****/
  if(isServer){
    // check for incomming data on Serial and HC-05  ports in order
    // We expect that only one ofthe three channels will be used at a time.
    if (Serial.available())
    {
      myChar = Serial.read();
    }
    if (mySerial.available())
    {
      myChar = mySerial.read();
    }
    if (myChar != ' ')  // if a new character is received
    {
      radio.stopListening();  // First, stop listening so we can change the writing pipe address and send data
      switch (myChar){
        case 'a':
        case 'b':
          radio.openWritingPipe(addresses[0]);  // broadcast to all nodes
          radio.write( &myChar, sizeof(char) );
          break;
        case 'c':
        case 'd':
          radio.openWritingPipe(addresses[1]);  // send to the first relay
          myChar = myChar - 2;  // reset the character back to (a, b) pair
          radio.write( &myChar, sizeof(char) );
          break;
        case 'e':
        case 'f':
          radio.openWritingPipe(addresses[2]);  // send to the second relay
          myChar = myChar - 4;  // reset the character back to (a, b) pair
          radio.write( &myChar, sizeof(char) );
          break;
      }
      radio.startListening(); // Now, continue listening
    }
  }
  /**** Code for the Relay nodes *****/
  else{
    // check for incomming data on Serial, HC-05 and nRF24L01+ ports in order
    // We expect that only one ofthe three channels will be used at a time.
    if (Serial.available())
    {
      myChar = Serial.read();
    }
    if (mySerial.available())
    {
      myChar = mySerial.read();
    }
    if(radio.available()){
      radio.read( &myChar, sizeof(char) );             // Get the payload
    }
    if (myChar == 'a')
    {
      digitalWrite(pinRelay, 1);  // switch on the relay
    }
    else if (myChar == 'b')
    {
      digitalWrite (pinRelay, 0); // switch off the relay
    }
    else if  (myChar != ' ')
    {
    }
  }
  delay(50);
}
