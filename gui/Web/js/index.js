
var theme = detectTheme();
var refreshTimer;
var refreshSpeed = 4000;
var progressTimer;

var solar_values = [0,1];
var solar_labels = ["OFF", "ON"];

var pot_values = [12,32,64];
var pot_labels = ["Small", "Medium", "Large"];

var soil_values = [280, 388, 460];
var soil_labels = ["Dry (Cactus)", "Moist (Regular)", "Wet (Tropical)"];

var p = 1;
var os = "";
var chip = "";

var connectMessage = "Connect Plant to Computer";

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
    });

    $("#slider-pot").ionRangeSlider({
        skin: "big",
        from: 0,
        values: pot_labels
    });
    
    $("#slider-soil").ionRangeSlider({
        skin: "big",
        from: 0,
        values: soil_labels
    });

    checkFirmwareUpdates();

    connectPlant(true);

    $('[data-toggle="tooltip"]').tooltip();
});

function stopConsole()
{
    clearTimeout(refreshTimer);

    var btn = $("#debugConsole");
    btn.text("Start Console");
    btn.attr("onclick","startConsole(0xEE)");
    btn.removeClass("btn-danger");
    btn.addClass("btn-primary");

    //$.ajax("usbasp.php?eeprom=write&offset=1,8&value=255,40");
};

function startConsole(hex)
{
    connectPlant(false);

    if(chip.length > 0) {

        if(hex == 0xEE) {
            p = 0;
            progressTimer = setInterval(progressCounter, 12);
        }

        //Clear old values
        $.ajax("usbasp.php?eeprom=write&offset=10,26,42&value=255,255,255", { async:false });

        $.ajax("usbasp.php?eeprom=write&offset=1,8&value=" + parseInt(hex) + ",0", {
            success: function(data) {
                console.log(data);
                if(data.length >= 64) {

                    var btn = $("#debugConsole");

                    var s = data.split("\n");
                    if(s[1] == parseInt(0xEE))
                    {
                        //console.log(data);
                        $.notify({ message: "EEPROM Debug Enabled" }, { type: "success" });

                        $("#debugOutput").empty();
                        btn.text("Stop Console");
                        btn.attr("onclick","startConsole(0xFF)");
                        btn.removeClass("btn-primary");
                        btn.addClass("btn-danger");

                        getEEPROM();
                      
                    }else if(s[1] == parseInt(0xFF)) {
                        $.notify({ message: "EEPROM Debug Disabled" }, { type: "warning" });
                        
                        stopConsole();
                        
                        $.ajax("usbasp.php?reset=1");
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

function checkEEPROM()
{
    $.ajax("usbasp.php?eeprom=erase", {
        success: function(data) {
            console.log(data);
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
    $.ajax("usbasp.php?eeprom=read", {
        success: function(data) {
            console.log(data);
            if(data.length >= 64) {

                var s = data.split("\n");
                var info = $("#debugInfo").empty();
                info.append("Hardware Chip: " + chip + "\n");
                info.append("Firmware Version: " + s[0].charAt(0) + "." + s[0].charAt(1));

                var sl = HexShift(s,6);
                var pt = HexShift(s,4);
                var so = HexShift(s,2);

                console.log("Solar: " + sl);
                console.log("Pot: " + pt);
                console.log("Soil: " + so);

                if(sl == 0 && pt == 0 && so == 0) {
                    if(crc == undefined){
                        $.notify({ message: "EEPROM is Corrupt ...Trying to Fix" }, { type: "danger" });
                        checkEEPROM();
                    }else{
                        $.notify({ message: "Cannot Fix EEPROM, re-Flash Firmware" }, { type: "danger" });
                    }
                }else if (crc == 1) {
                    $.notify({ message: "EEPROM Fixed!" }, { type: "success" });
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
                   from: soil_values.indexOf(so)
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

    $.ajax("usbasp.php?eeprom=read", {
        async: true,
        timeout: refreshSpeed + 500,
        success: function(data) {
            //console.log(data);
            if(data.indexOf("could not find USB") !=-1) {
                $.notify({ message: "USB Disconnected" }, { type: "danger" });
                stopConsole();
            }if(data.length >= 64) {
                var s = data.split("\n");
                var debug = $("#debugOutput").empty();

                var sl = (5 * HexShift(s,10) / 1024); //0xA
                var so = (4.8 * HexShift(s,26) / 1024); //0x1A
                var er = HexShift(s,42); //0x2A

                console.log(HexShift(s,26));

                if(sl > 0) {
                    debug.append("Solar Voltage: " + sl.toFixed(2) + "V\n");
                }

                debug.append("Soil Moisture: " + so.toFixed(2) + "V (" + (so/4.8*100).toFixed(0)+ "%)\n");

                if(er == 69) {
                    debug.append("Error Code: Empty Bottle\n");
                }else if (so < 4) {
                    debug.append("Error Code: Sensor not in Soil\n");
                }
            }else{
                $.notify({ message: "Cannot read EEPROM" }, { type: "danger" });
            }
        }
    });

    refreshTimer = setTimeout(function () {
        getEEPROM();
    }, refreshSpeed);
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
        $.ajax("usbasp.php?connect=plant", {
            async: async,
            success: function(data) {
                if(data == "ATtiny13" || data == "ATtiny45" || data == "ATtiny85") {
                    chip = data;
                    $.notify({ message: "Plant Connected" }, { type: "success" });
                    $(".icon-chip").attr("data-original-title", "<h6 class='text-white'>" + data + "</h6>");
                    getEEPROMInfo();
                }else if(data == "fix") {
                    $.notify({ message: "... Fixing USB Driver" }, { type: "danger" });
                    $.ajax("usbasp.php?driver=fix", {
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

        $.ajax("usbasp.php?eeprom=write&offset=6,4,2&value=" + solar_values[$("#slider-solar").data().from] + "," + pot_values[$("#slider-pot").data().from] + "," + soil_values[$("#slider-soil").data().from] , {
            success: function(data) {
                console.log(data);

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