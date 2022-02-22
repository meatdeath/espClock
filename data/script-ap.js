var time = 1642561783;
var h_offset = -5;
var m_offset = 0;

function setOffset() {
    h_offset = document.getElementById('hour_offset').value;
    m_offset = document.getElementById('minutes_offset').value;

    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
    };
    param='?hour_offset=' + h_offset + '&minutes_offset=' + m_offset;
    xhttp.open('GET', '/set_time_offset'+param, true);
    xhttp.send();
}

// const myTimeout = setTimeout (reloadPage, 5*60*1000);
// function reloadPage() {
//     location.reload();
// }

function getTime() 
{
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() 
    {
        if (this.readyState == 4 && this.status == 200) 
        {
            var my_date = new Date(0); // The 0 there is the key, which sets the date to the epoch
            time = this.responseText;
            my_date.setUTCSeconds(time*1);
            document.getElementById("utc-time-string").innerText = my_date.toUTCString();
        }
    };
    xhttp.open('GET', 'getTime', true);
    xhttp.send();
}

function getOffset() 
{
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() 
    {
        if (this.readyState == 4 && this.status == 200) 
        {
            var my_date = new Date(0); // The 0 there is the key, which sets the date to the epoch
            var offset_min = this.responseText;

            my_date.setUTCSeconds(time*1 + offset_min*60);
            document.getElementById("corrected-time-string").innerText = my_date.toUTCString();//my_date.toISOString();

            h_offset = offset_min/60;
            m_offset = offset_min%60;
            document.getElementById("time-offset-string").innerText = ""+h_offset+"h "+m_offset+"m";
        }
    };
    xhttp.open('GET', 'getTimeOffset', true);
    xhttp.send();
}

function getPressure() 
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
    xhttp.open('GET', 'getPressure', true);
    xhttp.send();
}

function getTemperature() 
{
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() 
    {
        if (this.readyState == 4 && this.status == 200) 
        {
            var temperature = this.responseText;
            document.getElementById("temperature-text").innerText = temperature;
        }
    };
    xhttp.open('GET', 'getTemperature', true);
    xhttp.send();
}

window.onload = function() 
{
    setInterval( function() {
        getTime();
        getOffset();
        getPressure();
        getTemperature();
    }, 1000 );
}