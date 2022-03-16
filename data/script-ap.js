var time = 1642561783;
var h_offset = -5;
var m_offset = 0;

function setOffset() {
    h_offset = document.getElementById('hour_offset').value;
    m_offset = document.getElementById('minute_offset').value;

    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
    };
    param='?hour_offset=' + h_offset + '&minute_offset=' + m_offset;
    xhttp.open('GET', '/set_time_offset'+param, true);
    xhttp.send();
}

function GetFormattedDTString(dt) {
    return (
        "" + dt.getFullYear() + "-" +
        ("0"+(dt.getMonth()+1)).slice(-2) + "-" +
        ("0"+dt.getDate()).slice(-2) + " " +
        ("0"+dt.getHours()).slice(-2) + ":" +
        ("0"+dt.getMinutes()).slice(-2) + ":" +
        ("0"+dt.getSeconds()).slice(-2)
    );
}

function getCorrectionString() {
    return ""+h_offset+"h "+m_offset+"m";
}

function getFastTelemetry() {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() 
    {
        if (this.readyState == 4 && this.status == 200) 
        {
            var telemetry = JSON.parse(this.responseText);
            document.getElementById("pressure-text").innerText = telemetry.Pressure;
            document.getElementById("temperature-text").innerText = telemetry.Temperature;
            document.getElementById("utc-time-string").innerText = 
                ("0"+telemetry.Hours).slice(-2) + ":" + ("0"+telemetry.Minutes).slice(-2) + ":" + ("0"+telemetry.Seconds).slice(-2);
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

            h_offset = telemetry.HourOffset;
            m_offset = telemetry.MinuteOffset;
            document.getElementById("time-offset-string").innerText = getCorrectionString();
            document.getElementById("pressure-collection-time-left-text").innerText = 
                    (telemetry.SecondsUntilPressureCollection/60).toFixed() + "min " + 
                    ("0"+(telemetry.SecondsUntilPressureCollection%60)).slice(-2) + "sec";
        }
    };
    xhttp.open('GET', 'getFastTelemetry', true);
    xhttp.send();
}

function getPressureHistory() 
{
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() 
    {
        if (this.readyState == 4 && this.status == 200) 
        {
            var pressure = this.responseText;
            document.getElementById("pressure-text").innerText = pressure;
        }
    };
    xhttp.open('GET', 'getPressureHistory', true);
    xhttp.send();
}

window.onload = function() 
{    
    setInterval( getFastTelemetry, 1000 );
    getPressureHistory();
    getFastTelemetry();
    setInterval( getPressureHistory, 5000 );
}