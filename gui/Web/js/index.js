var theme = detectTheme();
var refreshTimer;
var refreshSpeed = 8000;
var progressTimer;
var deepSleep = 40; //8seconds x 40 = 5.5 min
var moistureOffset = 260; //this is caused by double 10k pullup resistor (on both sides "out" and sense "in")
var saveReminder;

var solar_values = [0,1];
var solar_labels = ["OFF", "ON"];

var pot_values = [10,20,40];
var pot_labels = ["Small", "Medium", "Large"];

var soil_values = [220, 480];
var soil_labels = ["Dry (Cactus)", "Wet (Tropical)"];

var soil_type_values = [380, 320, 380, 280, 460];
var soil_type_labels = ["Sand", "Clay", "Dirt", "Loam", "Moss"];

var connectMessage = "Connect Plant to Computer";

var p = 1;
var os = "";
var chip = "";
var errorCode = "";

/*==========
EEPROM Map
============*/
var ee = parseInt(0x01);
var e_versionID = parseInt(0x00);
var e_sensorMoisture = parseInt(0x02);
var e_potSize = parseInt(0x04);
var e_runSolar = parseInt(0x06);
var e_deepSleep = parseInt(0x08);
var e_VReg = parseInt(0xA);
var e_VSolar = parseInt(0xC);
var e_moisture = parseInt(0x1A);
var e_water = parseInt(0x1C);
var e_errorCode = parseInt(0x2A);
var e_empty = parseInt(0x38);
var e_log = parseInt(0x3A);

$(document).ready(function ()
{
    loadTheme();

    $("#slider-solar").ionRangeSlider({
        skin: "big",
        from: 0,
        values: solar_labels,
        /* prettify: function (n) {
            return solar_labels[solar_values.indexOf(n)];
        },*/
        onFinish: function (data) {

            if(data.from_value == "ON")
            {
                $.notify({ message: "Place solar cell as close to direct sunlight as possible" }, { type: "success" });
            }else{
                $.notify({ message: "ATtiny will go into DEEP-SLEEP mode, saving battery power" }, { type: "success" });
            }
        },
        onChange: function (e) {
            clearTimeout(saveReminder);
            saveReminder = setInterval(saveReminderCounter, 20000);
        }
    });

    $("#slider-pot").ionRangeSlider({
        skin: "big",
        from: 0,
        values: pot_labels,
        onChange: function (e) {
            clearTimeout(saveReminder);
            saveReminder = setInterval(saveReminderCounter, 20000);
        }
    });
    
    $("#slider-soil").ionRangeSlider({
        skin: "big",
        from: 0,
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
            return n;
        },
        onChange: function (e) {
            clearTimeout(saveReminder);
            saveReminder = setInterval(saveReminderCounter, 20000);
        }
    });

    $("#slider-soil-type").ionRangeSlider({
        skin: "big",
        from: 0,
        values: soil_type_labels
    });

    checkFirmwareUpdates();

    connectPlant(true);

    $('[data-toggle="tooltip"]').tooltip();

    window.addEventListener("beforeunload", function (e) {
        sendStop();
    });
});

function autoConfigure()
{
    var row = $("<div>", { class: "row" });
    var img_src = ["sand.png", "clay.png", "dirt.png", "loam.png", "moss.png"];

    for (var i = 0; i < soil_type_labels.length; i++) {
        var h = $("<h6>").append(soil_type_labels[i]);
        var img = $("<img>", { class: "img-thumbnail bg-light rounded-circle", src: "img/" + img_src[i], onClick: "setSoil(" + soil_type_values[i] + ");$.fancybox.close();"});
        
        var col = $("<div>", { class: "col" });
        col.append(h);
        col.append(img);
        row.append(col);
    }
    $("#autoconfig div").empty().append(row);
    $("#autoconfig").removeClass("d-none");
    $(".autoconfig").trigger('click');
};

function setSoil(value)
{
    var instance = $("#slider-soil").data("ionRangeSlider");
    instance.update({
       from: value
    });
};

function stopConsole()
{
    clearTimeout(refreshTimer);

    var btn = $("#debugConsole");
    btn.text("Start Console");
    btn.attr("onclick","startConsole(0xEE,0)");
    btn.removeClass("btn-danger");
    btn.addClass("btn-primary");

    sendStop();
};

function sendStop()
{
    $.ajax("usb.php?eeprom=write&offset=" + ee + "," + e_deepSleep + "&value=255," + deepSleep);
};

function startConsole(hex,delay)
{
    connectPlant(false);

    if(chip.length > 0) {

        if(hex == 0xEE) {
            p = 0;
            progressTimer = setInterval(progressCounter, 12);
        }

        $.ajax("usb.php?eeprom=write&offset=" + ee + "," + e_deepSleep + "," + e_VSolar + "," + e_moisture + "," + e_water + "," + e_errorCode + "," + e_empty + "," + e_log + "&value=" + parseInt(hex) + "," + parseInt(delay) + ",255,255,255,255,0,0", {
            success: function(data) {
                consoleHex(data);
                if(data.length >= 64) {

                    var btn = $("#debugConsole");

                    var s = data.split("\n");
                    if(s[ee] == parseInt(0xEE))
                    {
                        //console.log(data);
                        $.notify({ message: "EEPROM Debug Enabled" }, { type: "success" });

                        $("#debugOutput").empty();
                        btn.text("Stop Console");
                        btn.attr("onclick","startConsole(0xFF," + deepSleep  + ")");
                        btn.removeClass("btn-primary");
                        btn.addClass("btn-danger");

                        getEEPROM();
                      
                    }else if(s[ee] == parseInt(0xFF)) {
                        $.notify({ message: "EEPROM Debug Disabled" }, { type: "warning" });
                        
                        stopConsole();
                        
                        $.ajax("usb.php?reset=1");
                    }
                    
                }else{
                    $.notify({ message: "Cannot read EEPROM" }, { type: "danger" });
                }
            }
        });
    }else{
        $.notify({ message: connectMessage }, { type: "danger" });
    }
};

function EEPROM_T85(offset)
{
	/*
	Strange bug, ATtiny85 eeprom is large (uint16_t) starts write @ address 0x100 (256)

	It is flash-costly to handle uint16_t in attiny ...so offset it here

	Wastefull, but backwards compatible with ATtiny13
	*/

    if(e_versionID == 0) {
    	ee += offset;
    	e_versionID += offset;
    	e_sensorMoisture += offset;
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

function checkEEPROM()
{
    $.ajax("usb.php?eeprom=erase", {
        success: function(data) {
            consoleHex(data);
            if(data.length >= 64) {
               getEEPROMInfo(1);
            }else{
                $.notify({ message: "Cannot read EEPROM" }, { type: "danger" });
            }
        }
    });
};

function getEEPROMInfo(crc)
{
    $.ajax("usb.php?eeprom=read", {
        success: function(data) {
            consoleHex(data);
            if(data.length >= 64) {

                var s = data.split("\n");
                var info = $("#debugInfo").empty();
                info.append("Hardware Chip: " + chip + "\n");

                var vr = HexShift(s,e_versionID).toString();
                var vreg = HexShift(s,e_VReg);

                var vchip = "TP4056";
                if(vreg == 1) {
                    vchip = "WS78L05";
                }else if(vreg== 2) {
                    vchip = "LM2731";
                }else if(vreg == 3) {
                    vchip = "TPL5110";
                }
                info.append("Solar Regulator: " + vchip + "\n");
                info.append("Firmware Version: " + vr.charAt(0) + "." + vr.charAt(1));

                var sl = HexShift(s,e_runSolar);
                var pt = HexShift(s,e_potSize);
                var so = HexShift(s,e_sensorMoisture);

                console.log("Solar: " + sl);
                console.log("Pot: " + pt);
                console.log("Soil: " + so);

                if(sl == 0 && pt == 0 && so == 0) {
                    if(crc == undefined) {
                        $.notify({ message: "EEPROM is Corrupt ...Trying to Fix" }, { type: "warning" });
                        checkEEPROM();
                    }else{
                        $.notify({ message: "Cannot fix EEPROM" }, { type: "danger" });
                        $.notify({ message: "Flashing Firmware ..." }, { type: "warning" });

                        $.ajax("usb.php?eeprom=flash" , {
				            success: function(data) {
				                console.log(data);
                                if(data.indexOf("flash verified") !=-1) {
                                    $.notify({ message: "Firmware Fixed!" }, { type: "success" });
                                }else{
                                    $.notify({ message: "Cannot fix Firmware, try manually (.hex File)" }, { type: "danger" });
                                }
				            }
				        });
                    }
                }else if (crc == 1) {
                    $.notify({ message: "EEPROM Fixed!" }, { type: "success" });
                }else{
                    if(s[ee] == parseInt(0xEE)) {
                        $.notify({ message: "Plant was left in Debug mode" }, { type: "danger" });
                        sendStop();
                        $.notify({ message: "Next time 'Stop Console' before disconnecting" }, { type: "warning" });
                    }
                }

                var instance = $("#slider-solar").data("ionRangeSlider");
                instance.update({
                   from: solar_values.indexOf(sl)
                });

                var instance = $("#slider-pot").data("ionRangeSlider");
                instance.update({
                    from: pot_values.indexOf(pt)
                });

                var instance = $("#slider-soil").data("ionRangeSlider");
                instance.update({
                   from: so
                });

            }else{
                $.notify({ message: "Cannot read EEPROM" }, { type: "danger" });
            }
        }
    });
};

function getEEPROM()
{
    clearTimeout(refreshTimer);

    $.ajax("usb.php?eeprom=read", {
        async: true,
        timeout: refreshSpeed + 500,
        success: function(data) {
            //consoleHex(data);
            if(data.indexOf("could not find USB") !=-1) {
                $.notify({ message: "USB Disconnected" }, { type: "danger" });
                stopConsole();
            }if(data.length >= 64) {
                var s = data.split("\n");
                var debug = $("#debugOutput").empty();

                var sl = HexShift(s,e_VSolar);
                sl = (5 * sl / 888);

                var so = HexShift(s,e_moisture);
                if(so > moistureOffset) { //Calibration Offset
                    so -= moistureOffset;
                }
                so = (4.8 * so / 800);

                var water = HexShift(s,e_water);
                var empty = HexShift(s,e_empty);
                var err = HexShift(s,e_errorCode);

                //console.log(HexShift(s,26));
                console.log("Solar:" + HexShift(s,e_VSolar));
                console.log("Sensor:" + HexShift(s,e_moisture) + "(" + HexShift(s,e_sensorMoisture) + ")");
                console.log("Water:" + water);
                console.log("Empty:" + empty);
                console.log("Log:" + HexShift(s,e_log));
                console.log("Error:" + err);

                if(sl > 0) {
                    debug.append("Solar Voltage: " + sl.toFixed(2) + "V (" + (sl/5*100).toFixed(0) + "%)\n");
                }

                if (so > 1.51 && so < 1.56) { //AVR anomaly with ADC
                    so = 0;
                }
                debug.append("Soil Moisture: " + so.toFixed(2) + "V (" + (so/4.8*100).toFixed(0) + "%)\n");

                if (so < 0.2) {
                    debug.append("Error Code: Sensor not in Soil\n");
                }

                if(err == 0 && errorCode == 8) {
                    //Debug only
                    //$.ajax("usb.php?eeprom=write&offset=" + e_log + "&value=0");

                    debug.append("Error Code: Water Refilled!\n");
                }

                if(empty == 3) {
                    debug.append("Error Code: Overwater Protection\n");
                }

                if(empty == 11) {
                    debug.append("Error Code: No Water (Sensorless)\n");
                }
                if(empty == 12) {
                    debug.append("Error Code: No Water (Sensored)\n");
                }

                if(water > 10 && water < 255) {
                    debug.append("Error Code: Overwater Protection\n");
                }

                errorCode = err;

            }else{
                $.notify({ message: "Cannot read EEPROM" }, { type: "danger" });
            }
        }
    });

    refreshTimer = setTimeout(function () {
        getEEPROM();
    }, refreshSpeed);
};

function consoleHex(data)
{
    var _data = "";

    var s = data.split("\n");
    for (i = 0; i < s.length-1; i++) {
        _data += "[" +  parseInt(i).toString(16) + "] " + s[i] + "\n";
    }

    console.log(_data);
};

function HexShift(hex,bit)
{
    if(hex[bit] == 255) {
        return 0;
    }else if(hex[bit+1] == 255) {
        return parseInt(hex[bit]);
    }else{
        return hex[bit] | hex[bit+1] << 8;
    }
    return 0;
};

function connectPlant(async)
{
    if(chip == "") {
        $.ajax("usb.php?connect=plant", {
            async: async,
            success: function(data) {
                if(data == "ATtiny13" || data == "ATtiny45" || data == "ATtiny85") {
                    if (data == "ATtiny45") {
                        EEPROM_T85(128);
                	}else if (data == "ATtiny85") {
                		EEPROM_T85(256);
                		refreshSpeed = 10000; //EEPROM takes longer to read, do not force early interrupt
                	}
                    chip = data;
                    $.notify({ message: "Plant Connected" }, { type: "success" });
                    $(".icon-chip").attr("data-original-title", "<h6 class='text-white'>" + data + "</h6>");
                    getEEPROMInfo();
                }else if(data == "fix") {
                    $.notify({ message: "... Fixing USB Driver" }, { type: "danger" });
                    $.ajax("usb.php?driver=fix", {
                        success: function(data) {
                            if(data == "ok") {
                                $.notify({ message: "USB Driver Installed" }, { type: "success" });
                            }
                        }
                    });
                }else if(data == "sck") {
                    $.notify({ message: "... Waiting for Plant to Connect" }, { type: "warning" });
                }else{
                    setTimeout(function () {
                        connectPlant();
                    }, 4000);
                }
            }
        });
    }
};

function flashFirmware() {

    connectPlant(false);
    stopConsole();

    if(chip.length > 0) {
        $(".fileUpload").trigger("click");
        $(".fileUpload").change(function() {
            p = 0;
            progressTimer = setInterval(progressCounter, 60);
            $("#formFirmware").submit();
        });
    }else{
        $.notify({ message: connectMessage }, { type: "danger" });
    }
};

function saveSettings()
{
    connectPlant(false);

    if(chip.length > 0) {
        //$.notify({ message: "... Flashing new Firmware" }, { type: "warning" });
        $.notify({ message: "... Saving Settings" }, { type: "warning" });

        $.ajax("usb.php?eeprom=write&offset=" + e_runSolar + "," + e_potSize + "," + e_sensorMoisture + "&value=" + solar_values[$("#slider-solar").data().from] + "," + pot_values[$("#slider-pot").data().from] + "," + $("#slider-soil").data().from , {
            success: function(data) {
                consoleHex(data);

                clearTimeout(saveReminder);
                $.notify({ message: "Happy Plant &#127807;" }, { type: "success" });
                stopConsole();
            }
        });
    }else{
        $.notify({ message: connectMessage }, { type: "danger" });
    }
};

function detectTheme()
{
    var t = getCookie("theme");
    if(t == undefined) {
        if (window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches) {
            return ".slate";
        }else{
            return ""
        }
    }
    return t;
};

function switchTheme(element,dark,light) {
     if(theme == "") {
        var e = $(element + "." + dark);
        e.removeClass(dark);
        e.addClass(light);
    }else{
        var e = $(element + "." + light);
        e.removeClass(light);
        e.addClass(dark);
    }
};

function setTheme() {
    if(theme == ".slate") {
        theme = "";
    }else{
        theme = ".slate";
    }
    setCookie("theme", theme, 1);
    loadTheme();
};

function loadTheme() {
    if(theme == ".slate") {
        $('link[title="main"]').attr('href', "css/bootstrap.slate.css");
        $(".icon-day-and-night").attr("data-original-title", "<h6 class='text-white'>Light Theme</h6>");
    }else{
        $('link[title="main"]').attr('href', "css/bootstrap.css");
        $(".icon-day-and-night").attr("data-original-title", "<h6 class='text-white'>Dark Theme</h6>");
    }
    switchTheme("i.icons","text-white","text-dark");
    switchTheme("div","bg-primary","bg-light");
    switchTheme("div","text-white","text-dark");
    switchTheme("img","bg-secondary","bg-light");
};

function checkFirmwareUpdates()
{
   var check = Math.random() >= 0.5;
    if (check === true)
    {
        $.ajax("https://raw.githubusercontent.com/dimecho/ATtiny13-Plant/master/gui/Web/firmware/version.txt", {
            async: true,
            success: function success(data) {
                try {
                    var split = data.split(".");
                    var version = parseFloat(split[0]);
                    var build = parseFloat(split[1]);

                    $.ajax("firmware/version.txt", {
                        async: true,
                        success: function success(data) {

                            var _split = data.split(".");
                            var _version = parseFloat(_split[0]);
                            var _build = parseFloat(_split[1]);

                            if(version > _version || build > _build)
                            {
                                var url = "https://github.com/dimecho/ATtiny13-Plant/releases/download/";
                                if(os === "mac"){
                                    url += version + "ATtiny13.Plant.dmg";
                                }else if(os === "windows"){
                                    url += version + "ATtiny13.Plant.Windows.zip";
                                }else if(os === "linux"){
                                    url += version + "ATtiny13.Plant.Linux.tgz";
                                }
                                $.notify({
                                    icon: "icon icon-download",
                                    title: "New Firmware",
                                    message: "Available <a href='" + url + "' target='_blank'>Download</a>"
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
    $(".progress-bar").css("width", p + "%");
    if(p == 100) {
        clearInterval(progressTimer);
    }
};

function saveReminderCounter() {
    $.notify({ message: "Don't forget to Save Settings!" }, { type: "warning" });
};

function deleteCookie(name, path, domain) {

  if(getCookie(name)) {
    document.cookie = name + "=" +
      ((path) ? ";path="+path:"")+
      ((domain)?";domain="+domain:"") +
      ";expires=Thu, 01 Jan 1970 00:00:01 GMT";
  }
};

function setCookie(name, value, exdays) {

    var exdate = new Date();
    exdate.setDate(exdate.getDate() + exdays);
    var c_value = escape(value) + (exdays == null ? "" : "; expires=" + exdate.toUTCString());
    document.cookie = name + "=" + c_value;
};

function getCookie(name) {
    
    var i,
        x,
        y,
        ARRcookies = document.cookie.split(";");

    for (var i = 0; i < ARRcookies.length; i++) {
        x = ARRcookies[i].substr(0, ARRcookies[i].indexOf("="));
        y = ARRcookies[i].substr(ARRcookies[i].indexOf("=") + 1);
        x = x.replace(/^\s+|\s+$/g, "");
        if (x == name) {
            return unescape(y);
        }
    }
};