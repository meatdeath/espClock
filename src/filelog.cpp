
#include <LittleFS.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <Arduino.h>
#include "filelog.h"
#include "rtc.h"

#define DISABLE_FILE_LOG


void fLog::Init() {
#ifndef DISABLE_FILE_LOG
    if( LittleFS.exists(filename) ) {
        Serial.println("Log file exist. Use it to initial write");
    } else {
        Serial.println("Log file doesn't exist. Creating it...");
        filelog = LittleFS.open(filename,"w");
        if(!filelog) {
            Serial.printf("ERROR creating file \"%s\".\r\n", filename.c_str());
        } else {
            filelog.close();
            Serial.println("File \"logfile.txt\" created.");
        }
    }
    printf("------------------------------------------------\r\n ");
    printf("Starting...\r\n");
#endif
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
#ifndef DISABLE_FILE_LOG
    va_list arg;
    va_start(arg, format);
Serial.print("file open... ");
    filelog = LittleFS.open(filename,"a");
    if(filelog) {
Serial.println("ok");
        if( printTimestamp )
        {
Serial.print("file printf time... ");
            len = filelog.printf( "[%s] ", rtc_GetTimeString().c_str() );
Serial.println("ok");
        }
Serial.print("file printf... ");
        len = filelog.printf(format, arg);
Serial.println("ok");
        filelog.close();
        if(format[strlen(format)-1] == '\n') printTimestamp = true;
        else printTimestamp = false;
    } else {
Serial.println("error");
    }
    va_end(arg);
#endif
    return len;
}

String fLog::readFirstString() {
#ifndef DISABLE_FILE_LOG
    readOffset = 0;
    return readNextString();
#else   
    return "";
#endif
}
String fLog::readNextString() {
    String line = "";
#ifndef DISABLE_FILE_LOG
    filelog = LittleFS.open(filename,"r");
    if(filelog) {
        filelog.seek(readOffset);
        if( filelog.available() ) {
            line = filelog.readStringUntil('\n');
            readOffset = filelog.position();
        } else {
            readOffset = 0;
        }
        filelog.close();
    } else {
    }
#endif
    return line;
}


fLog Log;
