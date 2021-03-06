#ifndef _LOGGER_H
#define _LOGGER_H

#include <pthread.h>
#include "Buffer.h"
#include "Noncopyable.h"

class EventLoop;

class Logger : Noncopyable {
public:
    Logger();
    ~Logger();
    enum level {
        DEBUG   = 001,
        INFO    = 002,
        WARN    = 004,
        ERROR   = 010,
    };
    void wakeUp();
    void output(int level, const char *file, int line, 
            const char *func, const char *fmt, ...);
    const char *levelStr(int level);
    void quit() 
    { 
        _quit = 1; 
        _writeBuf.swap(_flushBuf);
        pthread_cond_signal(&_cond);
    }
private:
    void writeToBuffer(const char *s, size_t len);
    static void *flushToFile(void *arg);
    Buffer _writeBuf;
    Buffer _flushBuf;
    pthread_t _tid;
    pthread_mutex_t _mutex;
    pthread_cond_t _cond;
    int _quit; 
};

extern Logger *_log;

#define logDebug(...) \
    _log->output(Logger::DEBUG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define logInfo(...) \
    _log->output(Logger::INFO, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define logWarn(...) \
    _log->output(Logger::WARN, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define logError(...) \
    _log->output(Logger::ERROR, __FILE__, __LINE__, __func__, __VA_ARGS__)

#endif // _LOGGER_H
