
	
	let connection = new WebSocket('ws://192.168.178.90/ws');
	let doUpdate = false;
	let cx, cy;
	
	/*document.onmousedown = function() { doUpdate = true; getPos();	};*/
	window.onmouseup	= function() { doUpdate = false; getPos(); };
	window.onmousemove	= getPos;
	window.touchend		= function() { doUpdate = true; getPos();	};
	window.touchstart	= function() { doUpdate = false; getPos(); };
	window.touchmove	= getPos;
	
	window.onresize  = function() { getPos(cx, cy);};
	window.onscroll  = function() { getPos(cx, cy);};
		
	function toSlide(index)	{
		document.getElementById("slide1").style="display:none;";
		document.getElementById("slide2").style="display:none;";
		document.getElementById("slide3").style="display:none";
		document.getElementById(index).style="display:block;";
	}

	function showPwd()	{
		if( document.getElementById("Pwd").type == "password" )	{
			document.getElementById("Pwd").type = "text";
		}	else	{
			document.getElementById("Pwd").type = "password";
		}
	}

	function submit(s)	{
		let a = false;
		let ssid = document.getElementById("SSID").value;
		let pwd = document.getElementById("Pwd").value;
		if(ssid != "" || pwd != "")
			a = true;
		document.getElementById("a").href = "/done?W=" + a + "&s=" + ssid + "&p=" + pwd;
		document.getElementById("a").click();
	}

	function draw() {
		let canvas = document.getElementById("ccube");
		let w = canvas.scrollWidth;
		let h = canvas.scrollHeight;
		let ctx = canvas.getContext("2d");

		for(let i = 0; i <= w; i++ )	{
			for(let j = 0; j <= h; j++ )	{
				let c = HSVtoRGB( i/w, .9, j/h);

				ctx.fillStyle = 'rgb(' + c.r + ', ' + c.g + ', '+ c.b +')';
				ctx.fillRect(i,j,1,1);
			}
		}
		getPos(w*0.5, h*0.5);
		console.log(w,h);
	}
	
	function mp(rect)	{
		let	x = event.clientX - rect.left;
		let	y = event.clientY - rect.top;
		return {
			x: x,
			y: y
		};
	}
	
	function getPos(ax, ay)	{
		let use = Boolean( ax+1 && ay+1 );
		if(doUpdate || use)	{
			let prev = document.getElementById("prev");
			let canvas = document.getElementById("ccube");
			let rect = canvas.getBoundingClientRect();
			let cursor = document.getElementById("c");
	
			if(use)	{
				cx = ax;
				cy = ay;
			}	else	{
	
				let a = mp(rect);
				cx = a.x;
				cy = a.y;
			}			
			cx = Math.min(Math.max(cx, 0), canvas.scrollWidth);
			cy = Math.min(Math.max(cy, 0), canvas.scrollHeight);

			let c = HSVtoRGB( cx / canvas.scrollWidth , 1 , cy / canvas.scrollHeight);
			let ctx = prev.getContext("2d");
			ctx.fillStyle = "rgb(" + c.r +"," + c.g +","+ c.b +")";
			ctx.fillRect(0,0, 1000,1000);
			
			cursor.style = "left: "+(cx+rect.left-11)+"px; top: "+(cy+rect.top-11)+"px; background-color: rgb("+c.r+","+c.g+","+ c.b+");";
			connection.send( "r="+c.r +"?g=" + c.g +"?b="+ c.b );
	
		}
		return false;
	}

	function HSVtoRGB(h, s, v)	 {
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
			case 0: r = v, g = t, b = p; break;
			case 1: r = q, g = v, b = p; break;
			case 2: r = p, g = v, b = t; break;
			case 3: r = p, g = q, b = v; break;
			case 4: r = t, g = p, b = v; break;
			case 5: r = v, g = p, b = q; break;
		}
		return {
			r: Math.round(r * 255),
			g: Math.round(g * 255),
			b: Math.round(b * 255)
		};
	}




Spry.Utils.addLoadListener(function() {
	var onloadCallback = function(e){ toSlide('slide1'); draw(); getPos(); }; // body
	Spry.$$("body").addEventListener('load', onloadCallback, false).forEach(function(n){ onloadCallback.call(n); });
	Spry.$$("#button1").addEventListener('click', function(e){ toSlide('slide2'); }, false);
	Spry.$$("#button2").addEventListener('click', function(e){ submit(true); }, false);
	Spry.$$("#input1").addEventListener('click', function(e){ showPwd() }, false);
	Spry.$$("#button3").addEventListener('click', function(e){ toSlide('slide3'); }, false);
	Spry.$$("#button4").addEventListener('click', function(e){ toSlide('slide1'); }, false);
	Spry.$$("#button5").addEventListener('click', function(e){ submit(true); }, false);
	Spry.$$("#button6").addEventListener('click', function(e){ toSlide('slide2'); }, false);
	Spry.$$("#div1").addEventListener('mousemove', function(e){ getPos(); }, false);
	Spry.$$("#ccube").addEventListener('mousedown', function(e){ doUpdate = true; getPos(); }, false);
});
