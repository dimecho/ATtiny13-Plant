var deepSleep = 50; //8 seconds x 50 = 6.5 min
/*==========
EEPROM Map
============*/
var e_versionID      =  parseInt(0x00);
var e_moistureLimit  =  parseInt(0x02);
var e_potSize        =  parseInt(0x04);
var e_runSolar       =  parseInt(0x06);
var e_deepSleep      =  parseInt(0x08);
var ee               =  parseInt(0xA); //Debug -> 0xFF = normal run, 0xEA = led monitor, 0xEE = console, 0xEF = graph
var e_VReg           =  parseInt(0xC);
var e_serial         =  parseInt(0xE); //Serial #
var e_moisture       =  parseInt(0x1A);
var e_water          =  parseInt(0x1C);
var e_VSolar         =  parseInt(0x1E);
var e_errorCode      =  parseInt(0x2A);
var e_empty          =  parseInt(0x38);
var e_log            =  parseInt(0x3A);

function EEPROM_T85(offset)
{
    console.log('EEPROM Offset: ' + offset);
	/*
	Strange bug, ATtiny85 eeprom is large (uint16_t) starts write @ address 0x100 (256)

	It is flash-costly to handle uint16_t in attiny ...so offset it here

	Wastefull, but backwards compatible with ATtiny13
	*/

    if(e_versionID == 0) {
    	ee += offset;
        e_serial += offset;
    	e_versionID += offset;
    	e_moistureLimit += offset;
    	e_potSize += offset;
    	e_runSolar += offset;
    	e_deepSleep += offset;
    	e_VReg += offset;
    	e_VSolar += offset;
    	e_moisture += offset;
    	e_water += offset;
    	e_errorCode += offset;
    	e_empty += offset;
    	e_log += offset;
    }
};

function consoleHex(response)
{
    var _data = '';

    var s = response.split('\n');
    for (i = 0; i < s.length-1; i++) {
        if(s[i].indexOf('.eeprom') != -1) { //debugging path
            _data +=  s[i] + '\n';
        }else{
            _data += '[' +  parseInt(i-1).toString(16) + '] ' + s[i] + '\n';
        }
    }

    console.log(_data);
};

function HexShift(hex,bit)
{
    if(hex[0].indexOf('.eeprom') != -1) { //debugging path
        bit++; //move one over
    }

    return hex[bit] | hex[bit+1] << 8;
    /*
    if(hex[bit] == 255) {
        return 0;
    }else if(hex[bit+1] == 255) {
        return parseInt(hex[bit]);
    }else{
        return hex[bit] | hex[bit+1] << 8;
    }
    return 0;
    */
};

function deleteCookie(name, path, domain) {

  if(getCookie(name)) {
    document.cookie = name + '=' +
      ((path) ? ';path='+path:'')+
      ((domain)?';domain='+domain:'') +
      ';expires=Thu, 01 Jan 1970 00:00:01 GMT';
  }
};

function setCookie(name, value, exdays) {

    var exdate = new Date();
    exdate.setDate(exdate.getDate() + exdays);
    var c_value = escape(value) + (exdays == null ? '' : '; expires=' + exdate.toUTCString());
    document.cookie = name + '=' + c_value;
};

function getCookie(name) {
    
    var i,
        x,
        y,
        ARRcookies = document.cookie.split(';');

    for (var i = 0; i < ARRcookies.length; i++) {
        x = ARRcookies[i].substr(0, ARRcookies[i].indexOf('='));
        y = ARRcookies[i].substr(ARRcookies[i].indexOf('=') + 1);
        x = x.replace(/^\s+|\s+$/g, '');
        if (x == name) {
            return unescape(y);
        }
    }
};