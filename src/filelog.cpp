
#include <LittleFS.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <Arduino.h>
#include "filelog.h"
#include "rtc.h"


void fLog::Init() {
    if( LittleFS.exists(filename) ) {
        Serial.println("Log file exist. Opening it for initial write");
    } else {
        Serial.println("Log file doesn't exist. Creating it...");
    }
    filelog = LittleFS.open(filename,"a+");
    if(!filelog)
    {
        Serial.println("ERROR! Failed to open file \"logfile.txt\".");
    }
    else
    {
        Serial.printf("File opened. File position is %u", filelog.position() );
        printf("------------------------------------------------\r\n ");
        printf("Starting...\r\n");
    }
    filelog.close();
}
fLog::fLog() {
    readOffset = 0;
    printTimestamp = true;
    filename = "/logfile.txt";
}

fLog::fLog(const String log_file_name) {
    readOffset = 0;
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

String fLog::readFirstString() {
    readOffset = 0;
    return readNextString();
}
String fLog::readNextString() {
    String line = "";
    filelog = LittleFS.open(filename,"r");
    if(filelog) {
        filelog.seek(readOffset);
        line = filelog.readStringUntil('\n');
        readOffset = filelog.position();
        filelog.close();
    }
    return line;
}


fLog Log;
