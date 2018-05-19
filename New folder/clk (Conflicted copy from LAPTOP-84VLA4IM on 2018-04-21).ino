/********************************************************************************
 *
 * @file	clk.ino
 * @author	Joe Todd
 * @version	1
 * @date	April 2018
 * @brief	ESP32 I2C RTC with WiFi, ws conn and led charliplexer
 *
******************************************************************************/

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

#ifdef __cplusplus
extern "C" {
#endif

#define h_button 32
#define spk_pin 33
#define r_pin 12
#define g_pin 13
#define b_pin 14
#define SCL	5
#define SDA	17
#define SER 0
#define RCLK 2
#define SRCLK 15
// RCLK = ST_CP, SRCLK = SH_CP

#define idx " <!doctype html><html><head> <meta charset=\"utf-8\"> <title> clock setup thing </title> <style>  html {   font-family: Segoe, \"Segoe UI\", \"DejaVu Sans\", \"Trebuchet MS\", Verdana, \"sans-serif\";   color: var(--main-font-color);   background-color: var(--main-bg-color);  }  body {   margin: auto;   padding: 20px 20px 500px 20px;   font-size: 18px;   text-align: center;   width: 40%;   background-color: var(--main-body-color);  }  #slide1 {   margin: 1% 5%;   position: relative;   -webkit-transition-duration: 0.3s;   transition-duration: 0.3s;  }  #slide2 {   margin: 1% 5%;   position: relative;   -webkit-transition-duration: 0.3s;   transition-duration: 0.3s;  }  #slide3 {   margin: 1% 5%;   position: relative;   -webkit-transition-duration: 0.3s;   transition-duration: 0.3s;  }  .center {   margin: auto;   width: 50%;   padding: 10px;   text-align: center;  }  input {   position: relative;   margin: auto;   padding: 1%;  }  #t1 {   padding: 2%  }  button {   cursor: pointer;   margin-top: 2%;   padding: 2% 4%;   background-color: var(--main-font-color);   color: var(--main-body-color);   font-size: 18px;   border: 2px solid transparent;   border-radius: 5%;   -webkit-transition-duration: 0.35s;   transition-duration: 0.35s;  }  button:hover {   color: var(--main-font-color);   background-color: transparent;   border: 2px solid var(--main-font-color);  }  .nW {   color: deepskyblue;   border: none;   background-color: transparent;   text-decoration: underline;   font-size: 13px;  }  .nW:hover {   border: none;  }  .circle {   cursor: pointer;   position: fixed;   display: none;   pointer-events: none;   border-radius: 50%;   width: 25px;   height: 25px;   border-style: solid;   border-color: white;   border-width: 1%;   z-index: 1;   -webkit-box-shadow: 5px 5px 2px 0px rgba(0, 0, 0, 0.25);   -moz-box-shadow: 5px 5px 2px 0px rgba(0, 0, 0, 0.25);   box-shadow: 5px 5px 2px 0px rgba(0, 0, 0, 0.25);  }  #ccube {   position: relative;   z-index: 0;   user-select: none;   cursor: pointer;   border-radius: 5%;  }  #prev {   border-radius: 500px;   user-select: none;   position: relative;   margin: auto;   padding: 1%  }  input[type=range] {   cursor: pointer;   -webkit-appearance: none;   margin: auto;   position: relative;   width: 250px;   background-color: transparent;  }  input[type=range]::-webkit-slider-runnable-track {   height: 50%;   background: var(--main-bg-color);   background: -webkit-linear-gradient(left, red, orange, yellow, green, cyan, blue, violet, red);   background: -o-linear-gradient(right, red, orange, yellow, green, cyan, blue, violet, red);   background: -moz-linear-gradient(right, red, orange, yellow, green, cyan, blue, violet, red);   background: linear-gradient(to right, red, orange, yellow, green, cyan, blue, violet, red);   border-radius: 50px;  }  input[type=range]::-webkit-slider-thumb {   -webkit-appearance: none;   border-radius: 50%;   width: 30px;   height: 30px;   margin-top: -10px;   border-style: solid;   border-color: white;   border-width: 1%;   -webkit-box-shadow: 5px 5px 2px 0px rgba(0, 0, 0, 0.25);   -moz-box-shadow: 5px 5px 2px 0px rgba(0, 0, 0, 0.25);   box-shadow: 5px 5px 2px 0px rgba(0, 0, 0, 0.25);   background: var(--main-color);  } </style> <style id=\"mStyle\"> </style> <style id=\"mC\"> </style> <script type=\"application/javascript\">  /*let connection = new WebSocket('ws://192.168.178.90/ws');*/  let connection;  let doUpdate = false;  let cx, cy;  let connected = false;  /*document.onmousedown = function() { doUpdate = true; getPos(); };*/  document.onmouseup = function() {   doUpdate = false;  };  document.onmousemove = getPos;  document.touchend = function() {   doUpdate = true;   getPos();  };  document.touchstart = function() {   doUpdate = false;  };  document.touchmove = getPos;  window.onresize = function() {   getPos(cx, cy);  };  window.onscroll = function() {   getPos(cx, cy);  };  window.onclose = function() {   try {    connection.close();    c_AP.close();    c_STA.close();   } catch(e) {}  };  function init() {   setStyle();   initWs();   toSlide('slide1');   draw();   getPos(125, 125);  }  function initWs() {   let c_AP = new WebSocket('ws://192.168.4.1/ws');   let c_STA = new WebSocket('ws://192.168.178.90/ws');   c_AP.onopen = function () {    if (!connected) {     console.log('AP');     connection = c_AP;     connected = true;     c_STA.close();     connection.onmessage = function(event) {      console.log(event.data);     };    }   };   c_STA.onopen = function () {    if (!connected) {     console.log('STA');     connection = c_STA;     connected = true;     c_AP.close();     connection.onmessage = function(event) {      console.log(event.data);     };    }   };  }  function setStyle() {   let d = new Date();   if (d.getHours() < 8 || d.getHours() > 17) {    document.getElementById(\"mStyle\").innerHTML =     \" :root { --main-bg-color: #111; --main-body-color: #222; --main-font-color: #EEE; }\";   } else {    document.getElementById(\"mStyle\").innerHTML =     \" :root { --main-bg-color: #DDD; --main-body-color: #EEE; --main-font-color: #111; }\";   }  }  function toSlide(index) {   document.getElementById(\"slide1\").style = \"display:none;\";   document.getElementById(\"slide2\").style = \"display:none;\";   document.getElementById(\"slide3\").style = \"display:none\";   document.getElementById(index).style = \"display:block;\";  }  function showPwd() {   if (document.getElementById(\"Pwd\").type == \"password\") {    document.getElementById(\"Pwd\").type = \"text\";   } else {    document.getElementById(\"Pwd\").type = \"password\";   }  }  function submit(s) {   let a = false;   let ssid = document.getElementById(\"SSID\").value;   let pwd = document.getElementById(\"Pwd\").value;   if (ssid != \"\" || pwd != \"\")    a = true;   document.getElementById(\"a\").href = \"/done?W=\" + a + \"&s=\" + ssid + \"&p=\" + pwd;   document.getElementById(\"a\").click();  }  function draw() {   let canvas = document.getElementById(\"ccube\");   let w = canvas.scrollWidth;   let h = canvas.scrollHeight;   let ctx = canvas.getContext(\"2d\");   let hue = document.getElementById(\"hue\").value;   hue = hue / 500.00;   for (let i = w; i >= 0; i--) {    let c0 = HSVtoRGB(hue, 0, i / w);    let c1 = HSVtoRGB(hue, 1, i / w);    let g = ctx.createLinearGradient(0, i, w, 0);    g.addColorStop(0, \"rgb(\" + c0.r + \", \" + c0.g + \", \" + c0.b + \")\");    g.addColorStop(1, \"rgb(\" + c1.r + \", \" + c1.g + \", \" + c1.b + \")\");    ctx.fillStyle = g;    ctx.fillRect(0, i, w, 1);    }   getPos(cx, cy);  }  function mp(rect) {   let x = event.clientX - rect.left;   let y = event.clientY - rect.top;   return {    x: x,    y: y   };  }  function getPos(ax, ay) {   let use = Boolean(ax + 1 && ay + 1);   if (doUpdate || use) {    let prev = document.getElementById(\"prev\");    let canvas = document.getElementById(\"ccube\");    let rect = canvas.getBoundingClientRect();    let cursor = document.getElementById(\"c\");    let hue = document.getElementById(\"hue\").value;    hue = hue / 500.00;    if (use) {     cx = ax;     cy = ay;    } else {     let a = mp(rect);     cx = a.x;     cy = a.y;    }    cx = Math.min(Math.max(cx, 0), canvas.scrollWidth);    cy = Math.min(Math.max(cy, 0), canvas.scrollHeight);    let c = HSVtoRGB(hue, cx / canvas.scrollWidth, cy / canvas.scrollHeight);    let ctx = prev.getContext(\"2d\");    ctx.fillStyle = \"rgb(\" + c.r + \",\" + c.g + \",\" + c.b + \")\";    ctx.fillRect(0, 0, 1000, 1000);    cursor.style = \"display: block; left: \" + (cx + rect.left - 11) + \"px; top: \" + (cy + rect.top - 11) +     \"px; background-color: rgb(\" + c.r + \",\" + c.g + \",\" + c.b + \");\";    document.getElementById(\"mC\").innerHTML = \" :root { --main-color: rgb(\" + c.r + \",\" + c.g + \",\" + c.b + \"); }\";    try {     connection.send(\"r=\" + c.r + \"?g=\" + c.g + \"?b=\" + c.b);    } catch (e) {}   }  }  function HSVtoRGB(h, s, v) {   let r, g, b, i, f, p, q, t;   if (arguments.length === 1) {    s = h.s, v = h.v, h = h.h;   }   i = Math.floor(h * 6);   f = h * 6 - i;   p = v * (1 - s);   q = v * (1 - f * s);   t = v * (1 - (1 - f) * s);   switch (i % 6) {    case 0:     r = v, g = t, b = p;     break;    case 1:     r = q, g = v, b = p;     break;    case 2:     r = p, g = v, b = t;     break;    case 3:     r = p, g = q, b = v;     break;    case 4:     r = t, g = p, b = v;     break;    case 5:     r = v, g = p, b = q;     break;   }   return {    r: Math.round(r * 255),    g: Math.round(g * 255),    b: Math.round(b * 255)   };  } </script></head><body onload=\"init();\"> <div id=\"slide1\">  <h1>We updated the time for you!</h1>  <span id=\"t1\">Your Clock is now nearly ready to use. You can also connect the clock to an existing WiFi Network to update the timer over   the internet for more preceice timing. However this will require more power due to the constant WiFi connection, thus   it is recommended to plug the clock into the supplied wall plug.</span>  <br>  <div class=\"center\">   <button onClick=\"toSlide('slide2');\">Setup WiFi</button>   <br>   <button class=\"nW\" onClick=\"submit(true);\">Continue without setup</button>  </div> </div> <div id=\"slide2\">  <h1>Please Enter your WiFi Credentials.</h1>  <div id=\"s2_b\" class=\"center\">   <input type=\"text\" id=\"SSID\" placeholder=\"SSID\" name=\"ssid\" required title=\"Name of the Network\">   <br>   <input type=\"password\" id=\"Pwd\" placeholder=\"Password\" name=\"pwd\" pattern=\".{8,}\" title=\"8 characters minimum\">   <br> Show Password   <input onClick=\"showPwd()\" type=\"checkbox\">  </div>  <div class=\"center\">   <button onClick=\"toSlide('slide3');\">Continue</button>   <br>   <button class=\"nW\" onClick=\"toSlide('slide1');\">back</button>  </div> </div> <div id=\"slide3\">  <h1>Let's calibrate it!</h1>  <div class=\"center\">   <span>To calibrate your clock manually turn it clockwise until the front panel shows zero. Then just press the calibrate button.</span>   <br>   <button onClick=\"submit(true);\">Calibrate</button>   <br>   <button class=\"nW\" onClick=\"toSlide('slide2');\">back</button>  </div> </div> <a href=\"\" id=\"a\" style=\"display:none\"></a> <div style=\"position: relative;\">  <div class=\"circle\" id=\"c\"></div>  <canvas id=\"ccube\" width=\"250%\" height=\"250%\" onmousedown=\"doUpdate = true; getPos();\"> </canvas>  <br>  <canvas id=\"prev\" width=\"250%\" height=\"30%\" style=\"position: relative\"> </canvas>  <br>  <input type=\"range\" min=\"0\" max=\"500\" id=\"hue\" oninput=\"draw();\"> </div></body></html> "
#define gtx "<!doctype html><html><head><meta charset=\"utf-8\"><title>Clock Time Setup</title>  <script type=\"text/javascript\">    window.onload = function() {   let d = new Date();   document.getElementById(\"l\").href = \"/index?h=\" + p( d.getHours() ) + \"&m=\" + p( d.getMinutes() ) + \"&s=\" + p( d.getSeconds() ) + \"&mo=\" + p( d.getMonth() ) + \"&y=\" + d.getFullYear();   document.getElementById(\"l\").click();  };    function p(a) {   if(a<=9) {    return \"0\"+a;   } else {    return a;   }  }   </script> </head><body>   <a href=\"/index\" id=\"l\" style=\"display: none;\" ></a></body></html>"
#define def "{\"s_ssid\":\"kayetan\",\"s_pwd\":2772549034111546,\"rgb_led\":[125,0,220]}"

//#define ReWriteFiles	1
#define ReReadFiles		1

AsyncWebServer server(80);
AsyncWebSocket ws("/ws"); // access at ws://[esp ip]/ws
//AsyncEventSource events("/events"); // event source (Server-Sent events)

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
uint8_t temprature_sens_read();
static int taskCore = 0;
bool loopdebug = true;

const char * ap_ssid = "4";
const char * ap_pwd = "12345678";
const char * http_username = "admin";
const char * http_password = "alpine";
String s_ssid;
String s_pwd;

uint8_t disp[4] = {0, 0, 0, 0};
uint8_t timer;
//								0		1		2		3		4		5		6		7		8		9
const uint16_t segments[10] = { 0xFD19, 0xF101, 0xFC91, 0xF591, 0xF189, 0xF598, 0xFD98, 0xF111, 0xFD99, 0xF599 };
uint8_t test;
uint8_t nu;

const char* ntpServer[3] = { "pool.ntp.org", "de.pool.ntp.org", "1.pool.ntp.org" };
//const char * ntpServer = "de.pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

const uint8_t max_read_args = 6;

uint8_t rgb_led[3];
bool useWifi;
bool shouldReboot = false;

const bool rwf = ReWriteFiles == 1 ? true : false;
const bool rrf = ReReadFiles == 1 ? true : false;

const int freq = 2000;
const uint8_t channel[4] = {0,1,2,3};
const uint8_t resolution = 8;

#ifdef __cplusplus
}
#endif

void onRequest(AsyncWebServerRequest* request) {
	//Handle Unknown Request
	request->send(404);
}

void onBody(AsyncWebServerRequest * request, uint8_t* data, size_t len, size_t index, size_t total) {
	//Handle body
	Serial.println("onBody:");
}

void onUpload(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final) {
	//Handle upload
	Serial.println("onUpload:");
}

void onEvent(AsyncWebSocket * server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
	
	// TODO: CLEAN THIS SHIT UP!!!!
	if(type == WS_EVT_CONNECT)	{
		Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
		//client->printf("Hello Client %u :)", client->id());
		//client->ping();
	} else if(type == WS_EVT_DISCONNECT)	{
		Serial.printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
	}	else if(type == WS_EVT_ERROR)	{
		Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
	}	else if(type == WS_EVT_PONG)	{
		Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
	}	else if(type == WS_EVT_DATA)	{
		AwsFrameInfo * info = (AwsFrameInfo*)arg;
		if(info->final && info->index == 0 && info->len == len)	{
		//the whole message is in a single frame and we got all of it's data
			Serial.println("Single frame");
			Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);
			if(info->opcode == WS_TEXT){
				data[len] = 0;
				Serial.printf("%s (char*) \n", (char*)data);
				evalJson( (char*)data );
			} else {
				Serial.println("message consists of an array // not supoorted");
				/*
				String t;
				for(size_t i=0; i < info->len; i++){
					t += data[i];
					Serial.printf("%02x", data[i]);
				}
				Serial.printf("(array) \n");
				*/
			}
			//if(info->opcode == WS_TEXT)
				//client->text("I got your text message");
		} else {	//message is comprised of multiple frames or the frame is split into multiple packets
		
			Serial.println("message consists of multiple frames // not supoorted");
			/*
			if(info->index == 0)	{
				if(info->num == 0)
					Serial.printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
				Serial.printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
			}

			Serial.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT)?"text":"binary", info->index, info->index + len);
			if(info->message_opcode == WS_TEXT)	{
				data[len] = 0;
				Serial.printf("%s\n", (char*)data);
			} else {
				String t;
				for(size_t i=0; i < len; i++)	{
					t += data[i];
					Serial.printf("%02x ", data[i]);
				}
				Serial.printf("\n");
			}

			if((info->index + len) == info->len){
				Serial.printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
			if(info->final)	{
					Serial.printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
				if(info->message_opcode == WS_TEXT)
					client->text("I got your text message");
				else
					client->binary("I got your binary message");
				}
			}
			*/
		}
	}
}

void IRAM_ATTR ISR_button() {
	portENTER_CRITICAL_ISR(&mux);

	//Wire.requestFrom(0x68, 19);
	//I2C_receive();
	Serial.printf("Hall: %i\n", hallRead() );
	Serial.printf("Temp: %f°C\n", (temprature_sens_read() - 32) / 1.8);

	portEXIT_CRITICAL_ISR(&mux);
}

void setup() {

	Serial.begin(115200);
	Serial.println();
	Serial.print("Setup running on Core ");
	Serial.println( xPortGetCoreID() );

	rgb_led[0]=0;
	rgb_led[1]=0;
	rgb_led[2]=0;

	pinMode(h_button, INPUT_PULLUP);
	pinMode(SER		, OUTPUT);
	pinMode(RCLK	, OUTPUT);
	pinMode(SRCLK	, OUTPUT);

	ledcSetup(channel[0], freq, resolution);
	ledcSetup(channel[1], freq, resolution);
	ledcSetup(channel[2], freq, resolution);
	ledcSetup(channel[3], 1, 12);

	ledcAttachPin(r_pin, channel[0]);
	ledcAttachPin(g_pin, channel[1]);
	ledcAttachPin(b_pin, channel[2]);
	ledcAttachPin(spk_pin, channel[3]);

	Wire.begin(SDA, SCL);
	Wire.setClock(100000);	// in kHz
	// 0x57 - AT24C32 EEPROM
	// 0x68 - DS3231 RTC

	if(digitalRead(h_button) == HIGH)   {
		useWifi = true;
		Serial.println("Button is High");
	}   else    {
		useWifi = false;
		Serial.println("Button is Low");
	}

	attachInterrupt(digitalPinToInterrupt(h_button), ISR_button, FALLING);

	if(!SPIFFS.begin()){
		Serial.println("SPIFFS mount Failed");
		return;
	}

	playTune(0);
	searchI2C();

	if( rwf )	{
		writeFile(SPIFFS, "/index.htm", idx);
		writeFile(SPIFFS, "/getTime.htm", gtx);
		writeFile(SPIFFS, "/init.a", def);
	}

	if( rrf )	{
		readFile(SPIFFS, "/index.htm", true);
		readFile(SPIFFS, "/getTime.htm", true);
		readFile(SPIFFS, "/init.a", true);
	}

	loadInit(SPIFFS, "/init.a");

	WiFi.disconnect();

	if(useWifi) {

		Serial.println("Loading credentials done");

		Serial.println();
		Serial.print("SSID:");
		Serial.println(s_ssid);
		Serial.print("PWD:");
		Serial.println(s_pwd);

		Serial.printf("Connecting to %s \n", s_ssid);
		WiFi.begin( s_ssid.c_str() , s_pwd.c_str() );
		Serial.println( WiFi.status() );
		while (WiFi.status() != WL_CONNECTED) {
			delay(250);
			Serial.print(".");
		}
		Serial.println(" CONNECTED");

		//init and get the time //maybe try to adjust more
		//configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

		Serial.println();
		Serial.println("WiFi connected");
		Serial.println("IP address: ");
		Serial.println(WiFi.localIP());

	}   else {
	
		Serial.println("Creating Accesspoint");
		WiFi.mode(WIFI_AP);
		WiFi.softAP(ap_ssid, ap_pwd); //???
		//WiFi.begin( ap_ssid, ap_pwd);
		Serial.print("IP address: ");
		Serial.println(WiFi.softAPIP());
	}

	// attach AsyncWebSocket
	ws.onEvent(onEvent);
	server.addHandler(&ws);

	// attach AsyncEventSource
	//server.addHandler( & events);

	// respond to GET requests on URL /heap
	server.on("/heap", HTTP_GET, [](AsyncWebServerRequest * request) {
		Serial.println("/heap");
		request->send(200, "text/plain", String(ESP.getFreeHeap()));
	});

	// upload a file to /upload
	server.on("/upload", HTTP_POST, [](AsyncWebServerRequest * request) {
		request->send(200);
	}, onUpload);

	// send a file when /index is requested
	server.on("/index", HTTP_ANY, [](AsyncWebServerRequest * request) {
		Serial.println("/index");
		request->send(SPIFFS, "/index.htm", "text/html");
		//request->send(200, "text/html", readFile(SPIFFS, "/index.htm") );

		printParams(request);

		Serial.println("done");
	
	});

	server.on("/", HTTP_ANY, [](AsyncWebServerRequest * request) {
		Serial.println("/");
		//request->send(SPIFFS, "/index.htm", "text/plain");
		request->send(200, "text/html", readFile(SPIFFS, "/getTime.htm", false) );

		printParams(request);
		evalParams(request);

		Serial.println("done");
	
	});

	server.on("/done", HTTP_ANY, [](AsyncWebServerRequest * request) {
		Serial.println("/done");
		//request->send(SPIFFS, "/index.htm", "text/plain");
		request->send(200, "text/plain", "200" );

		printParams(request);
		evalParams(request);

		Serial.println("done");
		Serial.println("Flag to reboot");
		vTaskDelay(100);
		shouldReboot = true;
	
	});

	// HTTP basic authentication
	server.on("/login", HTTP_GET, [](AsyncWebServerRequest * request) {
		Serial.println("/login");
		if (!request->authenticate(http_username, http_password))
			return request->requestAuthentication();
		request->send(200, "text/plain", "Login Success!");
	});

	// Catch all
	server.onNotFound(onRequest);
	server.onFileUpload(onUpload);
	server.onRequestBody(onBody);

	server.begin();

	playTune(1);
	xTaskCreatePinnedToCore( coreloop, "coreloop", 4096, NULL, 0, NULL, taskCore);
	vTaskDelay(150);

	//I2C_setTime(0, 37, 16, 5, 22, 03, 18 );
	
	I2C_resetRTCPointer();

	Wire.requestFrom(0x68, 19);
	I2C_receive();

	if(useWifi)	{
		configTime( gmtOffset_sec, daylightOffset_sec, ntpServer[0], ntpServer[1], ntpServer[2] );
		setToNTPTime();
	}

	I2C_resetRTCPointer();

	tm timeinfo = I2C_getRTC();
	Serial.printf("%i:%i:%i, %i, %i %i %i\n\r"  ,
			timeinfo.tm_sec,
			timeinfo.tm_min,
			timeinfo.tm_hour,
			timeinfo.tm_wday,
			timeinfo.tm_mday,
			timeinfo.tm_mon,
			timeinfo.tm_year
		);

}

void loop() {

	if(loopdebug)	{
		loopdebug = false;
			Serial.println();
			Serial.print("loop0 running on Core ");
			Serial.println( xPortGetCoreID() );
	}

	if (shouldReboot) {
		Serial.println("Rebooting...");
		delay(100);
		ESP.restart();
	}
	updateLed();

	if(timer % 50 == 0)	{
		tm timeinfo = I2C_getRTC();
		disp[0] = timeinfo.tm_min % 10;
		disp[1] = timeinfo.tm_min / 10;
		disp[2] = timeinfo.tm_hour % 10;
		disp[3] = timeinfo.tm_hour / 10;
		check_temp();
	}
	timer++;

	vTaskDelay(150);
}

void coreloop( void * pvParameters )	{
	Serial.println();
	Serial.print("loop1 running on Core ");
	Serial.println( xPortGetCoreID() );

	bool dots = true;
	uint8_t dtime = false;

	while(true) {

		shift(disp[0], 0);
		vTaskDelay(1);
		shift(disp[1], 1);
		vTaskDelay(1);
		//shift(disp[3], 3);
		//vTaskDelay(1);

		if(dots)	{
			shift(disp[2], 4);
		}	else	{
			shift(disp[2], 2);
		}

		vTaskDelay(1);
		shift(disp[3], 3);
		vTaskDelay(1);

		if(dtime % 100 == 0)	{
			dots = !dots;
		}
		dtime++;
	}
}

void check_temp()	{
	if(temprature_sens_read() >= 180)	{
		Serial.prinf("!!!OVERHETING!!!");
		shouldReboot = true;
	}
}

void shift(uint8_t num, uint8_t seg)	{

	uint16_t val = segments[num];
	
	switch(seg)	{
		case 0:	val |= 0b00100110; break;
		case 1:	val |= 0b01100100; break;
		case 2:	val |= 0b01100010; break;
		case 4:	val |= 0b001001100010; break;	// like 2 but with dots
		case 3:	val |= 0b01000110; break;
		default: break;
	}

	//Serial.printf("Shifting out %X at %i\n", val, seg);

	digitalWrite(RCLK, LOW);
	for(uint8_t i = 0; i < 16; i++)	{
		digitalWrite(SER, !!(val & (1 << i)));
		digitalWrite(SRCLK, HIGH);
		digitalWrite(SRCLK, LOW);
	}
    digitalWrite(RCLK, HIGH);
}

void searchI2C()	{
	byte error, address;
	int nDevices;

	Serial.println("Scanning...");

	nDevices = 0;
	for(address = 1; address < 127; address++ )	{
		Wire.beginTransmission(address);
		error = Wire.endTransmission();

		if (error == 0)	{
			Serial.print("I2C device found at address 0x");
			if (address<16)
				Serial.print("0");
			Serial.print(address,HEX);		
			Serial.println("");
			nDevices++;
			
		}
		else if (error == 4)	{
			Serial.print("Unknown error at address 0x");
			if (address<16)
				Serial.print("0");
			Serial.println(address,HEX);

		}    
	}
	if (nDevices == 0)
		Serial.println("No I2C devices found\n");
	else
		Serial.println("done\n");
}

void I2C_receive()	{
	Serial.println("I2C Receiving");
	uint8_t i = 0;
	uint8_t data;
	while(Wire.available())	{
		data = Wire.read();
		Serial.printf( "%x\t: %i\t: " ,i ,bcdToDec(data));
		Serial.print( data, BIN );
		Serial.println();
		i++;
	}
}

void I2C_setTime( tm timeinfo )	{

	//Serial.printf("\n Setting Time to %i:%i:%i on %i %i/%i/%i \n", hr, min, sec, day, yr, mon, dte);
	Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S \n");

	Wire.beginTransmission(0x68);

	Wire.write(0);
	Wire.write(decToBcd(timeinfo.tm_sec));
	Wire.write(decToBcd(timeinfo.tm_min)); 
	Wire.write(decToBcd(timeinfo.tm_hour));
	Wire.write(decToBcd(timeinfo.tm_wday));
	Wire.write(decToBcd(timeinfo.tm_mday));
	Wire.write(decToBcd(timeinfo.tm_mon));
	Wire.write(decToBcd(timeinfo.tm_year));

	if( Wire.endTransmission() == 1 )
		Serial.println("Success!");

	I2C_resetRTCPointer();
}

tm I2C_getRTC()	{
	struct tm timeinfo;

	I2C_resetRTCPointer();

	Wire.requestFrom(0x68, 7);

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
	return timeinfo;
}

inline void I2C_resetRTCPointer()	{
	Wire.beginTransmission(0x68);
	Wire.write(0);
	Wire.endTransmission();
}

String readFile(fs::FS &fs, const char * path, bool print)	{
	if(print)
 	   Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if(!file || file.isDirectory()){
		if(print)
    	    Serial.println("Failed to open file for reading");
        return "/r/n";
    }
	if(print)
    	Serial.print("Read from file: ");
	String a;
    while(file.available()){
		a += (char) file.read();
    }
	file.close();

	if(print)
		Serial.println(a);
	return a;
}

void writeFile(fs::FS &fs, const char * path, const char * message){
	Serial.printf("Writing file: %s\n", path);

	File file = fs.open(path, FILE_WRITE);
	if(!file){
		Serial.println("Failed to open file for writing");
		return;
	}
	if(file.print(message)){
		Serial.println("File written");
	} else {
		Serial.println("Write failed");
	}
}

void printParams(AsyncWebServerRequest * request)   {
	int params = request->params();
	Serial.println(params);
	for(int i=0;i<params;i++)   {
		AsyncWebParameter* p = request->getParam(i);
		Serial.println(p->name().c_str());
		Serial.println(p->value().c_str());
		Serial.println(p->size());
		Serial.println();
	}
}

void evalParams(AsyncWebServerRequest * request)    {
	int params = request->params();
	for(int i = 0; i < params; i++)     {
		AsyncWebParameter* p = request->getParam(i);
		String n = p->name().c_str();
		if(n == "s")    {
			s_ssid = (char*) p->value().c_str();
		}   else if(n == "p")   {
			s_pwd = (char*) p->value().c_str();
		}   else if(n == "y")   {
			Serial.println(p->value().c_str());
		}
	}

	Serial.print("ssid = ");
	Serial.println( s_ssid );
	Serial.print("Pwd = ");
	Serial.println( s_pwd );

	String s = "s_ssid=" + (String) s_ssid;		// MAKE ACTUAL FUNCTION WITH SHIT
	s.concat( "?s_pwd="+ (String) s_pwd );

	//writeFile(SPIFFS, "/init.a", s.c_str() );
	readFile(SPIFFS, "/init.a", true);

}

void loadInit( fs::FS &fs, const char* path )	{

	File file = fs.open(path);
	if(!file || file.isDirectory()){
		Serial.println("Failed to open file for reading");
		return;
	}

	String json;
	while(file.available()){
		json += (char) file.read();
	}

	evalJson(json);

}

void evalJson(String json)	{ 
	Serial.println(json);
	StaticJsonBuffer<256> jsonBuffer;
	JsonObject& root = jsonBuffer.parseObject( json.c_str() );

	if (!root.success()) {
		Serial.println("parseObject() failed");
		return;
	}

	s_ssid	=	root["s_ssid"].as<String>();
	s_pwd	=	root["s_pwd"].as<String>();
	rgb_led[0]	=	root["rgb_led"][0].as<uint8_t>();
	rgb_led[1]	=	root["rgb_led"][1].as<uint8_t>();
	rgb_led[2]	=	root["rgb_led"][2].as<uint8_t>();
	shouldReboot =	root["reboot"].as<bool>();
	bool dt  =	root["deviceTime"].as<bool>();

	if(dt)	{
		tm timeinfo;
		timeinfo.tm_year	= root["time"][0].as<uint8_t>();
		timeinfo.tm_mon		= root["time"][1].as<uint8_t>();
		timeinfo.tm_wday	= root["time"][2].as<uint8_t>();
		timeinfo.tm_hour	= root["time"][3].as<uint8_t>();
		timeinfo.tm_min		= root["time"][4].as<uint8_t>();
		timeinfo.tm_sec		= root["time"][5].as<uint8_t>();
		I2C_setTime(timeinfo);
		Serial.printf("%i:%i:%i, %i, %i %i %i \n\r"  ,
				*&timeinfo.tm_sec,
				*&timeinfo.tm_min,
				*&timeinfo.tm_hour,
				*&timeinfo.tm_wday,
				*&timeinfo.tm_mday,
				*&timeinfo.tm_mon,
				*&timeinfo.tm_year
		);
	}

	Serial.printf("%s %s %i %i %i %d %d \n\r", s_ssid, s_pwd, rgb_led[0], rgb_led[1], rgb_led[2], shouldReboot, dt );

}

void setToNTPTime()	{
	struct tm timeinfo;
	if( !getLocalTime(&timeinfo) )	{
		Serial.println("Failed to obtain time");
		return;
	}
	//Serial.println( *&timeinfo.tm_hour );
	Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
	Serial.printf("%i:%i:%i, %i, %i %i %i \n\r"  ,
					*&timeinfo.tm_sec,
					*&timeinfo.tm_min,
					*&timeinfo.tm_hour,
					*&timeinfo.tm_wday,
					*&timeinfo.tm_mday,
					*&timeinfo.tm_mon+1,
					*&timeinfo.tm_year-100
				);
	timeinfo.tm_mon+1;
	timeinfo.tm_year-100;
	I2C_setTime(timeinfo);
}

void updateLed()	{

	ledcWriteTone(channel[0], 2000);
	ledcWriteTone(channel[1], 2000);
	ledcWriteTone(channel[2], 2000);

	ledcWrite(channel[0], rgb_led[0] );
	ledcWrite(channel[1], rgb_led[1] );
	ledcWrite(channel[2], rgb_led[2] );

	//vTaskDelay(100); 
}

inline byte decToBcd(byte val) {
	return( (val/10*16) + (val%10) );
}

inline byte bcdToDec(byte val) {
	return( (val/16*10) + (val%16) );
}

void playTune(uint8_t a)	{

	ledcWriteTone(channel[3], 0); 
	ledcWrite(channel[3], 50); 
	switch(a)	{
		case 0:
	
			ledcWriteNote(channel[3], NOTE_Cs, 2);
			ledcWrite(channel[3], 50); 
			vTaskDelay(170);
			ledcWriteNote(channel[3], NOTE_C, 4);
			ledcWrite(channel[3], 50); 
			vTaskDelay(170);
			ledcWriteNote(channel[3], NOTE_Cs, 3);
			ledcWrite(channel[3], 50); 
			vTaskDelay(170);
			ledcWriteNote(channel[3], NOTE_D, 4);
			ledcWrite(channel[3], 50); 
			vTaskDelay(170);
			ledcWriteNote(channel[3], NOTE_E, 5);
			ledcWrite(channel[3], 50); 
			vTaskDelay(350);
			ledcWriteNote(channel[3], NOTE_F, 4);
			ledcWrite(channel[3], 50);
			vTaskDelay(150);
			ledcWriteNote(channel[3], NOTE_F, 6);
			ledcWrite(channel[3], 50);
			vTaskDelay(150);
			break;
		case 1:
			ledcWriteNote(channel[3], NOTE_D, 3);
			ledcWrite(channel[3], 50); 
			vTaskDelay(170);
			ledcWriteNote(channel[3], NOTE_E, 4);
			ledcWrite(channel[3], 50); 
			vTaskDelay(170);
			ledcWriteNote(channel[3], NOTE_F, 5);
			ledcWrite(channel[3], 50); 
			vTaskDelay(300);
			break;
	}

	ledcWriteTone(channel[3], 0);
	ledcWrite(channel[3], 0);

}