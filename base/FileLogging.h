#ifndef CUBER_FILELOGGING_H
#define CUBER_FILELOGGING_H

#include "base/Mutex.h"
#include "base/Logging.h"
#include "base/noncopyable.h"
#include "base/AsyncLogging.h"
#include <cstdlib> // atexit
#include <pthread.h>
#include <unordered_map>

namespace cuber {

class TimeZone;

class AsyncLogging;

namespace detail {
    class EmptyFileLogger : public noncopyable {
    private:
        std::string name_;

    public:
        explicit EmptyFileLogger(std::string name) : name_(std::move(name)) {
        }

        virtual ~EmptyFileLogger() = default;

        const std::string &name() const {
            return name_;
        }

    public:
        /* Null Logger. Do nothing */
        virtual void info(string &str) {
        }

        virtual void info(LogStream &stream) {
        }
    };
}

class FileLogger : public detail::EmptyFileLogger {
private:
    explicit FileLogger(std::string name, std::string path, long long rollSize=ROLL_SIZE);
    ~FileLogger() final;

    friend class FileLogging;   // FileLogging new FileLogger Object
public:
    void info(string &str) override {
        asyncLogging_.append(str.c_str(), str.length());
    }

    void info(LogStream &stream) override {
        const LogStream::Buffer &buf(stream.buffer());
        asyncLogging_.append(buf.data(), buf.length());
    }

    static string formatTime(Timestamp time_);
private:
    static const long long ROLL_SIZE = 500*1024*1024;
    static EmptyFileLogger *emptyLogger_;

    AsyncLogging asyncLogging_;
};

typedef FileLogger *FileLoggerPtr;

class FileLogging : noncopyable {
public:
    FileLogging() = delete;
    ~FileLogging() = delete;

public:
    static FileLoggerPtr getFileLogger(const char *name);
    static FileLoggerPtr getFileLogger(const string &name);

    static void addLogger(const char *name, const char *path, long long rollSize=0);
    static void addLogger(const std::string &name, const std::string &path, long long rollSize=0);
    static void addLogger(const std::pair<const std::string, const std::string> &logPair);
    static void addLogger(std::initializer_list<std::pair<const std::string, const std::string> > dictLst);

private:
    static void onProcessExit() {
        for (auto &logger : fileLoggerDict_) {
            // delete file logger for stop flush logger
            delete logger.second;
        }
    }

    static void registerProcessExit() {
        ::atexit(onProcessExit);
    }

private:
    static std::unordered_map<std::string, FileLoggerPtr> fileLoggerDict_;
    static pthread_once_t ponce_;

    static std::unique_ptr<MutexLock> mutex_;
};

} // cuber

#endif //CUBER_FILELOGGING_H
