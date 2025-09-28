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

// ���� ��ũ�� 
#define LOG_INFO(msg)::Logger::Log(::Logger::Level::Info, (msg))
#define LOG_WARN(msg)::Logger::Log(::Logger::Level::Warn, (msg))
#define LOG_ERROR(msg)::Logger::Log(::Logger::Level::Error, (msg))
#define LOG_FATAL(msg)::Logger::Log(::Logger::Level::Fatal, (msg))

namespace Logger {
    // { ���� ����, ����, ����, ����(������ ������) }
    enum class Level : uint8_t { Info, Warn, Error, Fatal };

    struct Config {
        // �α� ���� ���� �̸�
        std::string app_name = "Main";
        // �α� ��Ʈ ����
        std::filesystem::path dir = "logs";
        // ���� �뷮 (10MB)
        size_t rotate_bytes = 10 * 1024 * 1024;
        // ���� ���� 3��
        int keep_files = 3;
    };

    // �ΰ� ����
    void Init(const Config& cfg);
    // �α� ���� ����
    void Shutdown();
    // ��ϵ� ���� ��ũ��
    void Flush();

    // �α׸� �񵿱� ť�� �ֱ�. ��Ŀ�� ���� �Է�
    void Log(Level lvl, const std::string& msg);

    // �ð� ���� (����Ÿ��, YYYY-MM-DD HH:MM:SS.mmm)
    std::string NowString();
}

