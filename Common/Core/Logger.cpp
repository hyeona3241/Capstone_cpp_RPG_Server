#include "Logger.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cstdio>
#include <system_error>

#ifdef _WIN32
	#include <windows.h>
	#include <io.h> 
#endif

namespace Logger {

	namespace {

		// ť�� �� 1�� ���ڵ� ���� (�α� ó��?)
		struct Record {
			Level lvl;
			std::string msg;
		};

        // �ΰ� ���� ����
        struct State {
            Config cfg; // �ʱ�ȭ ���� 
            std::mutex mtx; // ť ��ȣ ���ؽ�
            std::mutex io_mtx;  // ���� ��Ʈ�� ��ȣ��
            std::condition_variable cv; // �� �αװ� ������ ��ȣ�� ����
            std::deque<Record> q;   // �α� ���ڵ� ť
            std::atomic<bool> running{ false }; // ��Ŀ �����ֱ�
            std::thread worker; // ���� ���� ���� ��Ŀ ������

            // ���� �Ϲ� / ������ �з��ؼ� ����
            // ������ ���� ����Ʈ
            std::ofstream file_all; 
            std::ofstream file_err;
            size_t bytes_all = 0;
            size_t bytes_err = 0;

            // ���� ����(app_name, dir)�� �������� ���� ��� ����
            std::filesystem::path path_all() const { return cfg.dir / (cfg.app_name + ".log"); }
            std::filesystem::path path_err() const { return cfg.dir / (cfg.app_name + "_error.log"); }
        };

        // ���� ����(State) ��ü ����
        State& S() { static State s; return s; }

        // �α׿� ���� ���ڿ��� ��ȯ
        inline const char* LvlName(Level l) {
            switch (l) {
            case Level::Info:  return "INFO";
            case Level::Warn:  return "WARN";
            case Level::Error: return "ERROR";
            case Level::Fatal: return "FATAL";
            }
            return "INFO";
        }

        // �α� ���丮 (���� ���) ����
        void ensure_dir(const std::filesystem::path& p) {
            std::error_code ec;
            std::filesystem::create_directories(p, ec);
        }

        // ���� ȸ�� (�뷮�� �� ä������ ���� ���� �ڷ� �������� �ϰ� ���Ӱ� ����)
        void rotate_files(const std::filesystem::path& base, int keep) {
            for (int i = keep; i >= 1; --i) {
                const std::string stem = base.stem().string(); 
                const std::string ext = base.extension().string();
                const auto parent = base.parent_path();

                const auto src = (i == 1)
                    ? base
                    : parent / (stem + "_" + std::to_string(i - 1) + ext);

                const auto dst = parent / (stem + "_" + std::to_string(i) + ext);

                std::error_code ec;
                if (std::filesystem::exists(dst, ec)) {
                    std::filesystem::remove(dst, ec); // �̹� �ִٸ� ����� ���� ����
                }
                if (std::filesystem::exists(src, ec)) {
                    std::filesystem::rename(src, dst, ec); // �����ص� ���� ������ ����
                }
            }
        }

        // ���� ����, ���� ũ�� ���
        void open_stream(std::ofstream& f, const std::filesystem::path& p, size_t& bytes) {
            f.close();
            f.open(p, std::ios::out | std::ios::app | std::ios::binary);

            std::error_code ec;
            bytes = std::filesystem::exists(p, ec) ? std::filesystem::file_size(p, ec) : 0;

            if (!f.is_open()) {
                // ��ũ ����/��� ���� ���
                // ���߿� ���� ���� ���� �ȸ��߰� ���⼭ ����
            }
        }

        // �α� ����, �뷮 �Ѿ����� �˻絵 ��
        void write_line(std::ofstream& f, const std::string& line, size_t& counter, 
            const std::filesystem::path& base, const Config& cfg) {
            if (!f.is_open()) return; // ������ ������ ������ ��ŵ

            // ���� ����
            f.write(line.data(), static_cast<std::streamsize>(line.size()));
            f.put('\n');

            // ���� �뷮 ����
            counter += (line.size() + 1);

            // ȸ�� ���� �˻�
            if (counter >= cfg.rotate_bytes) {
                f.flush();
                f.close(); 

                // ���� ���ϵ��� �ڷ� �б�
                rotate_files(base, cfg.keep_files);

                // base�� ���� ���� �� �ٽ� ����(���� ���� ���Ϸ�)
                {
                    std::ofstream nf(base, std::ios::out | std::ios::trunc | std::ios::binary);
                }
                open_stream(f, base, counter);
            }
        }

        // (�ð�, ����, �ؽ�Ʈ)�� �α� ���� ����
        std::string format_line(Level lvl, const std::string& msg) {
            std::ostringstream oss;

            oss << NowString() << " | " << LvlName(lvl) << " | " << msg;
            return oss.str();
        }

        // ���� ���� ���� ��Ŀ ������ ����
        void worker_loop() {
            auto& st = S();

            while (st.running.load(std::memory_order_acquire)) {
                // 1) ť���� ��ġ�� ������
                std::deque<Record> batch;
                {
                    std::unique_lock<std::mutex> lk(st.mtx);

                    if (st.q.empty()) {
                        // 200ms ���� ���, notify_one()�� ������ ��� �����.
                        st.cv.wait_for(lk, std::chrono::milliseconds(200));
                    }

                    batch.swap(st.q); // ť�� ��°�� ���� batch�� �̵�
                }

                // 2) ��ġ�� ���Ͽ� ���������� ���
                for (auto& r : batch) {
                    const std::string line = format_line(r.lvl, r.msg);

                    {
                        std::lock_guard<std::mutex> g(st.io_mtx);

                        // ��� ������ �Ϲ� �α׷�
                        write_line(st.file_all, line, st.bytes_all, st.path_all(), st.cfg);

                        // Warn �̻��� ���� �α׿��� �ߺ� ���
                        if (r.lvl >= Level::Warn) {
                            write_line(st.file_err, line, st.bytes_err, st.path_err(), st.cfg);
                        }

                        // Fatal�� ��� flush
                        if (r.lvl == Level::Fatal) {
                            st.file_all.flush();
                            st.file_err.flush();
                        }
                    }
                }
                {
                    // 3) ��ҿ��� �ֱ������� flush
                    std::lock_guard<std::mutex> g(st.io_mtx);
                    st.file_all.flush();
                    st.file_err.flush();
                }
            }

            // 4) ���� �ܰ�. ť�� ���� ���ڵ� ����
            {
                std::lock_guard<std::mutex> lk(st.mtx);

                while (!st.q.empty()) {
                    auto r = std::move(st.q.front()); st.q.pop_front();
                    const std::string line = format_line(r.lvl, r.msg);

                    write_line(st.file_all, line, st.bytes_all, st.path_all(), st.cfg);

                    if (r.lvl >= Level::Warn) {
                        write_line(st.file_err, line, st.bytes_err, st.path_err(), st.cfg);
                    }
                }
            }
            st.file_all.flush();
            st.file_err.flush();
        }

	}

    // ���� Ÿ�ӽ����� ���ڿ� ����
    std::string NowString() {
        using clock = std::chrono::system_clock;
        const auto tp = clock::now();
        const auto t = clock::to_time_t(tp);
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()) % std::chrono::seconds(1);

        std::tm tm{};
#ifdef _WIN32
        // Windows ���� �Լ�
        localtime_s(&tm, &t);
#else
        // POSIX ���� (�ּ� ó��)
        // localtime_r(&t, &tm);
#endif

        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);

        std::ostringstream oss;
        oss << buf << '.' << std::setw(3) << std::setfill('0') << ms.count();
        return oss.str();
    }

    // �ΰ� �ʱ�ȭ
    void Init(const Config& cfg) {
        auto& st = S();
        st.cfg = cfg;

        // �α� ��Ʈ ���� ���� (������ ����)
        ensure_dir(cfg.dir);

        // �⺻ ���ϵ� ���� (append ���)
        open_stream(st.file_all, st.path_all(), st.bytes_all);
        open_stream(st.file_err, st.path_err(), st.bytes_err);

        // ��Ŀ ���� ����
        st.running.store(true, std::memory_order_release);
        st.worker = std::thread(worker_loop);

        // �ʱ�ȭ �α� 1�� (���⼭���� �α� ��� ����)
        Log(Level::Info, "Logger init app=" + cfg.app_name);
    }

    // �α� ���� ����
    void Shutdown() {
        auto& st = S();

        // ���� ���� �α�
        Log(Level::Info, "Logger shutdown begin");

        // ��Ŀ ���� �÷��� ����, �����
        st.running.store(false, std::memory_order_release);
        st.cv.notify_all();

        // ��Ŀ ���� ���
        if (st.worker.joinable()) st.worker.join();

        // ���� ��Ʈ�� ����
        st.file_all.flush(); st.file_all.close();
        st.file_err.flush(); st.file_err.close();
    }

    // ���� ���� ��Ʈ�� ���۸� OS�� �о� �ֱ�
    void Flush() {
        auto& st = S();
        std::lock_guard<std::mutex> g(st.io_mtx);

        st.file_all.flush();
        st.file_err.flush();
    }

    void Log(Level lvl, const std::string& msg) {
        auto& st = S();

        // �ΰŰ� ���� ���� ������ �ٷ� ��ȯ
        if (!st.running.load(std::memory_order_acquire)) return; // Init ����/Shutdown ���� ��ȣ

        // ť�� ���� �� ���ؽ� ��ȣ
        {
            std::lock_guard<std::mutex> lk(st.mtx);
            st.q.push_back(Record{ lvl, msg });
        }

        // ��Ŀ ����
        st.cv.notify_one();
    }

}