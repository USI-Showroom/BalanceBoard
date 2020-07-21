// ORIGINAL CODE MODIFIED FROM:
//  I2C device class (I2Cdev) demonstration Arduino sketch for MPU6050 class using DMP (MotionApps v2.0)
// I2Cdev and MPU6050 must be installed as libraries, or else the .cpp/.h files
//   for both classes must be in the include path of your project

/* Change the settings here to turn on(1)/off(0) the use of the following features:
	- USE_NRF24L01_COMM		: nRF24L01+ communication (includes the RF24 library).
	- USE_SERIAL_BT_COMM	: Bluetooth communication (includes SoftwareSerial library).
	- USE_LED_PIXEL_STRIP	: LED strip on the board  (includes Adafruit_NeoPixel library).
	- USE_CYCLING_COLOR_LED : LED Strip will cycle in rainbow color when not used for some idle time
	- USE_DEBUG_INFO		: Debugging information sent to Serial port
The choice of which features to enable will affect the code size. Probably, enabling
all features will exceed the available ROM space on the pro-mini board.

UPDATE 2017.02.20 :
	After the latest code refinement, all features can be enabled and fit on the Arduino Pro Mini memory :)
*/
#define USE_NRF24L01_COMM			1
#define USE_SERIAL_BT_COMM			1
#define USE_LED_PIXEL_STRIP			1
#define USE_CYCLING_COLOR_LED		1
#define USE_FADING_COLOR_LED		1
#define USE_DEBUG_INFO				1
#define BOARD_NUM					1

/* Board-based settings
	These are arrays of default settings, depending on the particular board number

*/
#if BOARD_NUM == 1
#define NODE_DEF_OFFSETS			{ -2014, 453, 2374, 68, -39, 21 }
#define LED_INIT_COUNT              1
#define LED_PER_SEC                 1
#define LED_STRIP_COUNT             37
#define NRF_PIPE_ADDR				0xF0F0F0F0E1
#elif BOARD_NUM == 2
#define NODE_DEF_OFFSETS			{ 11, -2179, 1326, 115, -16, 8 }
#define LED_INIT_COUNT              2
#define LED_PER_SEC                 2
#define LED_STRIP_COUNT             74
#define NRF_PIPE_ADDR				0xF0F0F0F0E2
#elif BOARD_NUM == 3
#define NODE_DEF_OFFSETS			{ 2003, -820, 1456, 112, -1, 19 }
#define LED_INIT_COUNT              2
#define LED_PER_SEC                 2
#define LED_STRIP_COUNT             74
#define NRF_PIPE_ADDR				0xF0F0F0F0E3
#endif

// Communication settings
#define SERIAL_BAUD_RATE			115200

// Scoring settings
#define NUM_OF_CIRCLES				6	// This is also the maximum score
#define CENTRAL_CIRCLE_RADIUS		50	

// Timing settings (in seconds) 
//    NOTE: you can use the excel file to generate accurate numbers
#define TIMER_FIRST_CALIBRATION     36
#define TIMER_LATER_CALIBRATION     6
#define TIMER_DOWN                  3
#define TIMER_ATTEMPT               18
#define TIMER_END                   6
#define TIMER_IDLE					6
																 
// led strip setup goes here																	  
//    NOTE: you can use the excel file to generate accurate numbers
#if USE_LED_PIXEL_STRIP
#include "Adafruit_NeoPixel.h"
#define LED_STRIP_PIN               6
#define LED_FADE_STEPS				8
Adafruit_NeoPixel strip = Adafruit_NeoPixel(LED_STRIP_COUNT, LED_STRIP_PIN, NEO_GRB + NEO_KHZ800);
#endif

// nRF24L01+ setup goes here
#if USE_NRF24L01_COMM
#include "RF24.h"
#define NRF24_PIN_CE				9
#define NRF24_PIN_CSN				10
#define MAX_MSG_LEN					100
RF24 radio(NRF24_PIN_CE, NRF24_PIN_CSN);
const uint64_t pipe = NRF_PIPE_ADDR;
char tmpMsg[MAX_MSG_LEN]; 
#endif

//bluetooth HC-05 setup goes here
#if USE_SERIAL_BT_COMM
#include "SoftwareSerial.h"
#define SW_SERIAL_RX				4
#define SW_SERIAL_TX				5
SoftwareSerial mySerial(SW_SERIAL_RX, SW_SERIAL_TX); // (RX, TX)
#endif

													 // Code for debugging info macro
#if USE_DEBUG_INFO
#define RGB_DEBUG(XYZ)(Serial.println(XYZ))
#else
#define RGB_DEBUG(XYZ)
#endif

// MPU6050 setup goes here
// The values in MPU6050_OFFSETS should be re-generated for each board.
//		Note: Please run the 'MPU6050_calibration.ino' code to generate these values
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
MPU6050 mpu;
const int32_t MPU6050_OFFSETS[] = NODE_DEF_OFFSETS; // generate using the calibration code
bool dmpReady = false;  // set true if DMP init was successful
uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
uint16_t packetSize;    // expected DMP pafwcket size (default is 42 bytes)
uint16_t fifoCount;     // count of all bytes currently in FIFO
uint8_t fifoBuffer[64]; // FIFO storage buffer
Quaternion quater;      // [w, x, y, z]         quaternion container
VectorFloat gravity;    // [x, y, z]            gravity vector
float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector

unsigned long startTick;
unsigned long currTick;
uint8_t currFade;

char state = 'c';
int8_t ticksToGo;
int8_t timerCalibration;
float score = 0.0;;
float sample = 0.0;;
float best = 0.0;;
float last = 0.0;;
float mx = 0.0;;
float my = 0.0;;
float mxnew = 0.0;;
float mynew = 0.0;;
float calX = 0.0;;
float calY = 0.0;;
float calSample = 1;
int incomingByte = 0;
int sendSerial = 0;
int sendSerialCount = 1;


void countingStrip(int counterGroup, int rRGB, int gRGB, int bRGB)
{
	uint8_t ledgroup, ledsOff, ledsOn, ledCurr, _i, _j;
	ledgroup = (LED_STRIP_COUNT- LED_INIT_COUNT) / (counterGroup);
	ledsOff = floor(ledgroup * (1.0 / 2.0));
	ledsOn = ceil(ledgroup * (1.0 / 2.0));
	if (ledsOn < 1) ledsOn = 1;
	ledCurr = 0;
	strip.setPixelColor(0, rRGB, gRGB, bRGB);

	  for (_i=0; _i < LED_INIT_COUNT; _i++)
	    strip.setPixelColor(ledCurr++, rRGB, gRGB, bRGB);

	for (_i = 0; _i < ticksToGo - 1; _i++)
	{
		for (_j = 0; _j < ledsOff; _j++)
			strip.setPixelColor(ledCurr++, 0x00, 0x00, 0x00);
		for (_j = 0; _j < ledsOn; _j++)
			strip.setPixelColor(ledCurr++, rRGB, gRGB, bRGB);
	}

#if USE_FADING_COLOR_LED
	if (ticksToGo > 0) {
		int fadeRatio = currFade;
		for (_j = 0; _j < ledsOff; _j++)
			strip.setPixelColor(ledCurr++, 0x00, 0x00, 0x00);
		for (_j = 0; _j < ledsOn; _j++)
		    strip.setPixelColor(ledCurr++, rRGB / (4 * fadeRatio), gRGB / (4 * fadeRatio), bRGB / (4 * fadeRatio));
	}
#endif
	while (ledCurr < LED_STRIP_COUNT)
		strip.setPixelColor(ledCurr++, 0x00, 0x00, 0x00);

}

void lightUpStrip() {
	if (state == 'c') {
		countingStrip(timerCalibration * LED_PER_SEC, 0xFF, 0x00, 0x00);
	}
	else if (state == 'w') {
		for (int _i = 0; _i < LED_INIT_COUNT; _i++)
			strip.setPixelColor(_i, 0x00, 0xFF, 0x00);
	}
#if USE_CYCLING_COLOR_LED
	else if (state == 'r') {
		rainbowCycle();
	}
#endif // USE_CYCLING_COLOR_LED
	else if (state == 'd') {
		countingStrip(TIMER_DOWN * LED_PER_SEC, 0x00, 0xFF, 0xFF);
	}
	else if (state == 't') {
		countingStrip(TIMER_ATTEMPT * LED_PER_SEC, 0x00, 0x00, 0xFF);
	}
	else if (state == 'e') {
		float actualScore = score / sample;
		uint8_t ledCount = (((LED_STRIP_COUNT - LED_INIT_COUNT)*1.0)*(actualScore - 1.0)) / 5.0;
		for (int _ind = 0; _ind<ledCount + 1; _ind++) {
			strip.setPixelColor(_ind, 0x00, 0xFF, 0x00);
		}
		for (int _ind = 1; _ind <= actualScore; _ind++) {
			for (int _j = 0; _j < LED_PER_SEC; _j++)
				strip.setPixelColor(_ind * ((LED_STRIP_COUNT - LED_INIT_COUNT) / 6) + _j, 0x00, 0x00, 0xFF);
		}
	}
	else if (state == 's') {
		countingStrip(TIMER_END * LED_PER_SEC, 0xFF, 0xFF, 0x00);
	}
	strip.show();
}

#if  USE_CYCLING_COLOR_LED
void rainbowCycle() {
	uint16_t i, j;
	static uint8_t firstColorInd = 0;
	firstColorInd = (++firstColorInd) % 255;

	for (i = 0; i< LED_STRIP_COUNT; i++) {
		strip.setPixelColor(i, Wheel(((i * 256 / LED_STRIP_COUNT) + firstColorInd) & 255));
	}
	strip.show();
}
uint32_t Wheel(byte WheelPos) {
	WheelPos = 255 - WheelPos;
	if (WheelPos < 85) {
		return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
	}
	if (WheelPos < 170) {
		WheelPos -= 85;
		return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
	}
	WheelPos -= 170;
	return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
#endif // USE_CYCLING_COLOR_LED

// ================================================================
// ===               INTERRUPT DETECTION ROUTINE                ===
// ================================================================
volatile bool mpuInterrupt = false;     // indicates whether MPU interrupt pin has gone high
// the following interrupt handler will be attached to Arduino interrupt pin 
void dmpDataReady() {
	mpuInterrupt = true;
}

// ================================================================
// ===                      INITIAL SETUP                       ===
// ================================================================
void setup() {
	// join I2C bus (I2Cdev library doesn't do this automatically)
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
	Wire.begin();
	TWBR = 12; // 400kHz I2C clock (200kHz if CPU is 8MHz)
#elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
	Fastwire::setup(800, true);
#endif
	// initialize serial communication
	Serial.begin(SERIAL_BAUD_RATE);
#if USE_SERIAL_BT_COMM
	mySerial.begin(115200);
#endif
	//    while (! Serial); // wait for Leonardo enumeration, others continue immediately
	//    while (! //mySerial);

	//Initializing Software seria //mySerial for bluetooth comunication

#if USE_NRF24L01_COMM
	radio.begin();
	radio.setChannel(2);
	radio.setRetries(2, 15);
	radio.setPALevel(RF24_PA_HIGH);
	radio.setDataRate(RF24_1MBPS);
	radio.setAutoAck(0);
	radio.setPayloadSize(MAX_MSG_LEN);
	radio.openWritingPipe(pipe);
	radio.printDetails();
#endif

	RGB_DEBUG(F("Initializing I2C devices..."));

#	// initialize MPU6050 device
	mpu.initialize();


	// supply your own gyro offsets here, scaled for min sensitivity
	mpu.setXAccelOffset(MPU6050_OFFSETS[0]);
	mpu.setYAccelOffset(MPU6050_OFFSETS[1]);
	mpu.setZAccelOffset(MPU6050_OFFSETS[2]);
	mpu.setXGyroOffset(MPU6050_OFFSETS[3]);
	mpu.setYGyroOffset(MPU6050_OFFSETS[4]);
	mpu.setZGyroOffset(MPU6050_OFFSETS[5]);

	// Initialize the internal DMP processor on the MPU6050 and 
	//		make sure it worked (returns 0 if so)
	devStatus = mpu.dmpInitialize();
	if (devStatus == 0) { // DMP is working
		// turn on the DMP, now that it's ready
		mpu.setDMPEnabled(true);

		// enable Arduino interrupt detection
		attachInterrupt(0, dmpDataReady, RISING);
		mpuIntStatus = mpu.getIntStatus();

		// set DMP Ready flag so the main loop() function knows it's okay to use it
		dmpReady = true;

		// get expected DMP packet size for later comparison
		packetSize = mpu.dmpGetFIFOPacketSize();
	}
	else { // DMP is not working :(
		// ERROR! the code indicates:
		// 1 = initial memory load failed
		// 2 = DMP configuration updates failed
		// (if it's going to break, usually the code will be 1)
		RGB_DEBUG(F("DMP Initialization failed (code "));
		RGB_DEBUG(devStatus);
		RGB_DEBUG(F(")"));
		//mySerial.print(F("DMP Initialization failed (code "));
		//mySerial.print(devStatus);
		//mySerial.println(F(")"));
	}

	// configure LED for output

	startTick = currTick = millis() / (1000 / LED_PER_SEC);
	ticksToGo = TIMER_FIRST_CALIBRATION * LED_PER_SEC;
	timerCalibration = TIMER_FIRST_CALIBRATION;
	strip.begin();

	//	inputString.reserve(200);
}

// ================================================================
// ===                    MAIN PROGRAM LOOP                     ===
// ================================================================
void loop() {

	//if (!dmpReady) return; //don't do anything if the MPU6050 does not work
	while (!mpuInterrupt && fifoCount < packetSize) {

	}
	//get the current tick
	currTick = millis() / (1000 / LED_PER_SEC);
	currFade = ((millis() - (currTick * 1000 / LED_PER_SEC)) / 100)+1;
	/*States explained:
	* 'c':'Calibration' state, Will have two different timings depending whether it is the first calibration or not
	* 'w':'Waiting' state, After calibration, wait until the user steps-up on the board
	* 'd':'Counting Down' state, After the user steps up, small count-down before the balance is evaluated
	* 'r':'Rainbow' state, This will cycle colors of all leads in Rainbow-color loop
	* 't':'Trial' state, during this state, the balance (i.e. the IMU readings) will be recorded and averaged
	* 'e':'End' state, After the trial ends, display the result for some time before the next trial
	* 's':'Serial Input' state, TODO: allow remote configuration via serial commands
	*/
	if (state == 'c') {			// Calibration
		ticksToGo = (timerCalibration * LED_PER_SEC) - (currTick - startTick);
		if (ticksToGo < 5) {
			//todo iniziare a salvare i dati per la calibrazione
			calX += mx;
			calY += my;
			calSample += 1.0;
		}
		if (ticksToGo < 0) {

			state = 'w';
			timerCalibration = TIMER_LATER_CALIBRATION;
			startTick = currTick;
		}
	}
	else if (state == 'w') {	// Waiting to start the trial
		if (sqrt((mx*mx) + (my*my)) <= (NUM_OF_CIRCLES * CENTRAL_CIRCLE_RADIUS)) {
			if ((NUM_OF_CIRCLES - (int)sqrt((mx*mx) + (my*my)) / CENTRAL_CIRCLE_RADIUS) <= 3) {
				state = 'd';
				startTick = millis() / (1000 / LED_PER_SEC);
			}
		}
#if USE_CYCLING_COLOR_LED
		ticksToGo = (TIMER_IDLE * LED_PER_SEC) - (currTick - startTick);
		if (ticksToGo < 0) {
			state = 'r';
			startTick = currTick;
		}
	}
	else if (state == 'r') {	// Waiting to start the trial
		if (sqrt((mx*mx) + (my*my)) <= (NUM_OF_CIRCLES * CENTRAL_CIRCLE_RADIUS)) {
			if ((NUM_OF_CIRCLES - (int)sqrt((mx*mx) + (my*my)) / CENTRAL_CIRCLE_RADIUS) <= 3) {
				state = 'd';
				startTick = millis() / (1000 / LED_PER_SEC);
			}
		}
#endif // USE_CYCLING_COLOR_LED
	}
	else if (state == 'd') {	// counting Down
		ticksToGo = (TIMER_DOWN * LED_PER_SEC) - (currTick - startTick);
		if (ticksToGo<0) {
			state = 't';
			score += NUM_OF_CIRCLES - (int)sqrt((mx*mx) + (my*my)) / CENTRAL_CIRCLE_RADIUS;
			sample += 1.0;
			startTick = currTick;
		}
	}
	else if (state == 't') {	// Trial
		ticksToGo = (TIMER_ATTEMPT * LED_PER_SEC) - (currTick - startTick);
		sample += 1.0;
		float tmpF = sqrt((mx*mx) + (my*my));
		if (tmpF <= (NUM_OF_CIRCLES * CENTRAL_CIRCLE_RADIUS)) {
			score += NUM_OF_CIRCLES - (int)tmpF / CENTRAL_CIRCLE_RADIUS;
		}
		if (ticksToGo<0) {
			state = 'e';
			if (score>best) {
				best = score / sample;
			}
			last = score / sample;
			startTick = currTick;
		}
	}
	else if (state == 'e') {	//Trial end/Score display
		ticksToGo = (TIMER_END * LED_PER_SEC) - (currTick - startTick);

		if (ticksToGo<0) {
			calX = 0.0;
			calY = 0.0;
			calSample = 1.0;
			score = 0.0;
			sample = 0.0;
			state = 's';
			timerCalibration = TIMER_LATER_CALIBRATION;
			startTick = currTick;
		}
	}
	else if (state == 's') {	// TODO: Serial commands
		//      while (mySerial.available()) {
		//            // get the new byte:
		//            char inChar = (char) mySerial.read();
		//            // add it to the inputString:
		//            inputString += inChar;
		//            // if the incoming character is a newline, set a flag
		//            // so the main loop can do something about it:
		state = 'c';
		startTick = currTick;
	}

	mpuInterrupt = false; //reset the interrupt flag
	mpuIntStatus = mpu.getIntStatus(); //reset the INT_STATUS byte

	fifoCount = mpu.getFIFOCount(); // get current FIFO count

	if ((mpuIntStatus & 0x10) || fifoCount == 1024) {  // check for overflow that should not happen (if it does it means my code it's to slow)

		mpu.resetFIFO(); //reset
	}
	else if (mpuIntStatus & 0x02) { //if my code is fast enought i check here for the interrupt 
									//    while (fifoCount < packetSize) //wait for correct data lenght (short wait)
		fifoCount = mpu.getFIFOCount();
		while (fifoCount >= packetSize)
		{
			mpu.getFIFOBytes(fifoBuffer, packetSize); // read a packet from FIFO
			fifoCount -= packetSize; // update FIFO count, this let us immediatly read more without waiting for an interrupt.
		}
		{

			mpu.dmpGetQuaternion(&quater, fifoBuffer);
			mpu.dmpGetGravity(&gravity, &quater);
			mpu.dmpGetYawPitchRoll(ypr, &quater, &gravity);
			mxnew = ypr[2] * 180 / M_PI;
			mynew = ypr[1] * 180 / M_PI;
			mynew = mynew*-1.0;
			mxnew = mxnew * 13.50;
			mynew = mynew * 13.50;
			if ((state == 'c')) {
				mx = mxnew - (calX / calSample);
				my = mynew - (calY / calSample);
			}
			else if (abs(mxnew - mx)<100 && abs(mynew - my)<100) {
				mx = mxnew - (calX / calSample);
				my = mynew - (calY / calSample);
			}
			else {
				mx = mxnew - (calX / calSample);
				my = mynew - (calY / calSample);
				//              mx = (mx*0.9)+(mxnew*0.1)- (calX/calSample);
				//              my = (my*0.9)+(mynew*0.1)- (calY/calSample);  
			}
			//mySerial.flush();
			if ((state == 'c') && ((++sendSerial) / sendSerialCount)) {

				sendSerial = 0;
				Serial.println(String(String(ticksToGo / LED_PER_SEC) + ",0.0,c"));
#if USE_SERIAL_BT_COMM
				mySerial.println(String(String(ticksToGo / LED_PER_SEC) + ",0.0,c"));
#endif
#if USE_NRF24L01_COMM
				String(String(ticksToGo / LED_PER_SEC) + ",0.0,c").toCharArray(tmpMsg, MAX_MSG_LEN);
				radio.write(&tmpMsg, strlen(tmpMsg));
#endif
			}
			else if ((state == 'w') && ((++sendSerial) / sendSerialCount)) {
				sendSerial = 0;
				Serial.println(String("0.0,0.0,w"));
#if USE_SERIAL_BT_COMM
				mySerial.println(String("0.0,0.0,w"));
#endif

#if USE_NRF24L01_COMM
				String("0.0,0.0,w").toCharArray(tmpMsg, MAX_MSG_LEN);
				radio.write(&tmpMsg, strlen(tmpMsg));
#endif

			}
			else if ((state == 'd') && ((++sendSerial) / sendSerialCount)) {
				sendSerial = 0;
				Serial.println(String(String(ticksToGo / LED_PER_SEC) + ",0.0,d"));
#if USE_SERIAL_BT_COMM
				mySerial.println(String(String(ticksToGo / LED_PER_SEC) + ",0.0,d"));
#endif

#if USE_NRF24L01_COMM
				String(String(ticksToGo / LED_PER_SEC) + ",0.0,d").toCharArray(tmpMsg, MAX_MSG_LEN);
				radio.write(&tmpMsg, strlen(tmpMsg));
#endif
			}
			else if ((state == 't') && ((++sendSerial) / sendSerialCount)) {
				sendSerial = 0;
				Serial.println(String(String(int(mx)) + "," + String(int(my)) + ",a," + String(int(ticksToGo / LED_PER_SEC)) + "," + String(score / sample)));
#if USE_SERIAL_BT_COMM
				mySerial.println(String(String(int(mx)) + "," + String(int(my)) + ",a," + String(int(ticksToGo / LED_PER_SEC)) + "," + String(score / sample)));
#endif

				//change score, score to mx,my
#if USE_NRF24L01_COMM
				String(String(int(mx)) + "," + String(int(my)) + ",a," + String(int(ticksToGo / LED_PER_SEC)) + "," + String(score / sample)).toCharArray(tmpMsg, MAX_MSG_LEN);
				radio.write(&tmpMsg, strlen(tmpMsg));
#endif
			}
			else if ((state == 'e') && ((++sendSerial) / sendSerialCount)) {
				sendSerial = 0;
				Serial.println(String(String(best) + "," + String(last) + ",e"));
#if USE_SERIAL_BT_COMM
				mySerial.println(String(String(best) + "," + String(last) + ",e"));
#endif
#if USE_NRF24L01_COMM
				String(String(best) + "," + String(last) + ",e").toCharArray(tmpMsg, MAX_MSG_LEN);
				radio.write(&tmpMsg, strlen(tmpMsg));
#endif
			}
			else if ((state == 's') && ((++sendSerial) / sendSerialCount)) {
				sendSerial = 0;
				Serial.println(String("0.0,0.0,s"));
#if USE_SERIAL_BT_COMM
				mySerial.println(String("0.0,0.0,s"));
#endif
#if USE_NRF24L01_COMM
				String("0.0,0.0,s").toCharArray(tmpMsg, MAX_MSG_LEN);
				radio.write(&tmpMsg, strlen(tmpMsg));
#endif
			}
			//      fifoCount = mpu.getFIFOCount();
		}
	}
	lightUpStrip();
}






