/*jshint esversion: 6 */
"use strict";

	let doUpdate = false;
	let global_hue; 
	let global_cx, global_cy;
	let connection;
	let connected;

	window.onload = function ()	{
		initWs();
		updateStyle();
		//changeTab("L");
		//window.scroll(0,0);
		findColor( Math.random()*255, Math.random()*255, Math.random()*255, false );
		setTimeSelect(0);
		//draw(global_hue);
		/*loop();*/
		//updateTime();
	};
	
	window.onmousemove = function(event)	{
		if(doUpdate)	{
			document.body.style.cursor = "pointer";
			getColor(global_hue, event.clientX, event.clientY);
		}
	};

	window.onmouseup	= function()	{
		doUpdate = false;
		document.body.style.cursor = "auto";
	};

	window.onscroll	= function()	{
		getNeColor();
	};

	window.onresize = function()	{
		getNeColor();
	};

	window.onclose = function()		{
		try	{
			connection.close();
			c_AP.close();
			c_STA.close();
		}	catch(e)	{}
	};
		
	function sendWs(a)	{
		try {
			console.log(a);
			console.log( JSON.stringify(a) );
			connection.send(JSON.stringify(a));
		} catch (e) {}
		//triggerSnackbar("success");
	}
	
	function updateTime(a)	{
		let d = new Date();
		//document.getElementById("time").innerHTML = a[4]+":"+a[]
		setTimeout(updateTime, 1 );
	}
	
	function initWs() {

			try	{
				let c_AP = new WebSocket('ws://192.168.4.1/ws');
				let c_STA = new WebSocket('ws://192.168.178.90/ws');
			

			c_AP.onopen = function () {
				if (!connected) {
					connection = c_AP;
					connected = true;
					c_STA.close();
					receiveWs();
					triggerSnackbar("con");
				}
			};

			c_STA.onopen = function () {
				if (!connected) {
					connection = c_STA;
					connected = true;
					c_AP.close();
					receiveWs();
					triggerSnackbar("con");
				}
			};
			}	catch(e)	{
				console.log(e);	
			}
		}
	
	function receiveWs()	{
			connection.onmessage = function(event) {
				console.log("Received: "+ event.data);
				if(event.data === "__s__" )	{
					triggerSnackbar("success");
					return;
				}
				let a = JSON.parse(event.data);
				console.log(a);
				if(a.rgb_led !== undefined)	{
					findColor(a.rgb_led[0], a.rgb_led[1], a.rgb_led[2], false);
				}
			};
	}

	function sendWiFi()	{
		let s	= document.getElementById("s").value;
		let p	= document.getElementById("p").value;
		sendWs( {"s_ssid":s, "s_pwd":p} );
	}
	
	function sendDeviceTime()	{
		let d = new Date();
		sendWs( {"ntpTime":false, "deviceTime":true, "time":[d.getYear(), d.getMonth(), d.getDate(), d.getDay(), d.getHours(), d.getMinutes(), d.getSeconds()]} );
	}
	
	function sendNTP()	{
		let gmtOff = document.getElementById("gmtoff").value /100.0 * 3600.0;
		let dayOff = document.getElementById("dayoff").value /100.0 * 3600.0;
		sendWs( {"ntpTime":true, "deviceTime":false, "gmtOffset_sec":gmtOff, "daylightOffset_sec": dayOff} );
	}
	
	function sendALR()	{
		let re = 0;
		for(let i = 0; i < 7; i++)	{
			re = re | ( document.getElementById("wk"+i).checked ? 1 : 0 ) << i;
		}
		let h, m ,s;
		let val = document.getElementById("alr").value;
		if(val === "")
			return;
		h = parseInt(val.slice(0,2));
		m = parseInt(val.slice(3,5));
		let vol = document.getElementById("vol").value;
		sendWs( {"alarm":true, "re":re, "a_h":h, "a_m":m, "a_vol":vol} );
	}
	
	function sendTz()	{
		let b = document.getElementById("cs").options[document.getElementById("cs").selectedIndex].value;
		let a = document.getElementById("tz"+b);
		sendWs( {"ntpTime":true, "deviceTime":false, "TZC":a.options[a.selectedIndex].value} );
	}
	
	function setTimeSelect()	 {
		let b = document.getElementById("cs").options[document.getElementById("cs").selectedIndex].value;
		
		for(let i = 0; i < 10; i++)	{
			document.getElementById("tz"+i).parentElement.style.display = "none";
			if( i == b )	{
				document.getElementById("tz"+i).parentElement.style.display = "";	
			}
		}
		
	}
	
	function toggleOp(a, b)	  	{
		if(a == null)	{
			a = document.getElementById(b).style.opacity == 1 ? false : true;
		}
		if(a)	{
			document.getElementById(b).style.display="";
			setTimeout(function(b) {document.getElementById(b).style.opacity = 1;}, 10, b);
		}	else	{
			document.getElementById(b).style.opacity = 0;
			setTimeout(function(b) {document.getElementById(b).style.display="none";}, 200, b);
		}
	}
	
	function getNeColor()	{
		let canvas = document.getElementById("ccube");
		let rect = canvas.getBoundingClientRect();
		return getColor(global_hue, global_cx+rect.left, global_cy+rect.top, false);
	}

	function updateStyle(c) {
		let a;
		if (c === undefined) {
			let b = new Date();
			a = b.getHours();
		} else {
			a = c;
		}
		if (a > 5 && a < 18) {
			document.getElementById("sty").innerHTML =":root { --bg: #e4e4e4; --sec: #eee; --text: #444; --thumb: #9D9D9D; --hov: #e5e5e5;} ";
		} else {
			document.getElementById("sty").innerHTML =":root { --bg: #222; --sec: #333333; --text: #aaa; --thumb: #5e5e5e; --hov: #424242;} ";
		}
	}
	
	function draw(hue)	{
		let canvas	= document.getElementById("ccube");
		let w = canvas.scrollWidth;
		let h = canvas.scrollHeight;
		let ccube	= document.getElementById("ccube").getContext("2d");
		let infill = 4;
		hue /= 360.0;

		for (let i = 0; i <= h; i+=infill) {

			let a = HSVtoRGB(hue, 0, i/h);
			let b = HSVtoRGB(hue, 1, i/h);

			let g = ccube.createLinearGradient(0, i, w, i);
			g.addColorStop(0, "rgb("+a.r+","+a.g+","+a.b+")");
			g.addColorStop(1, "rgb("+b.r+","+b.g+","+b.b+")");
			ccube.fillStyle = g;
			ccube.fillRect(0, h-i, w, infill);	
		}
	}

	function getColor(h, x, y, send=true)	{
		let canvas = document.getElementById("ccube");
		let rect = canvas.getBoundingClientRect();
		let cursor = document.getElementById("c");

		let cx = x - rect.left;
		let cy = y - rect.top;

		cx = Math.min(Math.max(cx, 0), canvas.scrollWidth	);
		cy = Math.min(Math.max(cy, 0), canvas.scrollHeight	);

		global_cx = cx;
		global_cy = cy;

		let c = HSVtoRGB(h/360, cx / canvas.scrollWidth,(canvas.scrollHeight- cy) / canvas.scrollHeight);
		setPreview(c);
		cursor.style = "display: block; left: " + (cx + rect.left - 11) + "px; top: " + (cy + rect.top - 11) + "px; background-color: rgb(" + c.r + "," + c.g + "," + c.b + ");";
		if(send) 
			sendWs( { "rgb":[c.r, c.g, c.b] } );
		return  {r:c.r, g:c.g, b:c.b };
	}

	function setPreview(c)	{
		let r = c.r.toString(16).length === 1 ? "0"+String(c.r.toString(16)) : c.r.toString(16);
		let g = c.g.toString(16).length === 1 ? "0"+String(c.g.toString(16)) : c.g.toString(16);
		let b = c.b.toString(16).length === 1 ? "0"+String(c.b.toString(16)) : c.b.toString(16);
		updateIcon(c.r,c.g,c.b);
		document.getElementById("prev").value = "#"+r+""+g+""+b;
		document.getElementById("acc").innerHTML = " :root { --acc: rgb(" + c.r + "," + c.g + "," + c.b + "); }";
	}

	function findColor(r, g, b, send=true)	{
		
		let canvas = document.getElementById("ccube");
		let rect = canvas.getBoundingClientRect();

		let h, x, y;
		let c = RGBtoHSV(r, g, b);
		h = c.h * 360;
		if(h !== 0)	{
			global_hue = h;
		}
		document.getElementById("hue").value = global_hue;
		draw(global_hue);
		x = c.s*canvas.scrollWidth + rect.left;
		y = canvas.scrollHeight - c.v*canvas.scrollHeight + rect.top;
		getColor(global_hue, x, y, send);
	}

	function updateHue()	{
		let canvas = document.getElementById("ccube");
		doUpdate = false;
		global_hue = document.getElementById("hue").value;
		draw(global_hue);
		let c = HSVtoRGB(global_hue / 360, global_cx / canvas.scrollWidth, (canvas.scrollHeight - global_cy) / canvas.scrollHeight);
		findColor(c.r, c.g, c.b);
	}
	
	function updateIcon(r,g,b)	{
		let scale = 16;
		let canvas = document.createElement("canvas");
		canvas.width = scale;
		canvas.height = scale;
		let ctx = canvas.getContext("2d");

		ctx.fillStyle = "rgb(220,220,200)";
		ctx.fillRect(0, 0, scale, scale);
		ctx.fillStyle = "rgb("+r+","+g+","+b+")";
		ctx.beginPath();
		ctx.arc(scale/2,scale/2,scale/2-1,0,2*Math.PI);
		ctx.fill();

    	let link = document.querySelector("link[rel*='icon']") || document.createElement('link');
		link.type = 'image/x-icon';
		link.rel = 'shortcut icon';
		link.href = canvas.toDataURL();
		document.getElementsByTagName('head')[0].appendChild(link);
	}

	function findHex(s)	{
		s = s.slice(1,7);
		if(s.length < 6)	{
			s = "0"+String(s);
		}
		
		let r = parseInt(s.slice(0,2), 16);
		let g = parseInt(s.slice(2,4), 16);
		let b = parseInt(s.slice(4,6), 16);

		findColor(r,g,b);
	}
		
	function HSVtoRGB(h, s, v)	{
		let r, g, b, i, f, p, q, t;
		if (arguments.length === 1) {
			s = h.s, v = h.v, h = h.h;
		}
		i = Math.floor(h * 6);
		f = h * 6 - i;
		p = v * (1 - s);
		q = v * (1 - f * s);
		t = v * (1 - (1 - f) * s);
		switch (i % 6) {
			case 0:
				r = v, g = t, b = p;
				break;
			case 1:
				r = q, g = v, b = p;
				break;
			case 2:
				r = p, g = v, b = t;
				break;
			case 3:
				r = p, g = q, b = v;
				break;
			case 4:
				r = t, g = p, b = v;
				break;
			case 5:
				r = v, g = p, b = q;
				break;
			}
			return {
				r: Math.round(r * 255),
				g: Math.round(g * 255),
				b: Math.round(b * 255)
			};
		}

	function RGBtoHSV(r, g, b) {
				r /= 255, g /= 255, b /= 255;

			var max = Math.max(r, g, b), min = Math.min(r, g, b);
			var h, s, v = max;

			var d = max - min;
			s = max === 0 ? 0 : d / max;

			if (max === min) {
				h = 0;
			} else {
				switch (max) {
				case r: h = (g - b) / d + (g < b ? 6 : 0); break;
				case g: h = (b - r) / d + 2; break;
				case b: h = (r - g) / d + 4; break;
			}

			h /= 6;
			}

			return {
				h: h,
				s: s,
				v: v
			};
	}
		
	function revealPass()	{
		if (document.getElementById("p").type === "password")
			document.getElementById("p").type = "text";
		else
			document.getElementById("p").type = "password";
	}

	function triggerSnackbar(b)	{
		let a = document.getElementById(b);
		if( !a.className.includes("show"))	{
			a.className += " show";
			setTimeout( () => { a.className = a.className.replace("show", ""); }, 1400);
		}
	}
	
	

