let ctx = Runner.instance_;
let doLoop = true;
let delay = 5;
let p_y = [100, 75, 50];
let itm = 10;

let Jconfig = [];

let     jmpEvt = new Event("keydown");
		jmpEvt.keyCode=32;
		jmpEvt.which=jmpEvt.keyCode;
		jmpEvt.altKey=false;
		jmpEvt.ctrlKey=false;
		jmpEvt.shiftKey=false;
		jmpEvt.metaKey=false;
		jmpEvt.bubbles=true;
let     S_dckEvt = new Event("keydown");
		S_dckEvt.keyCode=40;
		S_dckEvt.which=S_dckEvt.keyCode;
		S_dckEvt.altKey=false;
		S_dckEvt.ctrlKey=false;
		S_dckEvt.shiftKey=false;
		S_dckEvt.metaKey=false;
		S_dckEvt.bubbles=true;
let     E_dckEvt = new Event("keyup");
		E_dckEvt.keyCode=40;
		E_dckEvt.which=E_dckEvt.keyCode;
		E_dckEvt.altKey=false;
		E_dckEvt.ctrlKey=false;
		E_dckEvt.shiftKey=false;
		E_dckEvt.metaKey=false;
		E_dckEvt.bubbles=true;
let     re = new Event("keyup");
		re.keyCode=13;
		re.which=re.keyCode;
		re.altKey=false;
		re.ctrlKey=false;
		re.shiftKey=false;
		re.metaKey=false;
		re.bubbles=true;

document.getElementsByClassName("runner-container")[0].addEventListener("click", () => {ctx.gameOver(); restart(); }, false);

let increment = 0;

let gamectx = document.getElementsByClassName("runner-canvas")[0].getContext("2d");

let c = document.createElement("canvas");
c.width = document.getElementsByClassName("runner-canvas")[0].width;
c.height = 200;
let cctx = c.getContext("2d");
document.getElementById("main-content").appendChild(c);
cctx.moveTo(0, 0);
resetBox(1);

let dv = document.createElement("p");
document.getElementById("main-content").appendChild(dv);
let ds = document.createElement("p");
document.getElementById("main-content").appendChild(ds);
let sc = document.createElement("p");
document.getElementById("main-content").appendChild(sc);

let iterator = 0;

let def_margin = 110;
let margin = def_margin;
let steady_speed = 13.0009;
//let speed_mult = 0.0003;
let speed_mult = 0.002;
//let random_mult = 0.00063;
let random_mult = 0.015;
let config = {"margin":def_margin,"steady_speed":steady_speed,"speed_mult":speed_mult,"random_mult":random_mult, "score":0};
console.log(JSON.stringify(config));

for(let i = 0; i < 10; i++)	{
	Jconfig.push( {"margin":margin+Math.random(),"steady_speed":steady_speed+Math.random(),"speed_mult":speed_mult - 0.001 * Math.random(),"random_mult":random_mult+Math.random(), "score":0} );	
}

init(Jconfig[iterator]);

//restart();

function init(cfg)	{
	let cfg_obj = cfg;
		//let cfg_obj = JSON.parse(cfg);
		console.log(cfg_obj);
		margin = cfg_obj.margin;
		steady_speed = cfg_obj.steady_speed;
		speed_mult = cfg_obj.speed_mult;
		random_mult = cfg_obj.random_mult;
		increment = 0;
		iterator >= Jconfig.length ? iterator = 0 : iterator++;
		jump();
		setTimeout( () => { main(); }, 500 );
}

function restart()	{

		let s = getScore();
		//Jconfig.push( {"margin":margin,"steady_speed":steady_speed,"speed_mult":speed_mult,"random_mult":random_mult, "score":s} );
		//console.log( {"margin":def_margin,"steady_speed":steady_speed,"speed_mult":speed_mult,"random_mult":random_mult, "score":s} );
		randomize(iterator);
		Jconfig.sort( comp );
		console.log(Jconfig.length);
		console.log(Jconfig[iterator]);
		

		//margin = def_margin + Math.random();
		//steady_speed = 13.0009  + Math.random();
		//speed_mult = 0.002 + 0.01 * Math.random();
		//random_mult = 0.015  + 0.01 * Math.random();
		
		//config = {"margin":margin,"steady_speed":steady_speed,"speed_mult":speed_mult,"random_mult":random_mult, "score":0};
		resetBox(1);
				
		document.dispatchEvent(re);
		doLoop = true;

		init(Jconfig[iterator]);
}

function main() {

		if(ctx.crashed) {
				setTimeout(function() {
						restart();
				}, 1000)
				return;
		}

		if(doLoop && !ctx.crashed)      {
				if( !(ctx.currentSpeed >= 13.00099999999789) )       {
						margin += (speed_mult * ctx.currentSpeed);
						//margin -= (random_mult * Math.random());
				}
				//console.log(margin);
				addBox(margin-def_margin);
				sc.innerHTML = getScore();
				if(imminent() ) {
						let type = ctx.horizon.obstacles[0].typeConfig.type;
						//console.log(type);

						if( type.includes("CACTUS") )   {
								jump();
						} else if( type.includes("PTERODACTYL") )       {
								let cpy = ctx.horizon.obstacles[0].yPos;
								switch( cpy )   {
										case p_y[0]:    jump();
																		break;
										case p_y[1]:    jump();

																		break;
										case p_y[2]:    break;
								}
						}

				}
				try     {
						if(ctx.horizon.obstacles[0].yPos !== p_y[1])    {
								EndDuck();
						}
				} catch(e)      {}

				setTimeout(function() {
						main();
				}, delay);
		}
}

function jump() {
		document.dispatchEvent(jmpEvt);
}
function duck() {
		document.dispatchEvent(S_dckEvt);
}
function EndDuck()      {
		document.dispatchEvent(E_dckEvt);
}

function map (num, in_min, in_max, out_min, out_max) {
	return (num - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

function imminent()     {
		try     {
				let tx = ctx.tRex.xPos + margin
				let ox = ctx.horizon.obstacles[0].xPos
				//console.log(tx, ox);
				return tx > ox;
		} catch(e)      {
				return null;
		}
}

function getScore()     {
		let a = ctx.distanceMeter.digits;
		let score = 0;
		a.forEach( (element) => {score += element});
		return parseInt(score);
}

function addBox(y)      {
		//console.log(y);
		dv.innerHTML = y;
		ds.innerHTML = ctx.currentSpeed;
		cctx.fillStyle="black";
		cctx.fillRect(increment, y + (c.height/4) , 1, 1);
		cctx.fillStyle="green";
		cctx.fillRect(increment, map( ctx.currentSpeed,0,13.000999999997896,0,150 )+(c.height/4)+6.1 , 1, 1);
		increment > c.width-1 ? resetBox(0.35): increment++;
}

function resetBox(opa)     {
		increment = 0;
		cctx.fillStyle="rgba(255, 255, 255, "+opa+")";
		cctx.fillRect(0,0,c.width,c.height);
}

function comp(a,b)	{

	a = a.score;
	b = b.score;
	if( a > b )	
		return -1;
	if( a < b )
		return 1;
	return 0; 

}

function randomize(i)	{
	console.log("randomizing...");
	console.log(Jconfig[i]);
	let a = Jconfig[i];
	if(a.score > 1000)	{	//FIXME
		return;
	}	else	{
		a.margin = a.margin - Math.random();
		a.steady_speed = a.steady_speed  - Math.random();
		a.speed_mult = a.speed_mult - 0.001 * Math.random();
		a.random_mult =  a.random_mult + 0.001 * Math.random();
		Jconfig[i] = {"margin":a.margin,"steady_speed":a.steady_speed,"speed_mult":a.speed_mult,"random_mult":a.random_mult, "score":0};
	}
	console.log(Jconfig[i]);
}