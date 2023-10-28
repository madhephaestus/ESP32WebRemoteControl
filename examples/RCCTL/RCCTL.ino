#include <Arduino.h>
#include <WiFi.h> 
#include <ESP32WebRemoteControl.h>
#include <ESP32Servo.h>
#include <Esp32WifiManager.h>
#include "wifi/WifiManager.h"
#include "WebPage.h"
#include <WiiChuck.h>

Servo m1;
Servo m2;
Servo m3;
Servo m4;
WebPage control_page;

WifiManager manager;
Accessory nunchuck1;
/*
 * This is the standard setup function that is called when the ESP32 is rebooted
 * It is used to initialize anything that needs to be done once.
 * In this example, it sets the Serial Console speed, initializes the web server,
 * sets up some web page buttons, resets some timers, and sets the initial state
 * the robot should start in
 *
 * HOWTO enter passwords:
 *
 * The Detailed documentation is here: https://github.com/madhephaestus/Esp32WifiManager
 *
 * You will use the Serial Monitor to enter the name of the network and the password to use.
 * The ESP32 will store them for later.
 * First type the SSID you want and hit send
 * It will prompt you for a password, type that and hit send
 *
 * IN LAB use:
 * In the lab, you will want to use AP mode. To set the AP, type AP:myNetName
 * to set myNetName as the AP mode and hit send
 * the ESP will prompt you for a password to use, enter it and hit send
 *
 * The ESP will default to trying to connect to a network, then fail over to AP mode
 *
 * To make the ESP use AP mode by default on boot, change the line below manager.setup();
 * to manager.setupAP();
 *
 * To access the RC control interface for Station mode you will watch the serial monitor as the
 * ESP boots, it will print out the IP address. Enter that address in your computer or phones web browser.
 * Make sure your ESP and computer or phone are on the same network.
 *
 * To access teh RC Control interface in AP mode, connect to the ESP with either you phone or laptop
 * then open in a browser on that device and go to:
 *
 * http://192.168.4.1
 *
 * to access the control website.
 *
 * NOTE you can use this class in your final project code to visualize the state of your system while running wirelessly.
 */
int inc = 0;
int timerTime = 0;
void setup() {
	ESP32PWM::allocateTimer(0);  // Used by servos
	m1.attach(14);
	m2.attach(25);
	m3.attach(26);
  m4.attach(12);

	m1.write(90);
	m2.write(90);
	m3.write(90);
  m4.write(90);
	//manager.setup();  // Connect to an infrastructure network first, then fail over to AP mode
	manager.setupAP(); // Launch AP mode first, then fail over to connecting to a station
	 while (manager.getState() != Connected) {
	 	manager.loop();
	 	delay(10);

	}
	delay(100);
	Serial.println("Wifi Connected!");
	//control_page.initalize();  // Init UI after everything else.
	Serial.println("Starting the Web Control");
	timerTime=millis()+100;
  nunchuck1.begin();
	if (nunchuck1.type == Unknown) {
		/** If the device isn't auto-detected, set the type explicatly
		 * 	NUNCHUCK,
		 WIICLASSIC,
		 GuitarHeroController,
		 GuitarHeroWorldTourDrums,
		 DrumController,
		 DrawsomeTablet,
		 Turntable
		 */
		nunchuck1.type = NUNCHUCK;
	}
}

/*
 * this is the state machine.
 * You can add additional states as desired. The switch statement will execute
 * the correct code depending on the state the robot is in currently.
 * For states that require timing, like turning and straight, they use a timer
 * that is zeroed when the state begins. It is compared with the number of
 * milliseconds the robot should reamain in that state.
 */
 float fmap(float x, float in_min, float in_max, float out_min, float out_max) {
    const float run = in_max - in_min;
    if(run == 0){
        log_e("map(): Invalid input range, min == max");
        return -1; // AVR returns -1, SAM returns 0
    }
    const float rise = out_max - out_min;
    const float delta = x - in_min;
    return (delta * rise) / run + out_min;
}
void runStateMachine() {

	float x =0;// control_page.getJoystickX();
	float y = 0;//control_page.getJoystickY();

	float slide =0;
  nunchuck1.readData();    // Read inputs and update maps
	// nunchuck1.printInputs(); // Print all inputs
	// for (int i = 0; i < WII_VALUES_ARRAY_SIZE; i++) {
	// 	Serial.println(
	// 			"Controller Val " + String(i) + " = "
	// 					+ String((uint8_t) nunchuck1.values[i]));
      
	// }
  long buttons = 0;
  long mouth = 40;
  if(nunchuck1.values[10]>0){
    buttons=90;
  }
  if(nunchuck1.values[11]>0){
    mouth=120;
  }
  slide = fmap(buttons,0,255,-1.0,1.0);
  x= -fmap(nunchuck1.values[0],0,255,-1.0,1.0);
  y= -fmap(nunchuck1.values[1],0,255,-1.0,1.0);
  //slide = control_page.getSliderValue(0);
	int offset = slide * 20 ;
  int scale =30;
  //Serial.println(String(offset));
	int m1val = offset + (90) +(x*-scale);
	int m2val = offset + (90)+(x*scale*cos(-120))+(y*scale*sin(-120));
	int m3val = offset + (90)+(x*scale*cos(-120))+(y*scale*sin(120));
	m1.write(m1val);
	m2.write(m2val);
	m3.write(m3val);
  m4.write(mouth);
}

/*
 * This function updates all the dashboard status that you would like to see
 * displayed periodically on the web page. You are free to add more values
 * to be displayed to help debug your robot program by calling the
 * "setValue" function with a name and a value.
 */

uint32_t packet_old = 0;
void updateDashboard() {
	// This writes values to the dashboard area at the bottom of the web page
	if ((timerTime + 100) > millis()) {
		//Serial.println("Update dashboard");
		control_page.setValue("Simple Counter", inc++);
		//if(control_page.getJoystickMagnitude()>0.1)
		//Serial.println("Joystick angle="+String(control_page.getJoystickAngle())+
		//		" magnitude="+String(control_page.getJoystickMagnitude())+
		//		" x="+String(control_page.getJoystickX())+
		//						" y="+String(control_page.getJoystickY()) +
		//						" slider="+String(control_page.getSliderValue(0)));

		control_page.setValue("packets from Web to ESP",
				control_page.rxPacketCount);

		control_page.setValue("Simple Counter", inc++);
		control_page.setValue("packets from Web to ESP",
				control_page.rxPacketCount);
		control_page.setValue("packets to Web from ESP",
				control_page.txPacketCount);

		timerTime = millis();
	}
}

/*
 * The main loop for the program. The loop function is repeatedly called
 * once the ESP32 is started. In here we run the state machine, update the
 * dashboard data, and handle any web server requests.
 */

void loop() {

	manager.loop();
	runStateMachine();                    // do a pass through the state machine
	if (manager.getState() == Connected)  // only update if WiFi is up
		updateDashboard();                  // update the dashboard values
	delay(1);
}
