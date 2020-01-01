var theme = detectTheme();
var os = "";

$(document).ready(function ()
{
    loadTheme();

    $("#slider-solar").ionRangeSlider({
        skin: "big",
        from: getCookie("attiny-plant-solar"),
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
        from: getCookie("attiny-plant-pot"),
        values: [
            "Small", "Medium", "Large"
        ]
    });

    $("#slider-soil").ionRangeSlider({
        skin: "big",
        from: getCookie("attiny-plant-soil"),
        values: [
            "Dry (Cactus)", "Moist (Regular)", "Wet (Tropical)"
        ]
    });

    checkFirmwareUpdates();

    connectPlant();

    $('[data-toggle="tooltip"]').tooltip();
});

function connectPlant()
{
    $.ajax("usbasp.php?connect=plant", {
        success: function(data) {
            if(data == "ATtiny13" || data == "ATtiny45" || data == "ATtiny85") {
                $.notify({ message: "Plant Connected" }, { type: "success" });
                $(".icon-chip").attr("data-original-title", "<h6 class='text-white'>" + data + "</h6>");
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

function saveSettings()
{
    connectPlant();

    var chip = $(".icon-chip").attr("data-original-title").toLowerCase();
    console.log(chip);

    if(chip.length > 0) {
        $.notify({ message: "... Flashing new Firmware" }, { type: "warning" });
        $.ajax("usbasp.php?flash=" + chip + "&solar=" + $("#slider-solar").data().from + "&pot=" + $("#slider-pot").data().from + "&soil=" + $("#slider-soil").data().from, {
            success: function(data) {
                console.log(data);
                $.notify({ message: "Happy Plant &#127807;" }, { type: "success" });

                setCookie("attiny-plant-solar", $("#slider-solar").data().from, 30);
                setCookie("attiny-plant-pot", $("#slider-pot").data().from, 30);
                setCookie("attiny-plant-soil", $("#slider-soil").data().from, 30);
            }
        });
    }else{
        $.notify({ message: "Connect Plant to Computer" }, { type: "danger" });
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