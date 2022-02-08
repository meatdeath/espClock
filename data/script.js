
var time = 1642561783;
var h_offset = -5;
var m_offset = 0;

function setOffset() {
    h_offset = document.getElementById('hour_offset').value;
    m_offset = document.getElementById('minutes_offset').value;

    /*var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
    };
    param='?hour_offset=' + h_offset + '&minutes_offset=' + m_offset;
    xhttp.open('GET', '/time_offset'+param, true);
    xhttp.send();*/
}

// const myTimeout = setTimeout (reloadPage, 5*60*1000);
// function reloadPage() {
//     location.reload();
// }

function getTime() {
    return Math.floor((new Date).getTime()/1000);
}

function getCorrectedTime() {
    var time = Math.floor((new Date).getTime()/1000);
    time += h_offset*60*60;
    time += m_offset*60;
    return time;
}

function getCorrectionString() {
    return ""+h_offset+"h"+m_offset+"m";
}


window.onload = function() {
    document.getElementById("hour_offset").value = h_offset;
    document.getElementById("minutes_offset").value = m_offset;

    setInterval( function() {
        var utc_time = getTime();
        var corrected_time = getCorrectedTime();
        var correction = getCorrectionString();
        var my_date = new Date(0); // The 0 there is the key, which sets the date to the epoch
        my_date.setUTCSeconds(utc_time);
        document.getElementById("utc-time-string").innerText = my_date.toISOString();
        my_date = new Date(0); 
        my_date.setUTCSeconds(corrected_time);
        document.getElementById("corrected-time-string").innerText = my_date.toISOString();
        document.getElementById("time-offset-string").innerText = correction;
    }, 1000 );

}

