set(base_SRCS
        AsyncLogging.cc
        Condition.cc
        CountDownLatch.cc
        CurrentThread.cc
        Date.cc
        Exception.cc
        FileUtil.cc
        LogFile.cc
        Logging.cc
        FileLogging.cc
        LogStream.cc
        ProcessInfo.cc
        Timestamp.cc
        Thread.cc
        ThreadPool.cc
        TimeZone.cc
        )

add_library(cuber_base ${base_SRCS})
target_link_libraries(cuber_base pthread rt)

file(GLOB HEADERS "*.h")
