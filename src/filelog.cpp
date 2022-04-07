
#include <LittleFS.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <Arduino.h>
#include "filelog.h"
#include "rtc.h"


fLog::fLog() {
    printTimestamp = true;
    if( LittleFS.exists("/logfile.txt") ) {
        Serial.println("Log file exist. Opening it for initial write");
    } else {
        Serial.println("Log file doesn't exist. Creating it...");
    }
    filelog = LittleFS.open("/logfile.txt","a");
    if(!filelog)
    {
        Serial.println("ERROR\r\nFailed to open file \"logfile.txt\".");
    }
    else
    {
        Serial.printf("File opened. File position is %u", filelog.position() );
        printf("------------------------------------------------\r\n ");
        printf("Starting...\r\n");
        filename = "/logfile.txt";
    }
    filelog.close();
}

fLog::fLog(const String log_file_name) {
    printTimestamp = true;
    filename = log_file_name;
}

size_t fLog::printf(const char *format, ...) {
    size_t len = 0;
    va_list arg;
    va_start(arg, format);

    filelog = LittleFS.open(filename,"a");
    if(filelog) {
        if( printTimestamp )
        {
            len = filelog.printf( rtc_GetTimeString().c_str() );
        }
        len = filelog.printf(format, arg);
        filelog.close();
        if(format[strlen(format)-1] == '\n') printTimestamp = true;
        else printTimestamp = false;
    }
    va_end(arg);

    return len;
}