var theme = detectTheme();
var os = "";
var chip = "";

$(document).ready(function ()
{
    loadTheme();

    $("#slider-solar").ionRangeSlider({
        skin: "big",
        from: 0,
        values: [
            "OFF", "ON"
        ],
        onFinish: function (data) {

            if(data.from_value == "ON")
            {
                $.notify({ message: "Place solar cell as close to direct sunlight as possible" }, { type: "success" });
            }
        },
    });

    $("#slider-pot").ionRangeSlider({
        skin: "big",
        from: 0,
        values: [
            "Small", "Medium", "Large"
        ]
    });

    $("#slider-soil").ionRangeSlider({
        skin: "big",
        from: 0,
        values: [
            "Dry (Cactus)", "Moist (Regular)", "Wet (Tropical)"
        ]
    });
    
    $.ajax("serial.php?os=1", {
        success: function(data) {
            os = data;
        }
    });

    checkFirmwareUpdates();

    $.ajax("serial.php?com=check", {
        success: function(data) {
            if(data.length > 0)
            {
                $.notify({ message: "Plant Connected!" }, { type: "success" });
            }
        }
    });

    $('[data-toggle="tooltip"]').tooltip()
});

function saveSettings()
{
    $.ajax("serial.php?com=check", {
        success: function(data) {
            //console.log(data);
            if(data.length > 0)
            {
                var s = data.split('\n');
                //for (var i = 0; i < s.length; i++) {
                //    if(s[i] != "")
                //        $("#serial-interface").append($("<option>",{value:s[i]}).append(s[i]));
                //}

                $.notify({ message: "Firmware is Waiting ...Please Reset Chip" }, { type: "warning" });

                $.ajax("serial.php?flash=" + s[0] + "&solar=" + $("#slider-solar").data().from + "&pot=" + $("#slider-pot").data().from + "&soil=" + $("#slider-soil").data().from, {
                    success: function(data) {
                        console.log(data);
                        $.notify({ message: "Happy Plant &#127807;" }, { type: "success" });
                    }
                });

            }else{
                $.notify({ message: "No USB Found, Connect Plant to Computer" }, { type: "danger" });
            }
        }
    });
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

    }else{
        $('link[title="main"]').attr('href', "css/bootstrap.css");
    }
    switchTheme("i.icons","text-white","text-dark");
    switchTheme("div","bg-primary","bg-light");
    switchTheme("div","text-white","text-dark");
    switchTheme("table","bg-primary","bg-light");
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