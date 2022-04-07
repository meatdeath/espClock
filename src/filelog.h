#ifndef __FILELOG_H__
#define __FILELOG_H__

class fLog {
    private:
        File filelog;
        String filename;
        bool printTimestamp;
    public:
        fLog();
        fLog(const String log_file_name);

        size_t printf(const char *format, ...)  __attribute__ ((format (printf, 2, 3)));
};

#endif //__FILELOG_H__