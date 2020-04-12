var theme = detectTheme();

var netAccess = getCookie('graph.netaccess') || false;
var roundEdges = (getCookie('graph.roundedges') == 'true') || true;
var showDataLabels = (getCookie('graph.datalabels') == 'true') || true;
var showAnimation = (getCookie('graph.animation') == 'true') || false;
var graphDivision = getCookie('graph.division') || 60;
var lineWidth = getCookie('graph.border') || 3;
var pageLimit = getCookie('graph.pages') || 4;

var chart_datasets = [{
	type: 'line',
	label: 'ADC Value',
	backgroundColor: 'rgba(220,53,69,0.5)',
	borderColor: 'rgba(220,53,69,1)',
	borderWidth: lineWidth,
    fill: false,
	data: [],
	yAxisID: 'y-axis-0'
}, {
	type: 'line',
	label: 'H2O %',
	backgroundColor: 'rgba(0,123,255,0.5)',
	borderColor: 'rgba(0,123,255,1)',
	borderWidth: lineWidth,
    fill: false,
	data: [],
	yAxisID: 'y-axis-1'
}];

var refreshSpeed = 10 * 60 * 1000; //10 minutes
var refreshTimer;
var data = {};
var options = {};
var chip = "";
var serial = '000';
var chart;
var ctxAxis;
var ctx;
var ctxFont = 12;
var ctxFontColor = 'black';
var ctxGridColor = '#BEBEBE';
var xhr;

$(document).ready(function ()
{
    loadTheme();

	buildGraphMenu();

	graphTheme();

    graphSettings();

    var canvas = $('#chartCanvas');
    ctx = canvas.get(0).getContext('2d');
	ctxAxis = $('#chartAxis').get(0).getContext('2d');

    if(navigator.userAgent.toLowerCase().match(/mobile/i)) {

        Chart.defaults.animationSteps = 0;
        canvas[0].height = 800;
        ctxFont = 40;
        graphDivision = 40;

    }else{
        Chart.defaults.animationSteps = 12;
        canvas[0].height = 640;
    }

    initChart();

    connectPlant(true);

    //ctx.fillStyle = 'white';
    /*
    ctx.webkitImageSmoothingEnabled = false;
    ctx.mozImageSmoothingEnabled = false;
    ctx.imageSmoothingEnabled = false;
    */
});

function graphTheme() {

	if(theme == '.slate') {
        ctxFontColor = 'white';
        ctxGridColor = '#707070';           
    }
};

function graphSettings(save) {

	if(save) {
        netAccess = $('input[name*="netAccess"]').is(':checked');
		roundEdges = $('input[name*="roundEdges"]').is(':checked');
		showDataLabels = $('input[name*="showDataLabels"]').is(':checked');
		showAnimation = $('input[name*="showAnimation"]').is(':checked');
		graphDivision = $('input[name*="graphDivision"]').val();
		lineWidth = $('input[name*="lineWidth"]').val();
		pageLimit = $('input[name*="pageLimit"]').val();
		
        setCookie('graph.netaccess', netAccess, 1);
		setCookie('graph.roundedges', roundEdges, 1);
		setCookie('graph.datalabels', showDataLabels, 1);
		setCookie('graph.animation', showAnimation, 1);
		setCookie('graph.division', graphDivision, 1);
		setCookie('graph.border', lineWidth, 1);
		setCookie('graph.pages', pageLimit, 1);

        if(netAccess == true) {
            $.ajax('usb.php?network=ip', {
                success: function(response) {
                    if(response != "") {
                        $.notify({ message: 'Application Restart Required' }, { type: 'warning' });
                        $.notify({ message: 'IP enabled, new URL http://' + response }, { type: 'success' });
                    }
                }
            });
        }else{
            $.ajax('usb.php?network=localhost');
        }
	}else{
        $('input[name*="netAccess"]').prop('checked', netAccess);
		$('input[name*="roundEdges"]').prop('checked', roundEdges);
		$('input[name*="showDataLabels"]').prop('checked', showDataLabels);
		$('input[name*="showAnimation"]').prop('checked', showAnimation);
		$('input[name*="graphDivision"]').val(graphDivision);
		$('input[name*="lineWidth"]').val(lineWidth);
		$('input[name*="pageLimit"]').prop('checked', pageLimit);
	}

	if(showDataLabels == true) {
		showDataLabels = 'auto';
	}
};

function buildGraphMenu() {

    var menu = $('#buildGraphMenu'); //.empty();
    var menu_buttons = $('#buildGraphButtons'); //.empty();
    var export_buttons = $('#buildGraphExport'); //.empty();

	var btn_start = $('<button>', { class: 'btn btn-success mr-4' }).append('Start Graph');
    var btn_stop = $('<button>', { class: 'btn btn-danger mr-4' }).append('Stop Graph');
    
    var e_settings = $('<i>', { class: 'icons icon-settings h1 ml-4', onClick: '$("#graphSettings").modal()', 'data-toggle': 'tooltip', 'title': 'Settings' });
    var e_img = $('<i>', { class: 'icons icon-png h1 ml-4', onClick: 'exportPNG()', 'data-toggle': 'tooltip', 'title': 'Export Image' });
    var e_csv = $('<i>', { class: 'icons icon-csv h1 ml-4', onClick: 'exportCSV()', 'data-toggle': 'tooltip', 'title': 'Export CSV' });

    var s = $('#buildGraphSlider').empty();
    var input_speed = $('<input>', { id: 'speed', type: 'text', 'data-provide': 'slider'} );
    s.append(input_speed);

    btn_start.click(function() {
        btn_start.prop('disabled', true);
        startChart();
    });

    btn_stop.click(function() {
        stopChart();
        btn_start.prop('disabled', false);
    });

    function speed_prettify (n) {
        if (n == 120) {
            return 'Slow';
        }else if (n == 1) {
            return 'Fast';
        }
        return n;
    };
    
    input_speed.ionRangeSlider({
        skin: 'big',
        grid: true,
        step: 1,
        min: 1,
        max: 120,
        from: (refreshSpeed / 60 / 1000),
        prettify: speed_prettify,
        postfix: ' Minutes',
        onFinish: function (e) {

            if(btn_start[0].disabled == true) {
                $.ajax('usb.php?eeprom=write&offset=' + ee + ',' + e_deepSleep + '&value=' + parseInt(0xEF) + ',' + calcDeepSleep(e.from), { async: true});
                if (xhr) xhr.abort();
                clearTimeout(refreshTimer);
                updateChart();
            }

            if(e.from == 1) { //Developers only
                $('#refreshConfirm').modal();
                $('#refreshconfirm-cancel').click(function() {
                    $('#speed').data('ionRangeSlider').update({from:refreshSpeed/60/1000});
                });
                $('#refreshconfirm-ok').click(function() {
                    refreshSpeed = 0.4 * 60 * 1000;
                });
                return;
            }else if(e.from < 12) {
                $.notify({ message: 'EEPROM has about 100,000 write cycles.' }, { type: 'warning' });
                setTimeout(function () {
                    $.notify({ message: 'Collecting data too fast will ware out the chip.' }, { type: 'danger' });
                }, 4000);
            }

            refreshSpeed = e.from * 60 * 1000; //1000 = second
            //console.log(refreshSpeed);
        }
    });
	
    menu_buttons.append(btn_start);
    menu_buttons.append(btn_stop);

    export_buttons.append(e_settings);
    export_buttons.append(e_img);
    export_buttons.append(e_csv);

    pageLimit = graphDivision * pageLimit;

    $('[data-toggle="tooltip"]').tooltip({
       container: 'body'
    });
};

function exportCSV() {

    var datasets = chart_datasets;
    var points = idDatasets(datasets);
    var value = csvDatasets(datasets);
    //console.log(value);

    let csvContent = 'data:text/csv;charset=utf-8,';

    let row = value[0].length;
    let col = points.length;

    for (var r = 0; r < row; r++) {
        if(r == 0) { //first row
            csvContent += points.join(',') + '\r\n';
        }
        for (var c = 0; c < col; c++) {
            //TODO: get timestamp
            csvContent += value[c][r] + ',';
        }
        csvContent += '\r\n';
    }

    //var encodedUri = encodeURI(csvContent);
    //window.open(encodedUri);

    var encodedUri = encodeURI(csvContent);
    var link = document.createElement('a');
    link.setAttribute('href', encodedUri);
    link.setAttribute('download', 'export.csv');
    document.body.appendChild(link); // Required for FF
    link.click();
};

function exportPNG() {

    //ctx.save();
    //ctx.scale(4,4);
    //var render = ctx.canvas.toDataURL('image/jpeg',1.0);
    //ctx.restore();

    var render = ctx.canvas.toDataURL('image/png', 1.0);
    var d = new Date();
    //d.setHours(10, 30, 53, 400);

    var data = atob(render.substring('data:image/png;base64,'.length)),
        asArray = new Uint8Array(data.length);

    for (var i = 0, len = data.length; i < len; ++i) {
        asArray[i] = data.charCodeAt(i);
    }
    var blob = new Blob([asArray.buffer], { type: 'image/png' });

    var url = URL.createObjectURL(blob);
    var a = document.createElement('a');
    a.href = url;
    a.download = 'graph ' + d.getDate() + '-' + (d.getMonth() + 1) + '-' + d.getFullYear() + ' ' + (d.getHours() % 12 || 12) + '-' + d.getMinutes() + ' ' + (d.getHours() >= 12 ? 'pm' : 'am') + '.png';
    document.body.appendChild(a);
    a.click();
    setTimeout(function () {
        document.body.removeChild(a);
        window.URL.revokeObjectURL(url);
    }, 0);
};

function initChart() {

    data = {};
    options = {};

    initADCChart();

    if(chart) {
        chart.clear();
        chart.destroy();
    }
    
    if(roundEdges == false) {
		Chart.defaults.elements.line.tension = 0;
	}
	if(showAnimation == true) {
		Chart.defaults.animation.duration = 800;
	}

    chart = new Chart(ctx, {
        type: 'line',
        data: data,
        options: options
    });
    //chart.update();
    
    $('.chartAreaWrapper2').width($('.chartAreaWrapper').width());
};

function idDatasets(dataset) {
	ids = [];
	for (var i = 0, l = dataset.length; i < l; i++) {
		if(dataset[i].label)
			ids.push(dataset[i].label);
	}
	console.log(ids);
	return ids;
};

function csvDatasets(dataset) {
    row = [];
    for (var i = 0, l = dataset.length; i < l; i++) {
        row.push(dataset[i].data);
    }
    console.log(row);
    return row;
};

function stopChart() {

    if (xhr) xhr.abort();
    clearTimeout(refreshTimer);

    $.ajax('usb.php?eeprom=write&offset=' + ee + ',' + e_deepSleep + '&value=' + parseInt(0xFF) + ',' + deepSleep, { async: true});
};

function calcDeepSleep(e) {

    var sleep = Math.round((e * 8) - 4 - e);
    if(sleep < 0)
        sleep = 0;

    return sleep;
};

function startChart() {

    console.log(refreshSpeed);

    $.ajax('usb.php?eeprom=write&offset=' + ee + ',' + e_deepSleep + '&value=' + parseInt(0xEF) + ',' + calcDeepSleep(refreshSpeed/60/1000), {
        async: false,
        success: function(response)
        {
        	//consoleHex(response);
            var s = response.split('\n');
        	serial = HexShift(s,e_serial);
            console.log('Serial #: ' + serial);

            var d = getCookie(serial);
            if(d !== undefined) {
            	$('#confirmBox').find('.modal-title').text('Graph Data Exists (Plant ID: ' + serial + ')');
            	$('#confirmBox').modal();
                return;
            }
        	updateChart();
        }
    });
};

function initTimeAxis(seconds, labels, stamp) {

    var xaxis = [];

    if(labels)
        xaxis = labels;

    for (var i = 0; i < seconds; i++) {
    	if (stamp != undefined) {
    		if (stamp == 0) {
	    		xaxis.push(i);
	    	}else{
	    		if (i % 10 == 0) {
		    		if (stamp == 1) {
		    			xaxis.push(i);
		    		}else{
		    			xaxis.push(i + ' ' + stamp);
		    		}
	    		}
    		}
    	}else{
    		xaxis.push('');
    	}
        /*
        if (i / 10 % 1 != 0) {
            xaxis.push('');
        } else {
            xaxis.push(i);
        }
        */
        //xaxis.push(i.toString());
    }
    return xaxis;
};

function initADCChart() {

    data = {
        labels: initTimeAxis(graphDivision),
        datasets: chart_datasets
    };

    options = {
        legend: {
            labels: {
            	fontColor: ctxFontColor,
                fontSize: ctxFont
            }
        },
        elements: {
            point: {
                radius: 0
            }
        },
        tooltips: {
            enabled: false
        },
        responsive: true,
        hoverMode: 'index',
        stacked: false,
        maintainAspectRatio: false,
        scales: {
            'x-axis-0': {
                //type: 'linear',
                display: true,
                position: 'bottom',
                scaleLabel: {
                    display: false,
                	fontColor: ctxFontColor,
                    fontSize: ctxFont,
                    labelString: 'Time (hh:mm:ss)'
                },
                ticks: {
                	fontColor: ctxFontColor,
                    fontSize: ctxFont,
                    maxRotation: 90,
                    reverse: false
                },
                gridLines: {
                    drawOnChartArea: true,
				    color: ctxGridColor
				}
            },
            'y-axis-0': {
                type: 'linear',
                display: true,
                position: 'left',
                suggestedMin: 0, //auto scale
                suggestedMax: 100, //auto scale
                scaleLabel: {
                    display: true,
                    fontColor: ctxFontColor,
                    fontSize: ctxFont,
                    labelString: data.datasets[0].label
                },
                ticks: {
                    fontColor: ctxFontColor,
                    fontSize: ctxFont,
                    //stepSize: 10
                },
                gridLines: {
                    drawOnChartArea: true,
                    color: ctxGridColor,
                    zeroLineColor: ctxFontColor,
                    //zeroLineWidth: 2
                }
            },
            'y-axis-1': {
                type: 'linear',
                display: true,
                position: 'right',
                suggestedMin: 0, //auto scale
                suggestedMax: 60, //auto scale
                scaleLabel: {
                    display: true,
                    fontColor: ctxFontColor,
                    fontSize: ctxFont,
                    labelString: data.datasets[1].label
                },
                ticks: {
                    fontColor: ctxFontColor,
                    fontSize: ctxFont,
                    stepSize: 10
                },
                gridLines: {
                    drawOnChartArea: false,
                    color: ctxGridColor,
                    zeroLineColor: ctxFontColor,
                    //zeroLineWidth: 2
                }
            }
        }
    };
};

function updateChart() {

    clearTimeout(refreshTimer);

	xhr = $.ajax('usb.php?eeprom=read', {
        async: true,
        success: function(response)
        {
            //consoleHex(response);
            //console.log(data);

            var s = response.split('\n');
            var adc = HexShift(s,e_moisture);
            var ref = adc / 1023;
            var v = 5 * (ref * 1.1); // Calculate Vcc from 5V
            var h2o = Math.round(ref * 100 / 1.8); //scientific value (actual H2O inside soil)
            console.log(adc + ' ' + h2o);

            data.datasets[0].data.push(adc);
            data.datasets[1].data.push(h2o);

            setCookie(serial, JSON.stringify(data.datasets), 30);
            //console.log(data.datasets[0].data);
            
            var l = data.datasets[0].data.length;
            //Scroll
            if (l == data.labels.length) {
                initTimeAxis(graphDivision, data.labels);
                var newwidth = $('.chartAreaWrapper').width() + chart.width;
                $('.chartAreaWrapper2').width(newwidth);
                $('.chartAreaWrapper').animate({scrollLeft:newwidth}, 1000);
                
                var copyWidth = chart.scales['y-axis-0'].width - 10;
                var copyHeight = chart.scales['y-axis-0'].height + chart.scales['y-axis-0'].top + 10;
                ctxAxis.canvas.height = copyHeight;
                ctxAxis.canvas.width = copyWidth;
                ctxAxis.drawImage(chart.chart.canvas, 0, 0, copyWidth, copyHeight, 0, 0, copyWidth, copyHeight);

            }else if (l > pageLimit) {
                //console.log('Max scroll pages ...reset');
                for (var x = 0; x <= last; x++) {
                    data.datasets[x].data = []; //empty
                }
                data.labels = initTimeAxis(graphDivision);
                $('.chartAreaWrapper2').width($('.chartAreaWrapper').width());
                //$('.chartAreaWrapper2').height($('.chartAreaWrapper').height());
            }
            
            //Time-stamp
            if (l / 10 % 1 == 0) {
                var d = new Date();
                var hr = d.getHours();
                //console.log(l);
                data.labels[l] = hr + ':' + d.getMinutes() + ':' + d.getSeconds();
            }
            
            chart.update();

            refreshTimer = setTimeout(function() {
                updateChart();
            }, refreshSpeed);
        },
        error: function() {
            $.notify({ message: 'HTTP Error'}, { type: 'success' });
        }
    });
};

function connectPlant(async)
{
    clearTimeout(refreshTimer);

    if(chip == "") {
        $.ajax("usb.php?connect=plant", {
            async: async,
            success: function(data) {
                console.log(data);

                if(data == "ATtiny13" || data == "ATtiny45" || data == "ATtiny85") {
                    if (data == "ATtiny45") {
                        EEPROM_T85(128);
                    }else if (data == "ATtiny85") {
                        EEPROM_T85(256);
                    }
                    chip = data;
                    $.notify({ message: "Plant Connected" }, { type: "success" });
                }else{
                    setTimeout(function () {
                        connectPlant();
                    }, 4000);
                }
            }
        });
    }
};

function detectTheme()
{
    var t = getCookie('theme');
    if(t == undefined) {
        if (window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches) {
            return '.slate';
        }else{
            return '';
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
    }else{
        $('link[title="main"]').attr('href', 'css/bootstrap.css');
        $('.icon-day-and-night').attr('data-original-title', '<h6 class="text-white">Dark Theme</h6>');
    }
    switchTheme('i.icons','text-white','text-dark');
    switchTheme('div','bg-primary','bg-light');
    switchTheme('div','text-white','text-dark');
    switchTheme('img','bg-secondary','bg-light');
};

function isEmpty( el ){
    return !$.trim(el.html())
};

Array.prototype.max = function() {
  return Math.max.apply(null, this);
};

Array.prototype.min = function() {
  return Math.min.apply(null, this);
};