var theme = detectTheme();
var debug = 0xFF;
var refreshTimer;
var refreshSpeed = 10000;
var progressTimer;
var saveReminder;

var solar_values = [0, 1];
var solar_labels = ['OFF', 'ON'];
var solar_adc_offset = 20; //with solar add offset

var pot_values = [10, 70, 120];
var pot_labels = ['Small', 'Medium', 'Large'];

var soil_values = [300, 780];
var soil_labels = ['Dry (Cactus)', 'Wet (Tropical)'];

var soil_type_values = [420, 700, 580, 680, 740];
var soil_pot_offsets = [[0,-10,-20], [0,0,0], [0,0,0], [5,10,0], [0,0,0]];
var soil_type_labels = ['Sand', 'Clay', 'Dirt', 'Loam', 'Moss'];

var connectMessage = 'Connect Plant to Computer';

var p = 1;
var os = '';
var chip = '';
var errorCode = '';
var xhr;

$(document).ready(function ()
{
    loadTheme();

    $('#slider-solar').ionRangeSlider({
        skin: 'big',
        from: 0,
        values: solar_labels,
        /* prettify: function (n) {
            return solar_labels[solar_values.indexOf(n)];
        },*/
        onFinish: function (data) {

            if(data.from_value == 'ON')
            {
                $.notify({ message: 'Place solar cell as close to direct sunlight as possible' }, { type: 'success' });
            }else{
                $.notify({ message: 'ATtiny will go into DEEP-SLEEP mode, saving battery power' }, { type: 'success' });
            }
        },
        onChange: function (e) {
            clearTimeout(saveReminder);
            saveReminder = setInterval(saveReminderCounter, 20000);
        }
    });

    $('#slider-pot').ionRangeSlider({
        skin: 'big',
        from: pot_values[0],
        min: pot_values[0],
        max: pot_values[2],
        step: 5,
        prettify: function (n) {
            //console.log(n);
            return (n * 2 / 10) + ' Seconds Pump'; //-1 extra 'wasted' time to prime
        },
        onChange: function (e) {
            clearTimeout(saveReminder);
            saveReminder = setInterval(saveReminderCounter, 20000);
        }
    });
    
    $('#slider-soil').ionRangeSlider({
        skin: 'big',
        from: soil_values[0],
        min: soil_values[0],
        max: soil_values[1],
        step: 1,
        prettify: function (n) {
            //console.log(n);
            if(n == soil_values[0]){
                return soil_labels[0];
            }else if(n == soil_values[1]){
                return soil_labels[1];
            }
            var ref = n / 1023;
            var v = 5 * (ref * 1.1); // Calculate Vcc from 5V
            var scientific = (ref * 100) / 2.0 //scientific value (actual H2O inside soil)
            return scientific.toFixed(0) + '% ' + (v).toFixed(2) + 'V (' + n + ')';
        },
        onChange: function (e) {
            clearTimeout(saveReminder);
            saveReminder = setInterval(saveReminderCounter, 20000);
        }
    });

    checkFirmwareUpdates();

    connectPlant(true);

    $('[data-toggle="tooltip"]').tooltip({
       container: 'body'
    });

    window.addEventListener('beforeunload', function (e) {
        if(debug != 0xEA)
            sendStop();
    });
});

function autoConfigure()
{
    var row = $('<div>', { class: 'row' });
    var img_src = ['sand.png', 'clay.png', 'dirt.png', 'loam.png', 'moss.png'];

    for (var i = 0; i < soil_type_labels.length; i++) {
        var h = $('<h6>').append(soil_type_labels[i]);
        var img = $('<img>', { class: 'img-thumbnail bg-light rounded-circle', src: 'img/' + img_src[i], 'data-dismiss': 'modal', onClick: 'setSoil(' + i + ')'});
        
        var col = $('<div>', { class: 'col' });
        col.append(h);
        col.append(img);
        row.append(col);
    }
    $('#Autoconfig').find('.modal-body').empty().append(row);
};

function setSoil(value)
{
    var slider_pot = $('#slider-pot').data('ionRangeSlider');
    //console.log(slider_pot.result.from);
    console.log(soil_pot_offsets[value]);

    for (var i = 0; i < pot_values.length; i++) {
        if(pot_values[i] >= slider_pot.result.from) {
            console.log(i);
            console.log(soil_pot_offsets[value][i]);
            slider_pot.update({
               from: slider_pot.result.from + soil_pot_offsets[value][i]
            });
            break;
        }
    }

    var slider_soil = $('#slider-soil').data('ionRangeSlider');
    slider_soil.update({
       from: soil_type_values[value]
    });
};

function stopLEDMonitor()
{
    var btn = $('#ledMonitor');
    btn.text('Enable LED Monitor');
    btn.attr('onclick', 'startConsole(0xEA,1)');
    btn.removeClass('btn-danger');
    btn.addClass('btn-primary');

	sendStop();
	debug = 0xFF;
}

function stopConsole()
{
    xhr.abort();
    clearTimeout(refreshTimer);

    var btn = $('#debugConsole');
    btn.text('Start Console');
    btn.attr('onclick', 'startConsole(0xEE,0)');

    sendStop();
    debug = 0xFF;
};

function USBTinyFirmware(step, element)
{
    var url = '';
    if(step == 2) {
        url = 'fuse=1&l=e1&h=0xdd'; //clock only
    }else if(step == 3) {
        url = 'fuse=1&l=e1&h=0x5d'; //clock + reset
    }else{
        var vusb = 'vusbtiny85';
        if (chip.indexOf('45') != -1)
            vusb = 'vusbtiny45';
        url = 'eeprom=flash&firmware=' + vusb;
        chip = '';
    }

    $.ajax('usb.php?' + url, {
        success: function(data) {
            if(data.indexOf('flash verified') != -1) {
                USBTinyFirmwareSuccess((step-1), element);
            }else{
                console.log(data);
            }
        }
    });
};

function USBTinyFirmwareSuccess(step, element)
{
    var n = $('#USBTinyFirmware').find('.modal-body').find('li')[step];

    $(element).removeClass('btn-primary');
    $(element).addClass('btn-success');
    element.disabled = true;

    $('<i>', { class: 'icons icon-ok text-success' }).appendTo(n);
};

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

async function testPump()
{
    $.ajax('usb.php?eeprom=write&offset=' + ee + ',' + e_deepSleep + '&value=' + parseInt(0xEB) + ',0', {
        success: function(data) {
            debug = 0xEB;
            $.notify({ message: 'Pump Test Ready ...' }, { type: 'success' });
            setTimeout(async function () {
                for (var i = 8; i > 0; i--) {
                    $.notify({ message: i }, { type: 'warning'});
                    await sleep(1000);
                }
                setTimeout(async function () {
                    $.notify({ message: 'Self Test Complete' }, { type: 'success'});
                    sendStop();
                }, refreshSpeed + ($('#slider-pot').data().from * 2 / 10) * 1000);
            }, 4000);
        }
    });
};

function sendStop()
{
	if(debug != 0xFF)
    	$.ajax('usb.php?eeprom=write&offset=' + ee + ',' + e_deepSleep + '&value=' + parseInt(0xFF) + ',' + deepSleep, { async: true});
};

function startConsole(hex,delay)
{
    if(chip.length == 0)
        connectPlant(false);

    if(chip.length > 0) {

        if(hex == 0xEE) {
            p = 0;
            progressTimer = setInterval(progressCounter, 12);
        }

        $.ajax('usb.php?eeprom=write&offset=' + ee + ',' + e_deepSleep + ',' + e_VSolar + ',' + e_moisture + ',' + e_water + ',' + e_errorCode + ',' + e_empty + ',' + e_log + '&value=' + parseInt(hex) + ',' + parseInt(delay) + ',255,255,255,255,0,0', {
            success: function(data) {
                consoleHex(data);
                if(data.length >= 64) {

                    if(data.indexOf('libusb: debug') != -1) {
                        $.notify({ message: 'LibUSB Driver Error' }, { type: 'danger' });
                    }else{
                        var s = data.split('\n');
                        var d = s[ee+1]; //+1 to skip debug path
                        console.log('EEPROM Debug Value:' + d);

                        if(d == parseInt(0xEA)) {
                            //console.log(data);
                            $.notify({ message: 'LED Monitor Enabled' }, { type: 'success' });
                            $.notify({ message: 'Battery usage will be higher!' }, { type: 'warning' });
                            setTimeout(function () {
				                $.notify({ message: 'Read moisture by counting LED blinks' }, { type: 'success', delay: 12000});
				            }, 4000);
                            setTimeout(function () {
				                $.notify({ message: 'Example: ---___^___- is 301 (quick pulse = 0)' }, { type: 'success', delay: 20000 });
				            }, 6000);
                            var btn = $('#ledMonitor');
                            btn.text('Disable LED Monitor');
                            btn.attr('onclick', 'startConsole(0xFF,' + deepSleep  + ')');
                            btn.removeClass('btn-primary');
                            btn.addClass('btn-danger');
                        }else if(d == parseInt(0xEE)) {
                            //console.log(data);
                            $.notify({ message: 'EEPROM Debug Enabled' }, { type: 'success' });
                            $('#debugOutput').empty();
                            var btn = $('#debugConsole');
                            btn.text('Stop Console');
                            btn.attr('onclick','startConsole(0xFF,' + deepSleep  + ')');
                            getEEPROM();
                        }else if(d == parseInt(0xFF)) {
                        	if(debug == 0xEA) {
                        		$.notify({ message: 'LED Monitor Disabled' }, { type: 'warning' });
                        		stopLEDMonitor();
                        	}else{
                        		$.notify({ message: 'EEPROM Debug Disabled' }, { type: 'warning' });
                            	stopConsole();
                            	//$.ajax('usb.php?reset=1');
                        	}
                        }
                        debug = d;
                    }
                }else{
                    $.notify({ message: 'Cannot read EEPROM' }, { type: 'danger' });
                }
            }
        });
    }else{
        $.notify({ message: connectMessage }, { type: 'danger' });
    }
};

function checkEEPROM()
{
    $.ajax('usb.php?eeprom=erase', {
        success: function(data) {
            consoleHex(data);
            if(data.length >= 64) {
                setTimeout(function () {
                    getEEPROMInfo(1);
                }, refreshSpeed);
            }else{
                $.notify({ message: 'Cannot read EEPROM' }, { type: 'danger' });
            }
        }
    });
};

function getEEPROMInfo(crc)
{
    xhr = $.ajax('usb.php?eeprom=read', {
        success: function(data) {
            consoleHex(data);
            if(data.length >= 64) {
                if(data.indexOf('libusb: debug') != -1) {
                    $.notify({ message: 'LibUSB Driver Error' }, { type: 'danger' });
                }else if(data.indexOf('initialization failed') != -1) {
                    $.notify({ message: 'Check USB Cable' }, { type: 'danger' });
                }else{
                    var s = data.split('\n');
                    var info = $('#debugInfo').empty();
                    info.append('Hardware Chip: ' + chip + '\n');

                    debug = s[ee+1];
                    if(debug == 0xEA) {
                        var btn = $('#ledMonitor')
                        btn.text('Disable LED Monitor');
                        btn.attr('onclick','startConsole(0xFF,' + deepSleep  + ')');
                        btn.removeClass('btn-primary');
                        btn.addClass('btn-danger');
                    }else if(debug == 0xEE) {
                    	$.notify({ message: 'EEPROM Debug was left enabled' }, { type: 'warning' });
                    	$.notify({ message: 'Stop Debug before disconnecting plant' }, { type: 'warning' });
                        stopConsole();
                    }

                    var vreg = HexShift(s,e_VReg);
                    var vchip = 'TP4056';
                    if(vreg == 1) {
                        vchip = 'WS78L05';
                    }else if(vreg== 2) {
                        vchip = 'LM2731';
                    }else if(vreg == 3) {
                        vchip = 'TPL5110';
                    }
                    info.append('Solar Regulator: ' + vchip + '\n');
                    info.append('Firmware Version: ');

                    var fv = s[e_versionID+1] + '0';
                    if(parseInt(fv) > 0) {
	                    info.append(fv.charAt(0) + '.' + fv.charAt(1));
	                }else{
	                	info.append('1.0');
	                }

                    var sl = HexShift(s,e_runSolar);
                    var pt = HexShift(s,e_potSize);
                    var so = HexShift(s,e_moisture);
                    var sm = HexShift(s,e_moistureLimit);
                    var d = s[ee+1]; //+1 to skip debug path

                    console.log('Solar: ' + sl);
                    console.log('Pot: ' + pt);
                    console.log('Soil: ' + so + ' (' + sm + ')');

                    if(sl == 0 && pt == 0 && sm == 0 && s[s.length-2] == '255') {
                        if(crc == undefined) {
                            $.notify({ message: 'EEPROM is Corrupt ...Trying to Fix' }, { type: 'warning' });
							$.ajax('usb.php?eeprom=write&offset=' + e_runSolar + ',' + e_potSize + ',' + e_moistureLimit + '&value=0,20,660', {
					            success: function(data) {
					            	getEEPROMInfo(1);
					            }
					        });
                            //checkEEPROM();
                        }else{
                            $.notify({ message: 'Cannot fix EEPROM' }, { type: 'danger' });
                            $.notify({ message: 'Flashing Firmware ...' }, { type: 'warning' });

                            $.ajax('usb.php?eeprom=flash&firmware=' + chip, {
    				            success: function(data) {
    				                console.log(data);
                                    if(data.indexOf('flash verified') != -1) {
                                        $.notify({ message: 'Firmware Fixed!' }, { type: 'success' });
                                    }else{
                                        $.notify({ message: 'Cannot fix Firmware, try manually (.hex File)' }, { type: 'danger' });
                                    }
    				            }
    				        });
                        }
                    }else if (crc == 1) {
                        $.notify({ message: 'EEPROM Fixed!' }, { type: 'success' });
                    }else if(data.indexOf('avrdude:') != -1) {
						$.notify({ message: 'Cannot Syncronize (unplug USB and plug back in)' }, { type: 'danger' });
                    }else if(so > 870 && (so-sm) > 400) {
                        $.notify({ message: 'Detecting Excess Moisture!' }, { type: 'danger' });
                        if(pt > 50) {
                            $.notify({ message: 'Lower Pot Size value' }, { type: 'success' });
                            $.notify({ message: 'Adjust sensor to soil height' }, { type: 'success' });
                        }else{
                            $.notify({ message: 'Compact soil water channels' }, { type: 'success' });
                            $.notify({ message: 'Move sensor away from outlet' }, { type: 'success' });
                        }
                    }

                    var instance = $('#slider-solar').data('ionRangeSlider');
                    instance.update({
                       from: solar_values.indexOf(sl)
                    });

                    var instance = $('#slider-pot').data('ionRangeSlider');
                    instance.update({
                        from: pt
                    });

                    var instance = $('#slider-soil').data('ionRangeSlider');
                    instance.update({
                       from: sm
                    });
                }
            }else{
                $.notify({ message: 'Cannot read EEPROM' }, { type: 'danger' });
            }
        }
    });
};

function getEEPROM()
{
    clearTimeout(refreshTimer);

    $.ajax('usb.php?eeprom=read', {
        async: true,
        //timeout: refreshSpeed,
        success: function(data) {
            consoleHex(data);

            if(data.length >= 64)
            {
                var s = data.split('\n');
                var debug = $('#debugOutput').empty();

                var sl = HexShift(s,e_VSolar);
                var so = HexShift(s,e_moisture);
                var sm = HexShift(s,e_moistureLimit);
                var water = HexShift(s,e_water);
                var empty = HexShift(s,e_empty);
                var err = HexShift(s,e_errorCode);

                //console.log(HexShift(s,26));
                console.log('Solar:' + sl);
                console.log('Sensor:' + so + '(' + sm + ')');
                console.log('Water:' + water);
                console.log('Empty:' + empty);
                console.log('Log:' + HexShift(s,e_log));
                console.log('Error:' + err);

                if(sl > 0) {
                    var ref = sl / 1023;
                    var v = (1125300 / (ref * 1.1)) / 1000; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
                    debug.append('Solar Voltage: ' + v.toFixed(2) + 'V (' + (ref * 100).toFixed(0) + '%)\n');
                }

                var ref = so / 1023;
                var v = 5 * (ref * 1.1); // Calculate Vcc from 5V
                var scientific = (ref * 100) / 2.0 //scientific value (actual H2O inside soil)
                debug.append('Soil Moisture: ' + v.toFixed(2) + 'V (' + scientific.toFixed(0) + '%)\n');

                if (so < 0.2) {
                    debug.append('Error Code: Sensor not in Soil\n');
                }

                if(err == 0 && errorCode == 8) {
                    //Debug only
                    //$.ajax('usb.php?eeprom=write&offset=' + e_log + '&value=0');

                    debug.append('Error Code: Water Refilled!\n');
                }

                if(empty == 3) {
                    debug.append('Error Code: Overwater Protection\n');
                }

                if(empty == 11) {
                    debug.append('Error Code: No Water (Sensorless)\n');
                }
                if(empty == 12) {
                    debug.append('Error Code: No Water (Sensored)\n');
                }

                if(water > 10 && water < 255) {
                    debug.append('Error Code: Overwater Protection\n');
                }
                
                var ai = calcNextWaterCycle(so, sm, $('#slider-pot').data().from); //Predictive AI
                debug.append('Next Water Cycle: ' + ai.toFixed(0) + ' days ' + (ai * 1000 >> 10) + ' hours\n');

                errorCode = err;

            }else{
                $.notify({ message: 'Cannot read EEPROM' }, { type: 'danger' });
            }
            refreshTimer = setTimeout(function () {
                getEEPROM();
            }, refreshSpeed);
        },
        error: function() {
            console.log('Timeout: ' + refreshSpeed);

            refreshSpeed += 500;
            refreshTimer = setTimeout(function () {
                getEEPROM();
            }, refreshSpeed);
        }
    });
};

function calcNextWaterCycle(start, stop, size)
{
    /* All calculations are approximate */

	var days = 0;
	if((start - stop) > 0) {

		//Calculate Days
		while (start > stop) {
			var c = 180;
			if(start >= 850) { //...slow start
				c = 24;
			}else if(start >= 800) {
				c = 40 + size;
			}else if(start >= 700) {
				c = 50 + size;
			}else if(start >= 600) {
				c = 80 + size;
			}else if(start >= 500) { //..faster ending
				c = 100 + size;
			}
			start -= c;
			days++;

            //Calculate Hours
            //TODO:
 		}
 		console.log('Days: ' + days);
 	
 		var hours = Math.abs(start-stop);
		console.log('Hours: ' + hours);
	}
	return days;
};

function connectPlant(async)
{
	clearTimeout(refreshTimer);

    if(chip == '') {
        $.ajax('usb.php?connect=plant', {
            async: async,
            success: function(data) {
                console.log(data);
                if(data.indexOf('ATtiny') != -1) {
                    if (data.indexOf('45') != -1) {
                        EEPROM_T85(128);
                        refreshSpeed = 10000;
                	}else if (data.indexOf('85') != -1) {
                		EEPROM_T85(256);
                		refreshSpeed = 12000; //EEPROM takes longer to read, do not force early interrupt
                	}
                    var s = data.split('\n');
                    chip = s[0];
                    $('.icon-chip').attr('data-original-title', '<h6 class="text-white">' + chip + '</h6>');

                    if(data.indexOf('USBTiny') != -1) {
                        $('#USBTinyFirmware').modal();
                    }else{
                        $.notify({ message: 'Plant Connected' }, { type: 'success' });
                        getEEPROMInfo();
                    }
                }else if(data == 'fix') {
                    $.notify({ message: 'Power cycle USB' }, { type: 'warning' });
                    /*
                    $.notify({ message: '... Fixing USB Driver' }, { type: 'danger' });
                    $.ajax('usb.php?driver=fix', {
                        success: function(data) {
                            if(data == 'ok') {
                                $.notify({ message: 'USB Driver Installed' }, { type: 'success' });
                            }
                        }
                    });
                    */
                }else if(data == 'sck') {
                    $.notify({ message: '... Waiting for Plant to Connect' }, { type: 'warning' });
                }else if(data.indexOf('dyld: Library not loaded') != -1) {
                    $.notify({ message: 'MacOS LibUSB Issue' }, { type: 'danger' });
                    $.notify({ message: data }, { type: 'warning' });
                }else{
                    setTimeout(function () {
                        connectPlant();
                    }, 4000);
                }
            }
        });
    }
};

function updateFirmware(emergency)
{
    if(chip.length == 0)
        connectPlant(false);

    if(chip.length > 0 || emergency === true) {

        //stopConsole();
        $('.fileUpload').trigger('click');
        $('.fileUpload').change(function() {
            p = 0;
            progressTimer = setInterval(progressCounter, 60);
            $('#formFirmware').submit();
        });
    }else{
        $.notify({ message: connectMessage }, { type: 'danger' });
        $.notify({ message: '<a href="#" onClick="updateFirmware(true)">Emergency Flash</a>' }, { type: 'warning' });
    }
};

function saveSettings()
{
    if(chip.length == 0)
        connectPlant(false);

    if(chip.length > 0) {
        $.notify({ message: '... Saving Settings' }, { type: 'warning' });
        clearTimeout(saveReminder);

        //calibrate with solar
        var moisture = $('#slider-soil').data().from;
        if($('#slider-solar').data().from > 0){ moisture += solar_adc_offset; }
        
        $.ajax('usb.php?eeprom=write&offset=' + e_runSolar + ',' + e_potSize + ',' + e_moistureLimit + '&value=' + $('#slider-solar').data().from + ',' + $('#slider-pot').data().from + ',' + moisture , {
            success: function(data) {
                consoleHex(data);
                if(data.indexOf('initialization failed') != -1) {
                    $.notify({ message: 'Check USB Cable' }, { type: 'danger' });
                }else if(data.indexOf('eeprom verified') != -1) {
                    if(data.indexOf('Input/output error') != -1) {
                        $.notify({ message: 'EEPROM Saved ...' }, { type: 'success' });
                        $.notify({ message: 'Detected Input/Output Errors' }, { type: 'warning' });
                    }else{
                        $.notify({ message: 'Happy Plant &#127807;' }, { type: 'success' });
                    }
                }else if(data.indexOf('verification error') != -1) {
                    $.notify({ message: 'EEPROM Verification Error ...' }, { type: 'danger' });
                    if (navigator.appVersion.indexOf('Linux') !=-1 ) {
                        $.notify({ message: 'AVRDUDE v6.3 is not <a href="https://raw.githubusercontent.com/dimecho/ATtiny13-Plant/master/usbtiny/avrdude-6.3-usbtiny.patch">patched</a>' }, { type: 'warning' });
                        $.notify({ message: 'Patch or downgrade to AVRDUDE v5.11.1' }, { type: 'warning' });
                    }else{
                        $.notify({ message: 'Check MISO Resistor' }, { type: 'warning' });
                    }
                }else if(data.indexOf('Broken pipe') != -1) {
                    $.notify({ message: 'EEPROM Saved ...' }, { type: 'success' });
                    $.notify({ message: 'Power cycle USB' }, { type: 'warning' });
                }else if(data.indexOf('Input/output error') != -1) {
                    $.notify({ message: 'Input/Output Error' }, { type: 'danger' });
                }else{
                    $.notify({ message: 'Chip Reset, Try Again' }, { type: 'danger' });
                }
                //stopConsole();
            }
        });
    }else{
        $.notify({ message: connectMessage }, { type: 'danger' });
    }
};

function detectTheme()
{
    var t = getCookie('theme');
    if(t == undefined) {
        if (window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches) {
            return '.slate';
        }else{
            return ''
        }
    }
    return t;
};

function switchTheme(element,dark,light) {
     if(theme == '') {
        var e = $(element + '.' + dark);
        e.removeClass(dark);
        e.addClass(light);
    }else{
        var e = $(element + '.' + light);
        e.removeClass(light);
        e.addClass(dark);
    }
};

function setTheme() {
    if(theme == '.slate') {
        theme = '';
    }else{
        theme = '.slate';
    }
    setCookie('theme', theme, 1);
    loadTheme();
};

function loadTheme() {
    if(theme == '.slate') {
        $('link[title="main"]').attr('href', 'css/bootstrap.slate.css');
        $('.icon-day-and-night').attr('data-original-title', '<h6 class="text-white">Light Theme</h6>');
        $('h1').addClass('text-white');
    }else{
        $('link[title="main"]').attr('href', 'css/bootstrap.css');
        $('.icon-day-and-night').attr('data-original-title', '<h6 class="text-white">Dark Theme</h6>');
        $('h1').removeClass('text-white');
    }
    switchTheme('i.icons','text-white','text-dark');
    switchTheme('div','bg-primary','bg-light');
    switchTheme('div','text-white','text-dark');
    switchTheme('img','bg-secondary','bg-light');
};

function checkFirmwareUpdates()
{
   var check = Math.random() >= 0.5;
    if (check === true)
    {
        $.ajax('https://raw.githubusercontent.com/dimecho/ATtiny13-Plant/master/gui/Web/firmware/version.txt', {
            async: true,
            success: function success(data) {
                try {
                    var split = data.split('.');
                    var version = parseFloat(split[0]);
                    var build = parseFloat(split[1]);

                    $.ajax('firmware/version.txt', {
                        async: true,
                        success: function success(data) {

                            var _split = data.split('.');
                            var _version = parseFloat(_split[0]);
                            var _build = parseFloat(_split[1]);

                            if(version > _version || build > _build)
                            {
                                var url = 'https://github.com/dimecho/ATtiny13-Plant/releases/download/';
                                if(os === 'mac'){
                                    url += version + 'ATtiny13.Plant.dmg';
                                }else if(os === 'windows'){
                                    url += version + 'ATtiny13.Plant.Windows.zip';
                                }else if(os === 'linux'){
                                    url += version + 'ATtiny13.Plant.Linux.tgz';
                                }
                                $.notify({
                                    icon: 'icon icon-download',
                                    title: 'New Firmware',
                                    message: 'Available <a href="' + url + '" target="_blank">Download</a>'
                                }, {
                                    type: 'success'
                                });
                            }
                        }
                    });
                } catch(e) {}
            }
        });
    }
};

function progressCounter() {
    p++;
    $('.progress-bar').css('width', p + '%');
    if(p == 100) {
        clearInterval(progressTimer);
    }
};

function saveReminderCounter() {
    $.notify({ message: 'Don\'t forget to Save Settings!' }, { type: 'warning' });
};