#ifndef _AS_LOG_H_
#define _AS_LOG_H_

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include "as_config.h"
#include "as_basetype.h"

#define ASLOG_API

typedef enum _ASLogLevel
{
    AS_LOG_EMERGENCY   = 0,
    AS_LOG_ALERT       = 1, 
    AS_LOG_CRITICAL    = 2,
    AS_LOG_ERROR       = 3,
    AS_LOG_WARNING     = 4, 
    AS_LOG_NOTICE      = 5,
    AS_LOG_INFO        = 6, 
    AS_LOG_DEBUG       = 7
}ASLogLevel;

ASLOG_API void ASSetLogLevel(int32_t lLevel);

ASLOG_API int32_t ASGetLogLevel();

ASLOG_API bool ASSetLogFilePathName(const char* szPathName);

ASLOG_API void ASSetLogFileLengthLimit(uint32_t ulLimitLengthKB);

ASLOG_API void ASStartLog(void);

ASLOG_API void ASWrite(const char* szFileName, int32_t lLine,
                             int32_t lLevel, const char* format, va_list argp);
ASLOG_API void ASWriteLog(int32_t lLevel,const char* szLog);
ASLOG_API void ASStopLog(void);

ASLOG_API void ASBackupLog(const char *strOldFileName);

class CDefaultWriter {
public:
        CDefaultWriter(const char* file, int32_t line) {
            m_file_ = file;
            m_line_ = line;
        }
        
        void operator()(int32_t level, const char* format, ...) {
            va_list argp;
            va_start(argp, format);
            ASWrite(m_file_,m_line_,level,format,argp);
            va_end(argp);
        }
    private:
        CDefaultWriter() {
            m_file_ = NULL;
            m_line_ = 0;
        }
        const char* m_file_;
        int32_t m_line_;
};


#define AS_LOG (CDefaultWriter(__FILE__, __LINE__))

#define AS_LOG_WRITE(file,line,level,format,...)    ASWrite(file,line,level,format,##__VA_ARGS__)

#define PrintLog(level, ...) AS_LOG(level, ##__VA_ARGS__)
#define PrintD(...)  PrintLog(AS_LOG_DEBUG, ##__VA_ARGS__)
#define PrintI(...)  PrintLog(AS_LOG_INFO, ##__VA_ARGS__)
#define PrintN(...)  PrintLog(AS_LOG_NOTICE, ##__VA_ARGS__)
#define PrintW(...)  PrintLog(AS_LOG_WARNING, ##__VA_ARGS__)
#define PrintE(...)  PrintLog(AS_LOG_ERROR, ##__VA_ARGS__)
#define PrintC(...)  PrintLog(AS_LOG_CRITICAL, ##__VA_ARGS__)
#define PrintA(...)  PrintLog(AS_LOG_ALERT, ##__VA_ARGS__)

#endif//_AS_LOG_H_

