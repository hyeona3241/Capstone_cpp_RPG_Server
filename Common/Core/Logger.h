#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <filesystem>

// 편의 매크로 
#define LOG_INFO(msg)::Logger::Log(::Logger::Level::Info, (msg))
#define LOG_WARN(msg)::Logger::Log(::Logger::Level::Warn, (msg))
#define LOG_ERROR(msg)::Logger::Log(::Logger::Level::Error, (msg))
#define LOG_FATAL(msg)::Logger::Log(::Logger::Level::Fatal, (msg))

namespace Logger {
    // { 정상 동작, 주의, 에러, 오류(종료할 수준의) }
    enum class Level : uint8_t { Info, Warn, Error, Fatal };

    struct Config {
        // 로그 적는 서버 이름
        std::string app_name = "Main";
        // 로그 루트 폴더
        std::filesystem::path dir = "logs";
        // 파일 용량 (10MB)
        size_t rotate_bytes = 10 * 1024 * 1024;
        // 보관 갯수 3개
        int keep_files = 3;
    };

    // 로거 시작
    void Init(const Config& cfg);
    // 로깅 정상 종료
    void Shutdown();
    // 기록된 내용 디스크로
    void Flush();

    // 로그를 비동기 큐에 넣기. 워커가 파일 입력
    void Log(Level lvl, const std::string& msg);

    // 시간 저장 (로컬타임, YYYY-MM-DD HH:MM:SS.mmm)
    std::string NowString();
}

