#include <Arduino.h>
#include <WebServer.h>
#include <HTTP_Method.h>
#include "WebPage.h"
#include "static/static.h"

#include <ESPAsyncWebServer/ESPAsyncWebServer.h>

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

AsyncWebServer server(80);
AsyncWebSocket ws("/test");
static WebPage *thisPage;
static char stringBuffer[200];
//static uint8_t buffer[labelbuflen];
//const String updtime="Uptime";
//const String js(nipplejs_min_js);
//const String myHTML(index_html);

static bool lockOutSending = false;

long timeSinceLastSend =0;



void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  uint32_t *asInt = (uint32_t *)data;
  float    *asFloat = (float *)data;
  if(type == WS_EVT_CONNECT){
    //Serial.println("Websocket client connection received");
	 thisPage->markAllDirty();
	 thisPage->updatePID=true;
  } else if(type == WS_EVT_DISCONNECT){
    //Serial.println("Client disconnected");

  } else if(type == WS_EVT_DATA){
	  thisPage->rxPacketCount++;
	  if (len<3) {
		  //Serial.println("Packet too short!");
		  return;
	  }
	  uint32_t command = asInt[0];
	  /*
    * Data Format
    * 4B: Message Type
	*		0x10 (16)	Value Update
	*  	  	4B: value index
	*			4B: value data
	* 		0x11 (17)	Console Data
	* 			4B:	length
	* 			*B: data
	*
	*
	* 		0x1d (29)	Bulk Label Update
	* 			4B:	Number of Labels in this update
	* 			4B: Start of string data
	* 			[repeated next 12B block for each label]
	* 			4B: Index to update
	* 			4B: String Offset in packet
	* 			4B: String Length
	*
	* 			nB: [all label strings concatenated ]
	*
	*  		0x1e (30)	Bulk Value Update
	*	  	  	4B: Number of Values
	* 			[repeat for all values]
	*  	    4B:	value index n
	*			4B:	value data n
	*
	*		0x1f (31)	New Value
	*			4B: value index
	*			*B: value name
    * 		0x20 (32)	Joystick Update (size 5*4 20 bytes)
    *			4B: Position X  (float, 0.0-1.0)
	*			4B: Position Y  (float, 0.0-1.0)
	*			4B: Angle       (float, radians)
	*			4B: Magnitude   (float, 0.0-1.0)
    * 		0x30 (48)	Slider Update
    *			4B: Slider Number Uint32
	*			4B: Slider Value (float, 0.0-1.0)
    *       0x40 (64)	Button Update
    *			4B: Button Number
    *			4B: Button State (0.0 or 1.0)
    * 		0x50 (80)	Heartbeat
    * 			4B: random int.
    * 		0x60 (96): PID Values Update
	* 			4b: PID Channel
	* 			4b: P (float)
	* 			4b: I (float)
	* 			4b: D (float)
    *
    */

    //Serial.println("Command is: "+String(command)+"\t["+String(packetCount++)+"]");
    switch(command){
    	case 0x20:

    		thisPage->setJoystickValue(asFloat[1], asFloat[2], asFloat[3], asFloat[4]);
    		break;
    	case 0x30:{
    		//Serial.println("Slider Update");
    		//Serial.println("slider:\t"+String(asFloat[1]));
    		//Serial.println("pos:\t"+String(asFloat[2]));
    		thisPage->setSliderValue(asInt[1],asFloat[2]);
    	}
    		break;
    	case 0x40:
    		//Serial.println("Button Update");
    		break;
    	case 0x50:
    		// heartbeat message.
    		thisPage->setHeartbeatUUID(asInt[1]);
    		break;
    	case 0x60:

    		break;

    }


  }
}



char* String2Chars(String str){
	str.toCharArray(stringBuffer, 200, 0);
	return stringBuffer;
}


WebPage::WebPage() {
  thisPage = this;
  joystick.xpos  = 0;
  joystick.ypos  = 0;
  joystick.angle = 0;
  joystick.mag   = 0;
  for(int i=0; i<numValues; i++) {
	  values[i].used=false;
	  values[i].value=0;
	  values[i].name=String("");
	  values[i].valueDirty=false;
	  values[i].labelDirty=false;
  }
  for(int i=0; i<numSliders; i++) sliders[i]=0;
}

bool isSafeToSend(){
	if(millis()-timeSinceLastSend>10){
		return !lockOutSending;
	}
	return false;
}
void lock(){
	while(!isSafeToSend()){
			//Serial.println("W daTA ");
			delay(1);
		}
		lockOutSending=true;
}
void unlock(){
	timeSinceLastSend=millis();
	//Serial.println("R data UnLock");
	lockOutSending=false;

}

void updateTask(void *param){
	//delay(1200);
	while(1){

		lock();
		//Serial.println("L data Lock");
		thisPage->sendHeartbeat();
		thisPage->SendAllLabels();
		thisPage->SendAllValues();
		//while (thisPage->sendPacketFromQueue());
		unlock();

	}
}

void packetTXTask(void *param){
	while(1);
}

void WebPage::initalize(){
	//ESP_LOGI("WebPage::WebPage","WebPage Init..");
	server.begin();
	//Serial.println("HTTP server started");
//
	/*
    server.on("/", 0b00000001, [](AsyncWebServerRequest *request){

    	lock();
		//Serial.println("L text/html Lock");
		request->send(200, "text/html",myHTML );
		unlock();

    });*/
	valuesSem = xSemaphoreCreateMutex();
	xSemaphoreGive(valuesSem);
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);
    server.on("/pidvalues", 0b00000001, [](AsyncWebServerRequest *request){

    	lock();
		//Serial.println("L text/javascript Lock");

		Serial.print("args: ");
		Serial.println(request->args());

		String pidvals="[";

		pidvals += "]";
		request->send(200, "text/html", pidvals);
		unlock();

    });
    server.on("/*", 0b00000001, [](AsyncWebServerRequest *request){
    	String url = request->url();
    	lock();
		//Serial.println("L text/html Lock");
    	Serial.println(url);
    	// lookup our file
    	if (url == "/") url = "/index.html";
    	for(int i=0; i<static_files_manifest_count; i++){
    		if(url.equals(static_files_manifest[i].name)){
    			// This is turbo broken?
    			request->send(200, (char*)static_files_manifest[i].mime, static_files_manifest[i].data);

    		}

    	}
		unlock();

    });


    xTaskCreatePinnedToCore(
    		updateTask, /* Function to implement the task */
          "UpdateTask", /* Name of the task */
		  8192 * 8,  /* Stack size in words */
          NULL,  /* Task input parameter */
          4,  /* Priority of the task */
          &thisPage->updateTaskHandle,  /* Task handle. */
          1); /* Core where the task should run */




}

float WebPage::getSliderValue(uint32_t number){
	//Serial.println("Index:\t"+String(number));
	if (number>3) return 0.0;
	return sliders[number];
}
float WebPage::getJoystickAngle(){
	return joystick.angle;
}
float WebPage::getJoystickMagnitude(){
	return joystick.mag;
}
float WebPage::getJoystickX(){
	return joystick.xpos;
}
float WebPage::getJoystickY(){
	return joystick.ypos;
}
JoyData * WebPage::getJoystickData(){

//	Serial.print("\nJoystick Update");
//	Serial.print(" X pos:\t"+String(joystick.xpos));
//	Serial.print(" Y pos:\t"+String(joystick.ypos));
//	Serial.print(" Angle:\t"+String(joystick.angle));
//	Serial.print(" Dist:\t"+String(joystick.mag));

	return &joystick;
}

void WebPage::setSliderValue(uint32_t number, float value){
	if (number<4) thisPage->sliders[number]=value;
}

void WebPage::setJoystickValue(float xpos, float ypos, float angle, float mag){
	angle-=(PI/2.0);
	joystick.xpos  = cos(angle)*mag;
	joystick.ypos  = sin(angle)*mag;
	joystick.angle = 180*angle/PI;
	joystick.mag   = mag;
}

void WebPage::setValue(String name, float data){
	while (xSemaphoreTake(valuesSem,( TickType_t ) 10)==false);
	if(abs(data)<0.00001)
		data=0;
	for(int i=0; i<numValues; i++){
			if (values[i].used){ // compare in use slots

				if (values[i].name==name){ // check label
					if(values[i].value!=data){ // Has the value itself changed?
						//Serial.println("Update '"+name+"' "+String(data));
						values[i].value = data; // update data
						values[i].valueDirty=true;
					}
					xSemaphoreGive(valuesSem);
					return;
				}
			} else {
				//
				Serial.println("Create '"+name+"' "+String(i));
				values[i].used=true;
				values[i].name = name;
				values[i].value = data;
				values[i].valueDirty=true;
				values[i].labelDirty=true;
				values[i].buffer=0;
				numValuesUsed++;
				xSemaphoreGive(valuesSem);
				return;
			}
	}
}

bool WebPage::dirtyLabels(){
	for(int i=0; i<numValues; i++){
		if (values[i].used && values[i].labelDirty) return true;
	}
	return false;
}

bool WebPage::dirtyValues(){
	for(int i=0; i<numValues; i++){
		if (values[i].used && values[i].valueDirty) return true;
	}
	return false;
}


bool WebPage::SendAllValues(){
	if (ws.count()==0) return false;
	if (dirtyValues()==false) return false;
	uint8_t * packetBuffer = new uint8_t[numValues*8]; // Allocate a new one 8 bytes per slot max

	// Allocate float and int access arrays
	uint32_t *bufferAsInt32=(uint32_t*)packetBuffer;
	float *bufferAsFloat=(float*)packetBuffer;

	bufferAsInt32[0]=0x0000001e; // packet id update all


	uint32_t bufferItemCount=0; // Count the values in this update.
	for(int i=0; i<numValues; i++){
		if (values[i].used){ // Is this slot in use?
			if (values[i].valueDirty){ // Has this value changed
				//values[i].valueDirty=false; //reset change flag
				bufferAsInt32[(bufferItemCount+1)*2] = i; // Index of this specific value
				bufferAsFloat[(bufferItemCount+1)*2+1] = values[i].value; // The float value
				bufferItemCount++;
			}
		}
	}

	bufferAsInt32[1]=bufferItemCount; // Record the number of elements at position 1 after the packet id
	uint32_t packetLength = (bufferItemCount+1)*8; // 8 byte header + each int/float pair


	if (bufferItemCount>0){
		// We have at least 1 item, lets send a packet.
		 if (sendPacket(packetBuffer,packetLength)){
			 // Packet sucessfully sent, clear dirty flags.
			 for(int i=0; i<numValues; i++) values[i].valueDirty=false;
			 return true;
		} else {
			return false;
		}
	}

	return false; // no update sent
	/*
	if(valueToSendThisLoop>=numValuesUsed)
		valueToSendThisLoop=0;
	int i= valueToSendThisLoop;
	if(values[i].used){
		if(values[i].buffer)
			delete values[i].buffer;
		values[i].buffer=new uint8_t[labelbuflen];
		sendLabelUpdate(i,values[i].buffer);
		sendValueUpdate(i,values[i].buffer);

		uint32_t datalen = 12+values[i].name.length()+1;
		for(int j=datalen;j<datalen+4;j++)values[i].buffer[j]=0;
		datalen += (4-(datalen%4)); // round up to multiple of 4
		if (datalen>=labelbuflen) datalen=labelbuflen;
		//datalen=labelbuflen;
	//		Serial.print("\r\nSending Bytes "+String(datalen)+" [");
	//		for(int j=0;j<datalen;j++){
	//
	//			Serial.print(", "+String(values[i].buffer[j])+" ");
	//
	//		}
	//		Serial.print("]");
		if (ws.availableForWriteAll())
			//txPacketCount++;
			ws.binaryAll(values[i].buffer, datalen);
		//delay(5);
		//Serial.println("Updating "+values[i].name);
	}
	valueToSendThisLoop++;
	return values[i].used;
	*/
}


bool WebPage::SendAllLabels(){
	// are any of our labels dirty?
	if (dirtyLabels()==false) return false; // No? don't continue.
	if (xSemaphoreTake(valuesSem,( TickType_t ) 10)==false) return false;
	uint32_t lengthPredict = labelBufferSize;
	uint8_t * labelBuffer = new uint8_t[lengthPredict]; // Allocate a buffer

	uint32_t *bufferAsInt32=(uint32_t*)labelBuffer;
	float *bufferAsFloat=(float*)labelBuffer;
	char *bufferAsChar=(char*)labelBuffer;

	bufferAsInt32[0]=0x0000001d; // packet id update all labels
	bufferAsInt32[1]=0; // num labels

	uint32_t bufferItemCount=0; // Count the values in this update.
	for(int i=0; i<numValues; i++){
		if (values[i].used){ // Is this slot in use?
			if (values[i].labelDirty){ // Has this value changed
				//Serial.println("Dirty: '"+String(values[i].name)+"' ("+String(i)+")");
				bufferAsInt32[(bufferItemCount+1)*3] = i; // Index of this specific label
				bufferAsInt32[(bufferItemCount+1)*3 + 1] = 0; // offset.
				bufferAsInt32[(bufferItemCount+1)*3 + 2] = values[i].name.length()+1; // length of this specific label. +1 for null terminator
				bufferItemCount++;
			}
		}
	}

	if (bufferItemCount==0) {
		xSemaphoreGive(valuesSem);
		return false; // Nothing to update, bail.
	}
	// Write number of labels.
	bufferAsInt32[1]=bufferItemCount;

	uint32_t startOfStringData=(bufferItemCount+1)*12;
	uint32_t stringOffset = 0;

	bufferAsInt32[2]=startOfStringData;
	//Serial.println("Item Count: '"+String(bufferItemCount)+"'");
	//Serial.println("String Data Start: '"+String(startOfStringData)+"'");
	// Load changed strings into buffer.
	// update offsets as you load them in
	for(int i=0; i<bufferItemCount; i++){
		uint32_t index  = bufferAsInt32[(i+1)*3];
		uint32_t length = bufferAsInt32[(i+1)*3 + 2];
		if (startOfStringData+stringOffset+values[index].name.length() > labelBufferSize){
			//Serial.println("Strings won't fit into buffer!");
			//Serial.println("target: "+String(startOfStringData+stringOffset+values[index].name.length()));
			//Serial.println("limit:  "+String(labelBufferSize));
			xSemaphoreGive(valuesSem);
			return false;
		}
		//Serial.print("Processing #"+String(i)+"  len: "+String(length)+"... ");
		bufferAsInt32[(i+1)*3 + 1] = stringOffset;
		memcpy(&bufferAsChar[startOfStringData+stringOffset],values[index].name.c_str(),length);
		stringOffset+=length;
		//values[index].labelDirty=false;
		//Serial.println("Done! packet offset: "+String(startOfStringData+stringOffset));

	}
	//Serial.println("Total Bytes in Buf: '"+String(startOfStringData+stringOffset)+"'");
	//Serial.println("Total Bytes Predicted: '"+String(lengthPredict)+"'");
	uint32_t packetLength = startOfStringData+stringOffset;
	packetLength += (4-(packetLength%4));
	xSemaphoreGive(valuesSem);
	if (bufferItemCount>0){
		// We have at least 1 item, lets send a packet.
		if (sendPacket(labelBuffer,packetLength)){
			// We've sent the data, clear ditry flags
			for(int i=0; i<numValues; i++) values[i].labelDirty=false;
			return true;
		}
	}
	return false;
}

void WebPage::sendValueUpdate(uint32_t index,uint8_t *buffer){
	if(index>numValues-1) return;
//	if (!values[index].used) return;
//	if (values[index].oldValue==values[index].value) return;

	uint32_t *bufferAsInt32=(uint32_t*)buffer;
	float *bufferAsFloat=(float*)buffer;
	bufferAsInt32[0]=0x00000010;
	bufferAsInt32[1]=index;
	bufferAsFloat[2]=values[index].value;
}


void WebPage::sendLabelUpdate(uint32_t index,uint8_t *buffer){
	if(index>numValues-1) return;
//	if (!values[index].used) return;
//	if(!values[index].valueDirty) return;
	values[index].valueDirty=false;

	// Write out the string to the buffer. offset by 12 bytes.
	for(int i=0;i<values[index].name.length();i++){
		buffer[i+12]=values[index].name.c_str()[i];
	}
	buffer[values[index].name.length()+12]=0;


}


void WebPage::newButton(String url, void (*handler)(String), String label, String description){

}


void WebPage::valueChanged(String name, float value){
	//ESP_LOGI("WebPage::valueChanged","Got async change for '%s'",String2Chars(name)); //
	setValue(name,value);
}

void WebPage::markAllDirty(){
	for(int i=0; i<numValues; i++){
		if (values[i].used){ // Is this slot in use?
			values[i].labelDirty=true;
			values[i].valueDirty=true;
		}
	}
}
void WebPage::setHeartbeatUUID(uint32_t uuid){
	_heartbeat_uuid=uuid;
}

bool WebPage::sendHeartbeat(){
	if (_heartbeat_uuid==0) return false;
	uint8_t * heartbeatBuffer = new uint8_t[8];
	uint32_t *bufferAsInt32=(uint32_t*)heartbeatBuffer;
	bufferAsInt32[0]=0x00000050;
	bufferAsInt32[1]=_heartbeat_uuid;
	_heartbeat_uuid=0;
	return sendPacket(heartbeatBuffer,8);
}

void WebPage::printToWebConsole(String data){
	uint32_t length = data.length()+1;
	uint32_t packetLength = length + 8 + (4-(length%4));
	uint8_t *  consolePacket = new uint8_t[packetLength];
	uint32_t *bufferAsInt32=(uint32_t*)consolePacket;

	bufferAsInt32[0]=0x11;
	bufferAsInt32[1]=length;
	data.toCharArray((char *)(consolePacket+8), length);
	sendPacket(consolePacket,packetLength);


}



bool WebPage::sendPacket(unsigned char* packet, uint32_t length){
	if (ws.enabled() && ws.availableForWriteAll()){
		ws.binaryAll(packet,length);
		txPacketCount++;
		delete packet;
		return true;
	}
	//binaryAll MALLOCs memory and MEMCPYs packet into it's own buffer.
	// this does not happen in another thread, it happens here.
	//once it returns we can dealloc.
	delete packet;
	return false;
}
