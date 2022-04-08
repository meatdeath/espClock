#ifndef __FILELOG_H__
#define __FILELOG_H__

class fLog {
    private:
        File filelog;
        String filename;
        bool printTimestamp;
        uint32_t readOffset;
    public:
        fLog();
        fLog(const String log_file_name);
        void Init();

        size_t printf(const char *format, ...)  __attribute__ ((format (printf, 2, 3)));
        String readFirstString();
        String readNextString();
};

extern fLog Log;

#endif //__FILELOG_H__