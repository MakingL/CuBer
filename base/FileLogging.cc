#include "base/Logging.h"
#include "base/FileLogging.h"
#include "base/TimeZone.h"

using namespace cuber;

pthread_once_t FileLogging::ponce_ = PTHREAD_ONCE_INIT;
std::unique_ptr<MutexLock> FileLogging::mutex_(new MutexLock);
std::unordered_map<string, FileLoggerPtr > FileLogging::fileLoggerDict_;
detail::EmptyFileLogger *FileLogger::emptyLogger_ = new detail::EmptyFileLogger("nullLogger");

__thread char t_time[64];
__thread time_t t_lastSecond;

FileLogger::FileLogger(std::string name, std::string path, long long rollSize)
            : EmptyFileLogger(std::move(name)),
            asyncLogging_(path, rollSize) {
    asyncLogging_.start();    // start async logging thread
}

FileLogger::~FileLogger() {
    asyncLogging_.stop();
}

FileLoggerPtr FileLogging::getFileLogger(const char *name) {
    pthread_once(&ponce_, &FileLogging::registerProcessExit);

    {
        MutexLockGuard lock(*mutex_);

        if (fileLoggerDict_.count(name)) {
            return fileLoggerDict_[name];
        } else {
            LOG_WARN << "Bad logger name: " << name << ". Which wasn't configured.";
            return FileLoggerPtr(FileLogger::emptyLogger_);
        }
    }
}

FileLoggerPtr FileLogging::getFileLogger(const string& name) {
    pthread_once(&ponce_, &FileLogging::registerProcessExit);

    {
        MutexLockGuard lock(*mutex_);
        if (fileLoggerDict_.count(name)) {
            return fileLoggerDict_[name];
        } else {
            LOG_WARN << "Bad logger name: " << name << ". Which wasn't configured.";
            return FileLoggerPtr(FileLogger::emptyLogger_);
        }
    }
}

void FileLogging::addLogger(const char *name, const char *path, long long rollSize) {
    MutexLockGuard lock(*mutex_);

    // FIXME: check log path dir exists
    if (fileLoggerDict_.count(name) == 0) {
        if (rollSize == 0) {
            fileLoggerDict_[name] = new FileLogger(name, path);
        } else {
            fileLoggerDict_[name] = new FileLogger(name, path, rollSize);
        }
    }
}

void FileLogging::addLogger(const std::string& name, const std::string& path, long long rollSize) {
    MutexLockGuard lock(*mutex_);

    if (fileLoggerDict_.count(name) == 0) {
        if (rollSize == 0) {
            fileLoggerDict_[name] = new FileLogger(name, path);
        } else {
            fileLoggerDict_[name] = new FileLogger(name, path, rollSize);
        }
    }
}

void FileLogging::addLogger(const std::pair<const std::string, const std::string> &logPair) {
    MutexLockGuard lock(*mutex_);

    if (fileLoggerDict_.count(logPair.first) == 0) {
        fileLoggerDict_[logPair.first] = new FileLogger(logPair.first, logPair.second);
    }
}

void FileLogging::addLogger(std::initializer_list<std::pair<const std::string, const std::string> > dictLst) {
    for (auto &dict : dictLst) {
        addLogger(dict);
    }
}

string FileLogger::formatTime(Timestamp time_) {
    int64_t microSecondsSinceEpoch = time_.microSecondsSinceEpoch();
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch / Timestamp::kMicroSecondsPerSecond);
    int microseconds = static_cast<int>(microSecondsSinceEpoch % Timestamp::kMicroSecondsPerSecond);
    if (seconds != t_lastSecond) {
        t_lastSecond = seconds;
        struct tm tm_time;
        if (g_logTimeZone.valid()) {
            tm_time = g_logTimeZone.toLocalTime(seconds);
        } else {
            ::gmtime_r(&seconds, &tm_time); // FIXME TimeZone::fromUtcTime
        }

        int len = snprintf(t_time, sizeof(t_time), "%4d%02d%02d %02d:%02d:%02d",
                           tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                           tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
        assert(len == 17);
        (void) len;
    }

    string time_str;
    if (g_logTimeZone.valid()) {
        Fmt us(".%06d ", microseconds);
        assert(us.length() == 8);
        time_str += t_time;
        time_str += us.data();
    } else {
        Fmt us(".%06dZ ", microseconds);
        assert(us.length() == 9);
        time_str += t_time;
        time_str += us.data();
    }

    return time_str;
}
