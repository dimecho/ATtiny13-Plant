var logTimer;
var logTimeout = 180;

$(document).ready(function ()
{
	logTimer = setInterval(logRefresh, 4000);
});

function runAsAdmin()
{
    $.ajax("usb.php?runas=admin", {
        success: function(data) {
            console.log(data);
        }
    });
}

function logRefresh()
{
	if(logTimeout == 0) {
		clearTimeout(logTimer);
		$.notify({ message: "Installation Timed Out ..." }, { type: "danger" });
		setTimeout(function () {
            window.location.href = "index.html";
        }, 2000);
	}
 	$.ajax("usb.php?log=read", {
        success: function(data) {
            //console.log(data);
            if(data.length > 0) {
                clearTimeout(logTimer);

                var center = $("<center>");
                var div = $("<div>",{class:"container"});
			    var row = $("<div>",{class:"row"});
			    var col = $("<div>",{class:"col"});
			    var pre = $("<pre>").append(data);

			    col.append(pre);
    			div.append(row.append(col));

    			$("#log").empty();
    			$("#log").append(div);

                if(data.indexOf("Access is denied") != -1) {
                    $.notify({ message: "Administrator Access is Required!" }, { type: "warning" });
                    var btn = $("<button>",{ class:"btn btn-primary", type:"button", onclick:"runAsAdmin()"});
                    var i = $("<i>", {class:"icons icon-magic"});
                    $("#log").append(center.append(btn.append(i).append(" Run as Administrator")));
                }else if(data.indexOf("canceled by the user") != -1) {
                    $.notify({ message: "Contact your System Administrator" }, { type: "warning" });
                    var btn = $("<button>",{ class:"btn btn-primary", type:"button", onclick:"location.reload()"});
                    var i = $("<i>", {class:"icons icon-refresh"});
                    $("#log").append(center.append(btn.append(i).append(" Try Agagin")));
                }else if(data.indexOf("StandardOut has not been redirected") != -1) {
                    setTimeout(function () {
                        window.location.href = "index.html";
                    }, 2000);
                }
            }
        }
    });
    logTimeout--;
};