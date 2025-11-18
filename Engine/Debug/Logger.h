#pragma once

#define ENABLE_LOGGING
#define KEEP_SPAM_LOGGING 1
#define LOG_TIME_ENABLED

#define DBG_CONSOLE_LOG_DEFAULT_ENABLED
#define DBG_MAX_LOG_FILE_LINES 10000
#define DBG_TIME_FORMAT "%H:%M:%S"

#define LOG_SPAM_COLOR          "\033[90m"
#define LOG_SPAM_COLOR_MSG      "\033[90m"
#define LOG_TRACE_COLOR         "\033[94m"
#define LOG_DEBUG_COLOR         "\033[36m"
#define LOG_INFO_COLOR          "\033[32m"
#define LOG_WARN_COLOR          "\033[93m"
#define LOG_ERROR_COLOR         "\033[91m"
#define LOG_FATAL_COLOR         "\033[41m"
#define LOG_IMPORTANT_COLOR_MSG "\033[97m"
#define LOG_END_SEQUENCE        "\033[0m"

#include <string>
#include <chrono>
#include <mutex>
#include <fstream>

namespace Debug{
#if KEEP_SPAM_LOGGING == 1
    void LogSpam(const std::string& message);
#else
    inline void LogSpam(const std::string&) {}
#endif

    void LogTrace(const std::string& message);
    void LogDebug(const std::string& message);
    void LogInfo(const std::string& message);
    void LogWarn(const std::string& message);
    void LogError(const std::string& message);

    // Fatal error instantly terminates the program after logging
    void LogFatal(const std::string& message);

class Logger{
public:
    // Fatal error instantly terminates the program after logging
    enum Level { SPAM, TRACE, DEBUG, INFO, WARN, ERROR, FATAL, NEVER };

    struct LogMessage{
        std::chrono::system_clock::time_point timestamp;
        Level level;
        std::string message;
    };

    struct ISink{
        virtual ~ISink() = default;
        virtual void Write(const LogMessage& message) = 0;
    };

    class ConsoleSink : public ISink{
    public:
        void Write(const LogMessage& message) override;
    private:
        std::mutex consoleMutex;
    };

    class FileSink : public ISink{
    public:
        FileSink(const std::string& filename);
        ~FileSink();
        void Write(const LogMessage& message) override;
    private:
        std::mutex fileMutex;
        std::ofstream logFile;
        uint64_t lineCount = 0;
    };

    void AddSink(ISink* sink);
    void ClearSinks();

    static Logger& Instance(){
        static Logger instance;
        return instance;
    }

    void Log(Level level, const std::string& message);
    Level minLogLevel = Level::NEVER;
private:
    Logger();
    ~Logger();

    static std::string FormatTime(const std::chrono::system_clock::time_point& timestamp);
    static std::string FormatRecord(const LogMessage& message, bool colorful);

    void WriteToSinks(const LogMessage& message);

    bool consoleColorSupported = true;
    std::vector<ISink*> sinks;
    std::mutex loggerMutex;
};

}