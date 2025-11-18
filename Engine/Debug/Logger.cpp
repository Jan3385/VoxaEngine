#include "Logger.h"

#include <iostream>

#ifdef _WIN32
#define NOGDI
#include <windows.h>
#endif

using namespace Debug;

void Logger::ConsoleSink::Write(const LogMessage &message)
{
    std::lock_guard<std::mutex> lock(consoleMutex);
    if(message.level <= Level::WARN)
        std::cout << FormatRecord(message, Logger::Instance().consoleColorSupported) << std::endl;
    else
        std::cerr << FormatRecord(message, Logger::Instance().consoleColorSupported) << std::endl;
}

Logger::FileSink::FileSink(const std::string &filename)
{
    std::lock_guard<std::mutex> lock(fileMutex);
    logFile.open(filename);
    if (!logFile.is_open()) {
        Debug::LogError("Failed to open log file: " + filename);
    }
}

Logger::FileSink::~FileSink()
{
    std::lock_guard<std::mutex> lock(fileMutex);
    if (logFile.is_open()) {
        logFile.close();
    }
}

void Logger::FileSink::Write(const LogMessage &message)
{
    std::lock_guard<std::mutex> lock(fileMutex);
    logFile << FormatRecord(message, false) << std::endl;
    lineCount++;

    if(lineCount >= DBG_MAX_LOG_FILE_LINES) {
        logFile.flush();
        lineCount = 0;
        this->Write(LogMessage{
            std::chrono::system_clock::now(), 
            Level::INFO, 
            "Log file flushed after reaching max line count"
        });
    }
}

void Logger::AddSink(ISink *sink)
{
    std::lock_guard<std::mutex> lock(loggerMutex);
    sinks.push_back(sink);
}

void Logger::ClearSinks()
{
    std::lock_guard<std::mutex> lock(loggerMutex);

    for(auto sink : sinks) {
        delete sink;
    }

    sinks.clear();
}

void Logger::Log(Level level, const std::string &message)
{
#ifdef ENABLE_LOGGING
    if(level < this->minLogLevel) return;
    if(level == Level::NEVER){
        Debug::LogWarn("You cannot log a message with level NEVER..");
        return;
    }

    LogMessage logMessage{
        std::chrono::system_clock::now(),
        level,
        message
    };

    WriteToSinks(logMessage);

    if(level == Level::FATAL){
        std::exit(EXIT_FAILURE);
    }
#endif
}
Logger::Logger()
{
    #ifdef _WIN32
    // enable ANSI escape codes on Windows
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (hOut != INVALID_HANDLE_VALUE && GetConsoleMode(hOut, &dwMode)) {
        if(!SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)){
            consoleColorSupported = false;
        }
    }
    #endif

    #ifdef DBG_CONSOLE_LOG_DEFAULT_ENABLED
    AddSink(new ConsoleSink());
    #endif
}
Logger::~Logger()
{
    ClearSinks();
}

std::string Logger::FormatTime(const std::chrono::system_clock::time_point &timestamp)
{
    auto t = std::chrono::system_clock::to_time_t(timestamp);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tm, DBG_TIME_FORMAT);
    return ss.str();
}

std::string Logger::FormatRecord(const LogMessage &message, bool colorful)
{
    std::ostringstream ss;

    if(colorful){
        switch (message.level) {
            case SPAM:  ss << "[" << LOG_SPAM_COLOR  << " SPAM  " << LOG_END_SEQUENCE << "] "; break;
            case TRACE: ss << "[" << LOG_TRACE_COLOR << " TRACE " << LOG_END_SEQUENCE << "] "; break;
            case DEBUG: ss << "[" << LOG_DEBUG_COLOR << " DEBUG " << LOG_END_SEQUENCE << "] "; break;
            case INFO:  ss << "[" << LOG_INFO_COLOR  << " INFO  " << LOG_END_SEQUENCE << "] "; break;
            case WARN:  ss << "[" << LOG_WARN_COLOR  << " WARN  " << LOG_END_SEQUENCE << "] "; break;
            case ERROR: ss << "[" << LOG_ERROR_COLOR << " ERROR " << LOG_END_SEQUENCE << "] "; break;
            case FATAL: ss << "[" << LOG_FATAL_COLOR << " FATAL " << LOG_END_SEQUENCE << "] "; break;
            default:    ss << "[" << LOG_FATAL_COLOR << "UNKNOWN" << LOG_END_SEQUENCE << "] "; break;
        }
    }else{
        switch (message.level) {
            case SPAM:  ss << "[ SPAM  ] "; break;
            case TRACE: ss << "[ TRACE ] "; break;
            case DEBUG: ss << "[ DEBUG ] "; break;
            case INFO:  ss << "[ INFO  ] "; break;
            case WARN:  ss << "[ WARN  ] "; break;
            case ERROR: ss << "[ ERROR ] "; break;
            case FATAL: ss << "[ FATAL ] "; break;
            default:    ss << "[UNKNOWN] "; break;
        }
    }

#ifdef LOG_TIME_ENABLED
    ss << LOG_SPAM_COLOR_MSG << "(" << FormatTime(message.timestamp) << ") " << LOG_END_SEQUENCE;
#endif

    ss << message.message;
    return ss.str();
}

void Logger::WriteToSinks(const LogMessage &message)
{
    std::lock_guard<std::mutex> lock(loggerMutex);
    for (auto sink : sinks) {
        sink->Write(message);
    }
}

#if KEEP_SPAM_LOGGING == 1
void Debug::LogSpam(const std::string &message)
{
    std::string coloredMessage = LOG_SPAM_COLOR_MSG + message + LOG_END_SEQUENCE;
    Logger::Instance().Log(Logger::Level::SPAM, coloredMessage);
}
#endif

void Debug::LogTrace(const std::string &message)
{
    Logger::Instance().Log(Logger::Level::TRACE, message);
}

void Debug::LogDebug(const std::string &message)
{
    Logger::Instance().Log(Logger::Level::DEBUG, message);
}

void Debug::LogInfo(const std::string &message)
{
    Logger::Instance().Log(Logger::Level::INFO, message);
}
void Debug::LogWarn(const std::string &message)
{
    Logger::Instance().Log(Logger::Level::WARN, message);
}

void Debug::LogError(const std::string &message)
{
    std::string coloredMessage = LOG_IMPORTANT_COLOR_MSG + message + LOG_END_SEQUENCE;
    Logger::Instance().Log(Logger::Level::ERROR, coloredMessage);
}

void Debug::LogFatal(const std::string &message)
{
    std::string coloredMessage = LOG_IMPORTANT_COLOR_MSG + message + LOG_END_SEQUENCE;
    Logger::Instance().Log(Logger::Level::FATAL, coloredMessage);
}
