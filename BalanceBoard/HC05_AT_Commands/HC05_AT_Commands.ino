/*
 Name:		HC05_AT_Commands.ino
 Created:	12/21/2017 2:46:01 AM
 Author:	Rami Baddour
*/
#include <SoftwareSerial.h>

SoftwareSerial mySerial(4, 5); // RX, TX

void setup() {
	Serial.begin(9600);
	Serial.println("Enter AT commands:");
  mySerial.begin(38400);
}

void loop()
{
	if (mySerial.available())
		Serial.write(mySerial.read());
	if (Serial.available())
		mySerial.write(Serial.read());
}
