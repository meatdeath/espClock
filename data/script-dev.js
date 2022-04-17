
var pressure_history_data;
var last_time = -1;
var back_color = 0;
var time = 1642561783;
var h_offset = 0;
var m_offset = 0;

function getCorrectionString() {
    return ""+h_offset+"h "+m_offset+"m";
}

function setOffset() {
    var h = document.getElementById('hour_offset').value;
    var m = m_offset = document.getElementById('minute_offset').value;
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) 
        {
            h_offset = document.getElementById('hour_offset').value;
            m_offset = document.getElementById('minute_offset').value;
        }
    };
    param='?hour_offset=' + h + '&minute_offset=' + m;
    xhttp.open('GET', '/set_time_offset'+param, true);
    xhttp.send();
}

function getFastTelemetry() {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() 
    {
        if (this.readyState == 4 && this.status == 200) 
        {
            var telemetry = JSON.parse(this.responseText);
            document.getElementById("pressure-text-value").innerText = telemetry.Pressure;
            document.getElementById("temperature-text-value").innerText = telemetry.Temperature;
            document.getElementById("utc-time-string").innerText = 
                ("0"+telemetry.Hours).slice(-2) + ":" + ("0"+telemetry.Minutes).slice(-2) + ":" + ("0"+telemetry.Seconds).slice(-2);
            hr_rotation = 30 * telemetry.Hours + telemetry.Minutes / 2; //converting current time
            min_rotation = 6 * telemetry.Minutes;
            sec_rotation = 6 * telemetry.Seconds;
            if(sec_rotation==0) sec_rotation+=0.01;
            analogClockUTCHour.style.transform = `rotate(${hr_rotation}deg)`;
            analogClockUTCMinute.style.transform = `rotate(${min_rotation}deg)`;
            analogClockUTCSecond.style.transform = `rotate(${sec_rotation}deg)`;

            var corr_hour = telemetry.Hours*1 + telemetry.HourOffset*1;
            var corr_minute = telemetry.Minutes*1 + telemetry.MinuteOffset*1;
            var corr_second = telemetry.Seconds;
            if( corr_minute >= 60 ) {
                corr_minute -= 60;
                corr_hour += 1;
            }
            if( corr_hour >= 24 ) {
                corr_hour -= 24;
            }

            document.getElementById("corrected-time-string").innerText =
                ("0"+corr_hour).slice(-2) + ":" + ("0"+corr_minute).slice(-2) + ":" + ("0"+telemetry.Seconds).slice(-2);

            hr_rotation = 30 * corr_hour + corr_minute / 2; //converting current time
            min_rotation = 6 * corr_minute;
            sec_rotation = 6 * telemetry.Seconds;
            if(sec_rotation==0) sec_rotation+=0.01;
            analogClockCorrectedHour.style.transform = `rotate(${hr_rotation}deg)`;
            analogClockCorrectedMinute.style.transform = `rotate(${min_rotation}deg)`;
            analogClockCorrectedSecond.style.transform = `rotate(${sec_rotation}deg)`;

            h_offset = telemetry.HourOffset;
            m_offset = telemetry.MinuteOffset;
            document.getElementById("time-offset-string").innerText = getCorrectionString();
            document.getElementById("pressure-collection-time-left-text").innerText = 
                            (telemetry.SecondsUntilPressureCollection/60).toFixed() + ":" + 
                            ("0"+(telemetry.SecondsUntilPressureCollection%60)).slice(-2);
        }
    };
    xhttp.open('GET', 'getFastTelemetry', true);
    xhttp.send();
}

function getPressureHistory() 
{
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            pressure_history_data = this.responseText;
            updatePressureGraph();
        }
    };
    xhttp.open('GET', 'getPressureHistory', true);
    xhttp.send();
}


var pressure_chart = undefined;

function updatePressureGraph() {
    if( pressure_chart != undefined ) {
        var p_history = JSON.parse(pressure_history_data);
        var labels_arr = [];
        var data_arr = [];
        var index_data_arr = [];
        var index = 0;
        var last_item_hour = 0;
        last_time = -1;
        back_color = 0;
        //document.getElementById("pressure-text").innerText = p_history.current;
        for( var i = 0; i < p_history.history.length; i++) {
            var itemDate = new Date((p_history.history[i].time + (30*60))*1000);
            var itemHour = itemDate.getHours();
            labels_arr.push(itemHour);
            if( i == 0 ) {
                if( itemHour < 8 || itemHour >= 20 ) {
                    index = 1;
                }
            } else {
                if( (itemHour == 8 && last_item_hour < 8) ||
                    (itemHour == 20 && last_item_hour < 20) ||
                    (i == (p_history.history.length-1)) ) {
                    index++;
                }
            }
            last_item_hour = itemHour;
            index_data_arr.push(index);
            data_arr.push(p_history.history[i].value);
        }
        var data = {
            labels: labels_arr,
            datasets: [
                {
                    fillColor: "rgba(220,220,220,0.5)",
                    strokeColor: "rgba(220,220,220,1)",
                    data: data_arr
                },
                {
                    fillColor: "rgba(220,220,220,0.5)",
                    strokeColor: "rgba(220,220,220,1)",
                    data: index_data_arr
                }
            ]
        };
        pressure_chart.data.datasets[0].data = data_arr;
        pressure_chart.data.labels = labels_arr;
        pressure_chart.update();
    }
}

var current_tab = 0;

function updateTabs() {
    tabs = document.getElementsByClassName("tab");
    Array.from(tabs).forEach(function(tab,i) {
        tab.classList.remove("active");
        if(i==current_tab) tab.classList.add("active");
    });
    tabcontents = document.getElementsByClassName("tab-content");
    Array.from(tabcontents).forEach(function(tabcontent,i) {
        tabcontent.setAttribute("hidden","");
        if(i==current_tab) tabcontent.removeAttribute("hidden");
    });
}

function tabClick(tab) {
    current_tab = tab;
    updateTabs();
}

window.onload = function() {
    pressure_chart = new Chart('pressureChart', {
        type: 'line',
        data: {
            labels: [],
            datasets: [{
                data: [],
                borderColor: 'red',
                fill: false,
                xAxisID: "time-axis",
                yAxisID: "pressure-axis"
            }]
        },
        plugins: [{
            beforeDraw: chart => {
                var ctx = chart.chart.ctx;
                var colorAxis = chart.scales["color-axis"];
                ctx.save();
                ctx.fillStyle = "lightgray";
                ctx.beginPath();
                var color_index = 1;
                var label_index = 0;
                var xLeft;
                var xRight;
                while( label_index < colorAxis.ticks.length )
                {
                    while(label_index < colorAxis.ticks.length)
                    {
                        if(colorAxis.getLabelForIndex(label_index) == color_index) break;
                        label_index++;
                    }
                    if( label_index < colorAxis.ticks.length ) {
                        if( color_index&1 ) {
                            xLeft = colorAxis.getPixelForTick(label_index);
                        } else {
                            xRight = colorAxis.getPixelForTick(label_index);
                            ctx.fillRect(xLeft, 0, xRight-xLeft, 1000);
                        }
                        label_index++;
                        color_index++;
                    }
                }
                ctx.stroke();
                ctx.restore();
            }
        }],
        options: {
            title:  {display: false, text: 'Pressue history', fontSize: 20},
            legend: {display: false},
            scales: {
                yAxes: [{
                    id: 'pressure-axis',
                    scaleLabel: {
                        display: true, 
                        labelString: "Pressure, mm"
                    },
                    ticks: {
                        suggestedMin: 730,
                        suggestedMax: 770
                    }
                }],
                xAxes: [{
                    id: 'time-axis',
                    scaleLabel: {
                        display: true,
                        labelString: 'Time, h'
                    },
                    //type: 'linear',
                    ticks:{
                        callback:function(label){
                            return label;
                        },
                        maxRotation: 0,
                        minRotation: 0
                    }
                },
                {
                    id: 'color-axis',
                    //type: 'linear',
                    display: false,
                    gridLines:{
                        drawOnChartArea: false
                    },
                    ticks:{
                        callback:function(label, index, ticks){
                            if( index == 0 ) 
                            {
                                back_color = 0;
                                if( label >= 20 || label < 8 ) back_color = 1;
                            } 
                            else if( index == (ticks.length-1) ) back_color++;
                            else if( last_time != -1 && (label == 8 || label == 20) ) back_color++;
                            last_time = label;
                            return back_color;
                        }
                    }
                }]
            },
            animations: {
                tension: {
                    duration: 1000,
                    easing: 'linear',
                    from: 1,
                    to: 0,
                    loop: true
                }
            }
        }
    });

    setInterval( getFastTelemetry, 1000 );
    getPressureHistory();
    getFastTelemetry();
    setInterval( getPressureHistory, 5000 );
    updateTabs();
}

function getLog() {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) 
        {
            var logarea = document.getElementById("logarea");
            if(this.responseText != "") {
                logarea.value += this.responseText+"\n";
                setTimeout(getLog, 10);
            } else {
                logarea.value += "---------";
            }
            logarea.scrollTop = logarea.scrollHeight;
        }
    };
    xhttp.open('GET', '/getLogString', true);
    xhttp.send();
}

function onGetLogClick() {
    document.getElementById("logarea").value = "";
    getLog();
}

