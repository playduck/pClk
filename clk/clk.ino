/**************************************************************************
 * 
 *	<ESP32 Clock project firmware>
 *	Copyright (C) <2018>  <Robin Prillwitz>
 *
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 *	This Work is based on the work of Arduino AG, FreeRTOS,
 *  Espressif Systems and their ESP-idf sdk aswll as
 *  GitHub(c) User "bblanchon"'s "ArduinoJson" library based on ECMA-404.
 *  This frimware also employs the NTP service.
 *
**************************************************************************/

#include <Arduino.h>
#include <WiFi.h>
#include "time.h"
#include "AsyncTCP.h"
#include "ESPAsyncWebServer.h"
#include "FS.h"
#include "SPIFFS.h"
#include <SPIFFSEditor.h>
#include "WebResponseImpl.h"
#include "AsyncEventSource.h"
#include "ArduinoJson.h"
#include "Wire.h"
#include <sys/time.h>
#include <driver/adc.h>
#include <NeoPixelBus.h>

#define NUM_LEDS 12
#define DATA_PIN 27

#define H_BUTTON	32	// HID button pin
#define SPK_PIN		33	// Speaker pin

#define R_PIN		12	// RGB Led Red pin
#define G_PIN		13	// RGB Led Green pin
#define B_PIN		14	// RGB Led Blue pin

#define SCL			22	// I2C Clock pin
#define SDA			21	// I2C Data pin

#define RTC_INT		23	// RTC ALARM INTERRUPT

#define SER			0	// Shift register Data pin
#define RCLK		2	// Shift register Latch pin
#define SRCLK		15	// Shift register Clock pin

#define I2C_RTC		0x68//

/*******************************************************************/

#ifdef __cplusplus
extern "C" {
#endif


NeoPixelBus<NeoGrbwFeature, Neo800KbpsMethod> strip(NUM_LEDS, DATA_PIN);

AsyncWebServer server(80);				// Async Webserver at port 80
AsyncWebSocket ws("/ws");				// Websocets at ws://[esp ip]/ws
//AsyncEventSource events("/events");	// event source (Server-Sent events) (unused)

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;	// used for disabling isr calls inside isr
uint8_t temprature_sens_read();			// enable internal temp sensor
static int taskCore = 0;				// defines secondary core
bool loopdebug = true;					// REPLACE

const char * ap_ssid = "4";				// AP SSID
const char * ap_pwd = "12345678";		// AP PWD
const char * http_username = "admin";	// HTTP Auth Username
const char * http_password = "alpine";	// HTTP Auth pwd
String s_ssid;							// STA SSID as String
String s_pwd;							// STA PWD as String

uint8_t disp[4] = {0, 0, 0, 0};			// number for each led segment
uint16_t timer;							// loop update timer

const char* ntpServer[3] = { "pool.ntp.org", "de.pool.ntp.org", "1.pool.ntp.org" };	// NTP servers
int  gmtOffset_sec = 3600;				// Offset from GMT in seconds
int  daylightOffset_sec = 3600;			// Daylight savings time in seconds
String tz;								// Timezone
uint8_t alarm_h, alarm_m;				// Alarm Houres and Minutes
uint8_t alarm_repeat;					// Alarm Repeat
bool handleAlarm;						// handles an alarm
boolean useNTP;							// use NTP or device Time

float rgb_led[3];						// stores RGB led values as R, G, B
bool useWifi;							// WiFi STA = true, WiFi AP = false
volatile bool shouldReboot = false;		// sets Reboot flag, executed in core 0 main loop

const int freq = 12000;					// led pwm driver frequency
const uint8_t channel[1] = {0};			// ledc lib channel(s) currently only speaker is used
const uint8_t resolution = 8;			// ledc lib "dac" resolution in bits

#ifdef __cplusplus
}
#endif

/*******************************************************************/

void IRAM_ATTR ISR_button() {
	portENTER_CRITICAL_ISR(&mux);

	Serial.printf("[ISRBUTTON] Hall: %i\n", hallRead() );
	Serial.printf("[ISRBUTTON] Temp: %i°C\n", int( (temprature_sens_read() - 32) / 1.8) );

	portEXIT_CRITICAL_ISR(&mux);
}

void IRAM_ATTR ISR_RTC()	{
	portENTER_CRITICAL_ISR(&mux);
	Serial.printf("[ISRRTC]\n");
	handleAlarm = true;
	portEXIT_CRITICAL_ISR(&mux);
}

// Entry Point
void setup() {

	Serial.begin(115200);
	while (!Serial);
	check_temp();
	Serial.printf("[SETUP] Setup running on Core %i\n", xPortGetCoreID() );

	rgb_led[0]=0;
	rgb_led[1]=0;
	rgb_led[2]=0;

	pinMode(H_BUTTON, INPUT_PULLUP);
	pinMode(SER		, OUTPUT);
	pinMode(RCLK	, OUTPUT);
	pinMode(SRCLK	, OUTPUT);
	pinMode(DATA_PIN, OUTPUT);

	ledcSetup(channel[0], 1, 12);
	ledcAttachPin(SPK_PIN, channel[0]);

	if(!SPIFFS.begin()){
		Serial.printf("[SETUP] SPIFFS mount Failed\n");
		return;
	}
	loadInit(SPIFFS, "/init.a");

	if(digitalRead(H_BUTTON) == HIGH)   {
		useWifi = true;
		Serial.printf("[SETUP] Button is High\n");
	}   else    {
		useWifi = false;
		Serial.printf("[SETUP] Button is Low\n");
	}

	playTune(0);

 	Serial.printf("[SETUP] Starting I2C\n");

	Wire.begin(SDA, SCL);
	Wire.setClock(400000);	// in kHz
	Wire.setTimeout(1000);
	// 0x57 - AT24C32 EEPROM
	// 0x68 - DS3231 RTC

	Serial.printf("[SETUP] Searching I2C\n");
	vTaskDelay(500);
	searchI2C();
	I2C_resetRTCPointer();
	I2C_configRTC();

	Serial.printf("[SETUP] Reading time from RTC\n");	
	struct tm timeinfo = I2C_getRTC();
	Serial.println(&timeinfo, "[SETUP] %A, %B %d %Y %H:%M:%S");

//	searchWiFi();
	startWiFi();
	initServer();

	if(useNTP)	{
		setToNTPTime();

		Serial.printf("[SETUP] Reading time from RTC\n");	
		struct tm timeinfo = I2C_getRTC();
		Serial.println(&timeinfo, "[SETUP] %A, %B %d %Y %H:%M:%S");
	}

	attachInterrupt(digitalPinToInterrupt(H_BUTTON), ISR_button, FALLING);
	attachInterrupt(digitalPinToInterrupt(RTC_INT), ISR_RTC, FALLING);

	strip.Begin();
	strip.Show();

	updateLed();
	playTune(1);
	xTaskCreatePinnedToCore( coreloop, "coreloop", 4096, NULL, 0, NULL, taskCore);
	Serial.printf("[SETUP] Setup complete\n");
}

void loop() {

	if(loopdebug)	{
		loopdebug = false;
			Serial.printf("[LOOP0] loop0 running on Core %i\n", xPortGetCoreID() );
	}
	if( (WiFi.status() != WL_CONNECTED) && useWifi )	{
		Serial.printf("[LOOP0] Lost Wifi Connection");
		startWiFi();
	}

	updateLed();

	vTaskDelay(50);

}

void coreloop( void * pvParameters )	{

	Serial.printf("[LOOP1] loop1 running on Core %i\n", xPortGetCoreID() );

	while(true) {

	// 	shift(disp[0], 0);
	// 	shift(disp[1], 1);
	// 	shift(disp[2], 2);
	// 	shift(disp[3], 3);

	if(shouldReboot) {
		saveInit(SPIFFS);
		Serial.printf("[LOOP1] Rebooting...\n");
		delay(100);
		ESP.restart();
	}
	if(handleAlarm)	{
		I2C_evalAlarm();
		handleAlarm = false;
	}

	if(timer % 512 == 0)	{
		struct tm timeinfo = I2C_getRTC();
		disp[0] = timeinfo.tm_min % 10;
		disp[1] = timeinfo.tm_min / 10;
		disp[2] = timeinfo.tm_hour % 10;
		disp[3] = timeinfo.tm_hour / 10;
		check_temp();
	}
	timer++;
	vTaskDelay(50);

	}
	
}

void searchWiFi()	{
	Serial.println("[SEARCHWIFI] scan start");

    // WiFi.scanNetworks will return the number of networks found
    int n = WiFi.scanNetworks();
    Serial.println("[SEARCHWIFI] scan done");
    if (n == 0) {
        Serial.println("[SEARCHWIFI] no networks found");
    } else {
        Serial.printf("[SEARCHWIFI] %i networks found\n", n);
        for (int i = 0; i < n; ++i) {
            // Print SSID and RSSI for each network found
			Serial.printf("[SEARCHWIFI] %i : ", i+1);
            Serial.print(WiFi.SSID(i));
            Serial.print(" (");
            Serial.print(WiFi.RSSI(i));
            Serial.print(")");
            Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
            delay(10);
        }
    }
}

void startWiFi()	{

	if(useWifi) {

		WiFi.mode(WIFI_STA);
		WiFi.disconnect(true);
		WiFi.persistent(false);		

		Serial.printf("[STARTWIFI] SSID: %s : PWD: %s\n[", s_ssid.c_str() , s_pwd.c_str() );
		WiFi.enableSTA(true);
		WiFi.begin( s_ssid.c_str() , s_pwd.c_str() );
		long m = millis();
		while (WiFi.status() != WL_CONNECTED) {
			delay(250);
			Serial.printf("=");
		/*	if(millis() > m +12000)	{
				Serial.printf("]\n[STARTWIFI] Timeout\n");
				delay(500);
				ESP.restart();
			}
		*/
		}
		Serial.printf("]\n[STARTWIFI] CONNECTED\n");

	}   else {
		WiFi.enableSTA(false);
		Serial.printf("[STARTWIFI] Creating Accesspoint\n");
		WiFi.mode(WIFI_AP);
		WiFi.softAP(ap_ssid, ap_pwd);
		//WiFi.begin( ap_ssid, ap_pwd);
		
	}

	Serial.print("[STARTWIFI] IP address: ");
	Serial.println(WiFi.localIP());
	Serial.print("[STARTWIFI] ESP Mac Address: ");
	Serial.println(WiFi.macAddress());
	Serial.print("[STARTWIFI] Subnet Mask: ");
	Serial.println(WiFi.subnetMask());
	Serial.print("[STARTWIFI] Gateway IP: ");
	Serial.println(WiFi.gatewayIP());
	Serial.print("[STARTWIFI] DNS: ");
	Serial.println(WiFi.dnsIP());
}

void initServer()	{

	Serial.printf("[INITSERVER]  /done\n");

	ws.onEvent(onEvent);
	server.addHandler(&ws);

	//attach AsyncEventSource
	//server.addHandler( & events);

	// respond to GET requests on URL /heap
	server.on("/heap", HTTP_GET, [](AsyncWebServerRequest * request) {
		Serial.printf("[INITSERVER] /heap\n");
		request->send(200, "text/plain", String(ESP.getFreeHeap()));
	});
	// upload a file to /upload
	server.on("/upload", HTTP_POST, [](AsyncWebServerRequest * request) {
		request->send(200);
	}, onUpload);
	// send a file when /index is requested
	server.on("/done", HTTP_ANY, [](AsyncWebServerRequest * request) {

		//request->send(SPIFFS, "/index.htm", "text/plain");
		request->send(200, "text/plain", "200" );
		Serial.printf("[INITSERVER]  done\n");
		Serial.printf("[INITSERVER]  Flag to reboot\n");
		shouldReboot = true;
	
	});
	server.on("/login", HTTP_GET, [](AsyncWebServerRequest * request) {
		Serial.printf("[INITSERVER]  /login\n");
		if (!request->authenticate(http_username, http_password))
			return request->requestAuthentication();
		request->send(200, "text/plain", "Login Success!");
	});
	server.on("/", HTTP_ANY, [](AsyncWebServerRequest * request) {
		Serial.printf("/\n");
		//request->send(SPIFFS, "/index.htm", "text/html");
		request->send(200, "text/html", readFile(SPIFFS, "/index.htm", false) );
		Serial.printf( "[INITSERVER] InitServer running on Core %i\n", xPortGetCoreID() );
	});

	// Catch all
	server.onNotFound(onRequest);
	server.onFileUpload(onUpload);
	server.onRequestBody(onBody);

	server.begin();
}

void check_temp()	{
	if(temprature_sens_read() >= 180)	{
		Serial.printf("---- !!!OVERHEATING!!! ----");
		ESP.restart();
	}
}

void shift(uint8_t num, uint8_t seg)	{
	
	num &= 0x0F;
	switch(seg)	{
		case 0:	num |= 0b01000000; break;
		case 1:	num |= 0b10000000; break;
		case 2:	num |= 0b00010000; break;
		case 3:	num |= 0b00100000; break;
		default: break;
	}

	//Serial.printf("num: %X on seg: %u \n", num, seg);

	digitalWrite(RCLK, LOW);
	shiftOut(SER, SRCLK, LSBFIRST, num);
    digitalWrite(RCLK, HIGH);

	vTaskDelay(1);
}

void searchI2C()	{
	byte error, address;
	int nDevices;

	Serial.printf("[SEARCHI2C] Scanning...\n");

	nDevices = 0;
	for(address = 1; address < 127; address++ )	{
		Wire.beginTransmission(address);
		error = Wire.endTransmission();

		if (error == 0)	{
			Serial.printf("[SEARCHI2C] I2C device found at address 0x");
			if (address<16)
				Serial.printf("0");
			Serial.printf("%x\n",address);		
			nDevices++;
			
		}
		else if (error == 4)	{
			Serial.printf("[SEARCHI2C] Unknown error at address 0x");
			if (address<16)
				Serial.printf("0");
			Serial.printf("%x\n",address);

		}    
	}
	if (nDevices == 0)	{
		Serial.printf("[SEARCHI2C] No I2C devices found\n");
	}	
	else	{
		Serial.printf("[SEARCHI2C] done\n");
	}

}

void I2C_receive()	{
	//Serial.printf("[I2CRECEIVE] I2C Receiving\n");
	uint8_t i = 0;
	uint8_t data;
	while(Wire.available())	{
		data = Wire.read();
		Serial.printf( "[I2CRECEIVE] %x\t: %i\t: " ,i ,bcdToDec(data));
		Serial.print( data, BIN );
		Serial.printf("\n");
		i++;
	}
}

void I2C_setTime( tm timeinfo )	{

	//Serial.printf("\n Setting Time to %i:%i:%i on %i %i/%i/%i \n", hr, min, sec, day, yr, mon, dte);
	Serial.println(&timeinfo, "[I2CSETTIME] %A, %B %d %Y %H:%M:%S");
	I2C_resetRTCPointer();

	vTaskDelay(100);

	Wire.beginTransmission(I2C_RTC);

	Wire.write(0);
	Wire.write(decToBcd(timeinfo.tm_sec));
	Wire.write(decToBcd(timeinfo.tm_min)); 
	Wire.write(decToBcd(timeinfo.tm_hour));
	Wire.write(decToBcd(timeinfo.tm_wday));
	Wire.write(decToBcd(timeinfo.tm_mday));
	Wire.write(decToBcd(timeinfo.tm_mon));
	Wire.write(decToBcd(timeinfo.tm_year));

	if( Wire.endTransmission(false) == 1 )
		Serial.printf("[I2CSETTIME] Success!\n");

	I2C_resetRTCPointer();
}

void I2C_setAlarm()	{

	tm timeinfo = I2C_getRTC();
	uint8_t day = timeinfo.tm_wday == 0 ? 7 : timeinfo.tm_wday;
	bool notToday = true;
	
	bool b = (timeinfo.tm_hour == alarm_h) && (timeinfo.tm_min < alarm_m);
	bool c = (timeinfo.tm_hour <= alarm_h);

	if( (b || c) && ((1<<(day-1) & alarm_repeat) > 0) )	{
		notToday = false;
		Serial.printf("[I2CSETALARM] Alarm will trigger today (%i)\n", day);
	}

	//Serial.printf("[I2CSETALARM] %i, %i, %i, %i, %i, %i, %i \n", timeinfo.tm_wday, timeinfo.tm_hour, timeinfo.tm_min, (1<<(day-1) & alarm_repeat), (1<<(day-1)), day, day-1 );

	I2C_resetRTCPointer();

	while(notToday)	{
		day = day % 7;
		if( (1<<day & alarm_repeat) > 0 )	{
			day = (day % 7 ) + 1;
			Serial.printf("[I2CSETALARM] Alarm will not trigger today \n");
			break;
		}
		day++;
	}	
	
	day = day == 7 ? 0 : day;
	Serial.printf("[I2CSETALARM] Alarm will trigger on %i at %i:%i!\n",day, alarm_h, alarm_m);

	Wire.beginTransmission(I2C_RTC);

	Wire.write(0x07);
	Wire.write(0b00000000);
	Wire.write(decToBcd(alarm_m)	& 0b01111111 );
	Wire.write(decToBcd(alarm_h)	& 0b00111111 );
	Wire.write((decToBcd(day)		& 0b01111111 ) | 0b01000000 );

	if( Wire.endTransmission(false) == 0 )
		Serial.printf("[I2CSETALARM] Success!\n");

	I2C_resetRTCPointer();

}

void I2C_evalAlarm()	{

	I2C_resetRTCPointer();
	
	playTune(2);
	Wire.requestFrom(I2C_RTC, 0x10);
	uint8_t cw = 0;

	while(Wire.available())	{
		cw = Wire.read();
	}

	if( (cw & 0b00000010) > 0) {
		Serial.printf("[I2CEVALALARM] A2F\n");	// Hourly Alarm
	}
	if( (cw & 0b00000001) > 0) {
		Serial.printf("[I2CEVALALARM] A1F\n");	// User Alarm
	}

	I2C_resetRTCPointer();
	I2C_configRTC();

}

tm I2C_getRTC()	{
	struct tm timeinfo;

	I2C_resetRTCPointer();

	Wire.requestFrom(I2C_RTC, 7);

	uint8_t i = 0;
	uint8_t data;
	while(Wire.available())	{
		data = Wire.read();

		switch(i)	{
			case 0x00:	timeinfo.tm_sec = bcdToDec(data);	
												break;	//Seconds
			case 0x01:	timeinfo.tm_min	= bcdToDec(data);
												break;	//Minutes
			case 0x02:	data &= 0b10111111;
						timeinfo.tm_hour = bcdToDec(data);	
												break;	//Hours
			case 0x03:	timeinfo.tm_wday = bcdToDec(data);
												break;	//Day
			case 0x04:	timeinfo.tm_mday = bcdToDec(data);
												break;	//Date
			case 0x05:	data &= 0b01111111;
						timeinfo.tm_mon = bcdToDec(data);			
												break;	//Month
			case 0x06:	timeinfo.tm_year = bcdToDec(data);
												break;	//Year
		}
		i++;
	}

	Serial.println(&timeinfo, "[I2CGETRTC] %A, %B %d %Y %H:%M:%S");

	I2C_resetRTCPointer();

	return timeinfo;
}

void I2C_configRTC()	{

	Wire.beginTransmission(I2C_RTC);
	
	Wire.write(0x0B);
	Wire.write(0b00000000);
	Wire.write(0b10000000);
	Wire.write(0b10000000);
	Wire.write(0b10000111);
	Wire.write(0b00000100);

	if( Wire.endTransmission(false) == 0 )
		Serial.printf("[I2CCONFIGRTC] Success!\n");

	I2C_resetRTCPointer();

}

inline void I2C_resetRTCPointer()	{
	Wire.beginTransmission(I2C_RTC);
	Wire.write(0);
	Wire.endTransmission(false);
	vTaskDelay(50);
}

String readFile(fs::FS& fs, const char* path, bool print)	{
	if(print)
 	   Serial.printf("[READFILE] Reading file: %s\n", path);

    File file = fs.open(path);
    if(!file || file.isDirectory()){
		if(print)
    	    Serial.printf("[READFILE] Failed to open file for reading\n");
        return "/r/n";
    }
	if(print)
    	Serial.printf("[READFILE] Read from file: ");
	String a;
    while(file.available()){
		a += (char) file.read();
    }
	file.close();

	if(print)
		Serial.printf("%s\n", a.c_str());
	return a;
}

void writeFile(fs::FS& fs, const char* path, const char* message)	{
	Serial.printf("[WRITEFILE] Writing file: %s\n", path);

	File file = fs.open(path, FILE_WRITE);
	if(!file){
		Serial.printf("[WRITEFILE] Failed to open file for writing\n");
		return;
	}
	if(file.print(message)){
		Serial.printf("[WRITEFILE] File written\n");
	} else {
		Serial.printf("[WRITEFILE] Write failed\n");
	}
}

void loadInit( fs::FS &fs, const char* path )	{

	File file = fs.open(path);
	if(!file || file.isDirectory()){
		Serial.printf("[LOADINIT] Failed to open file for reading\n");
		return;
	}

	String json;
	while(file.available()){
		json += (char) file.read();
	}

	evalJson(json.c_str() );

}

void saveInit(fs::FS &fs)	{
	//Serial.printf("[SAVEINIT]  s:%s p:%s rgb: %i %i %i \n", s_ssid.c_str(), s_pwd.c_str(), rgb_led[0],  rgb_led[1],  rgb_led[2] );
	//	const size_t bufferSize = JSON_ARRAY_SIZE(10) + JSON_OBJECT_SIZE(10);
	DynamicJsonBuffer jsonBuffer(512);
	JsonObject& root = jsonBuffer.createObject();

	root["s_ssid"] = s_ssid;
	root["s_pwd"] = s_pwd;

	//root["gmtOff"] = gmtOffset_sec;
	//root["dayOff"] = daylightOffset_sec;
	root["TZC"] = tz;

	root["useW"] = useWifi;
	root["ntpTime"] = useNTP;

	root["re"] = alarm_repeat;
	root["a_h"] = alarm_h;
	root["a_m"] = alarm_m;

	JsonArray& rgb = root.createNestedArray("rgb");
	rgb.add(rgb_led[0]);
	rgb.add(rgb_led[1]);
	rgb.add(rgb_led[2]);
	
	//root.printTo(Serial);
	String jstring;
	root.printTo(jstring);
	Serial.printf("[SAVEINIT] %s\n", jstring.c_str());

	writeFile(SPIFFS, "/init.a", jstring.c_str() );
}

void evalJson(String json)	{
	//Serial.printf("[EVALJSON] %s\n", json.c_str());
	StaticJsonBuffer<400> jsonBuffer;
	JsonObject& root = jsonBuffer.parseObject( json.c_str() );

	if (!root.success()) {
		Serial.printf("[EVALJSON] parseObject() failed\n");
		return;
	}

	s_ssid	=	root.containsKey("s_ssid")	? root["s_ssid"].as<String>()	: s_ssid;
	s_pwd	=	root.containsKey("s_pwd")	? root["s_pwd"].as<String>()	: s_pwd;

	if(root.containsKey("rgb") )	{
		rgb_led[0]	=	root["rgb"][0].as<float>();
		rgb_led[1]	=	root["rgb"][1].as<float>();
		rgb_led[2]	=	root["rgb"][2].as<float>();
	}

	useWifi				=	root.containsKey("useW")	? root["useW"].as<bool>() : useWifi;
	useNTP				=	root.containsKey("ntpTime")	? root["ntpTime"].as<bool>() : useNTP;
	shouldReboot		=	root.containsKey("reboot")	? root["reboot"].as<bool>() : false;
	//gmtOffset_sec		=	root.containsKey("gmtOff")	? root["gmtOff"].as<int>() : gmtOffset_sec;
	//daylightOffset_sec	=	root.containsKey("dayOff")	? root["dayOff"].as<int>() : daylightOffset_sec;
	tz					=	root.containsKey("TZC")		? root["TZC"].as<String>() : tz;

	if(root["alarm"].as<bool>())	{
		alarm_repeat = root["re"].as<uint8_t>();
		alarm_h = root["a_h"].as<uint8_t>();
		alarm_m = root["a_m"].as<uint8_t>();
		//TODO implement volume
		I2C_setAlarm();
	}

	if(root["deviceTime"].as<bool>())	{
		struct tm timeinfo;	//Set RTC to received time
		timeinfo.tm_year	= root["time"][0].as<uint8_t>();
		timeinfo.tm_mon		= root["time"][1].as<uint8_t>();
		timeinfo.tm_mday	= root["time"][2].as<uint8_t>();
		timeinfo.tm_wday	= root["time"][3].as<uint8_t>();
		timeinfo.tm_hour	= root["time"][4].as<uint8_t>();
		timeinfo.tm_min		= root["time"][5].as<uint8_t>();
		timeinfo.tm_sec		= root["time"][6].as<uint8_t>();
		I2C_setTime(timeinfo);
	}
	if(root["ntpTime"].as<bool>() && WiFi.status() == WL_CONNECTED)	{
		//Set RTC to NTP time
		//configTime( gmtOffset_sec, daylightOffset_sec, ntpServer[0], ntpServer[1], ntpServer[2] );
		setToNTPTime();	
	}
	if(root.containsKey("rgb"))	{
		updateLed();
	}
	if(root["saveSettings"].as<bool>())	{
		saveInit(SPIFFS);
	}
}

int setToNTPTime()	{
	//configTime( gmtOffset_sec, daylightOffset_sec, ntpServer[0], ntpServer[1], ntpServer[2] );
	Serial.printf("[SETTONTPTIME] TZ : %s\n", tz.c_str());
	configTime( 0, 0, ntpServer[0], ntpServer[1], ntpServer[2] );
	setenv("TZ", tz.c_str(), 1);
	tzset();
	vTaskDelay(50);
	struct tm timeinfo;
	if( !getLocalTime(&timeinfo) )	{
		Serial.printf("[SETTONTPTIME] Failed to obtain time\n");
		return 0;
	}
	//Serial.println( *&timeinfo.tm_hour );
	Serial.println(&timeinfo, "[SETTONTPTIME] %A, %B %d %Y %H:%M:%S");

	//timeinfo.tm_mon += 1;
	//timeinfo.tm_year -= 100;
	I2C_setTime(timeinfo);
	return 1;
}

void updateLed()	{

	//Serial.printf("[UPDATELED] RGB: %i %i %i\n", rgb_led[0], rgb_led[1], rgb_led[2]);	

	RgbwColor c ( HsbColor(rgb_led[0], rgb_led[1], rgb_led[2]));
	for(uint8_t index = 0; index < NUM_LEDS; index++)	{
		strip.SetPixelColor(index, c);
	}
	vTaskDelay(50);
	strip.Show();
}

inline byte decToBcd(byte val) {
	return( (val/10*16) + (val%10) );
}

inline byte bcdToDec(byte val) {
	return( (val/16*10) + (val%16) );
}

void playTune(uint8_t a)	{

	ledcWriteTone(channel[0], 0); 
	ledcWrite(channel[0], 50); 
	switch(a)	{
		case 0:
	
			ledcWriteNote(channel[0], NOTE_Cs, 2);
			ledcWrite(channel[0], 50); 
			vTaskDelay(170);
			ledcWriteNote(channel[0], NOTE_C, 4);
			ledcWrite(channel[0], 50); 
			vTaskDelay(170);
			ledcWriteNote(channel[0], NOTE_Cs, 3);
			ledcWrite(channel[0], 50); 
			vTaskDelay(170);
			ledcWriteNote(channel[0], NOTE_D, 4);
			ledcWrite(channel[0], 50); 
			vTaskDelay(170);
			ledcWriteNote(channel[0], NOTE_E, 5);
			ledcWrite(channel[0], 50); 
			vTaskDelay(350);
			ledcWriteNote(channel[0], NOTE_F, 4);
			ledcWrite(channel[0], 50);
			vTaskDelay(150);
			ledcWriteNote(channel[0], NOTE_F, 6);
			ledcWrite(channel[0], 50);
			vTaskDelay(150);
			break;
		case 1:
			ledcWriteNote(channel[0], NOTE_D, 3);
			ledcWrite(channel[0], 50); 
			vTaskDelay(170);
			ledcWriteNote(channel[0], NOTE_E, 4);
			ledcWrite(channel[0], 50); 
			vTaskDelay(170);
			ledcWriteNote(channel[0], NOTE_F, 5);
			ledcWrite(channel[0], 50); 
			vTaskDelay(300);
			break;
		case 2:
			ledcWriteNote(channel[0], NOTE_Cs, 3);
			ledcWrite(channel[0], 50); 
			vTaskDelay(150);
			break;
	}

	ledcWriteTone(channel[0], 0);
	ledcWrite(channel[0], 0);

}

void onRequest(AsyncWebServerRequest* request) {
	//Handle Unknown Request
	request->send(404);
}

void onBody(AsyncWebServerRequest * request, uint8_t* data, size_t len, size_t index, size_t total) {
	//Handle body
	request->send(404);
}

void onUpload(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final) {
	//Handle upload
	request->send(404);
}

void onEvent(AsyncWebSocket * server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {

	//Serial.printf("[ONEVENT] WS response running on Core %i\n", xPortGetCoreID() );
	
	// TODO: CLEAN THIS SHIT UP!!!!
	if(type == WS_EVT_CONNECT)	{
		//Serial.printf("[ONEVENT]  ws[%s][%u] connect\n", server->url(), client->id());

		client->printf("{\"rgb_led\":[%f,%f,%f]}", rgb_led[0], rgb_led[1], rgb_led[2]);
		return;

	} else if(type == WS_EVT_DISCONNECT)	{
		//Serial.printf("[ONEVENT]  ws[%s][%u] disconnect: %u\n", server->url(), client->id(), client->id());
		//saveInit(SPIFFS);
	}	else if(type == WS_EVT_ERROR)	{
		//Serial.printf("[ONEVENT]  ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
	}	else if(type == WS_EVT_PONG)	{
		//Serial.printf("[ONEVENT]  ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
	}	else if(type == WS_EVT_DATA)	{

		AwsFrameInfo * info = (AwsFrameInfo*)arg;

		if(info->final && info->index == 0 && info->len == len)	{
	
			//Serial.println("Single frame");
			//Serial.printf("[ONEVENT]  ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);
			if(info->opcode == WS_TEXT){
				data[len] = 0;
				Serial.printf(" %s (char*) \n", (char*)data);

				client->printf("__s__");
				evalJson( (char*)data );
				return;


			} else {
				Serial.printf("[ONEVENT]  message consists of an array // not supoorted\n");
			}
		} else {	//message is comprised of multiple frames or the frame is split into multiple packets
			Serial.printf("[ONEVENT]  message consists of multiple frames // not supoorted\n");
		}
	}	
	else	{
		Serial.printf("[ONEVENT]  Received unknown websocket activity!");
	}
	
}
