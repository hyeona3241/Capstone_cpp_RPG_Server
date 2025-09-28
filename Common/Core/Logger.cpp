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

		// 큐에 들어갈 1개 레코드 정의 (로그 처리?)
		struct Record {
			Level lvl;
			std::string msg;
		};

        // 로거 전역 상태
        struct State {
            Config cfg; // 초기화 설정 
            std::mutex mtx; // 큐 보호 뮤텍스
            std::mutex io_mtx;  // 파일 스트림 보호용
            std::condition_variable cv; // 새 로그가 들어오면 신호를 보냄
            std::deque<Record> q;   // 로그 레코드 큐
            std::atomic<bool> running{ false }; // 워커 생명주기
            std::thread worker; // 파일 쓰기 전용 워커 스레드

            // 파일 일반 / 에러로 분류해서 저장
            // 각각의 누적 바이트
            std::ofstream file_all; 
            std::ofstream file_err;
            size_t bytes_all = 0;
            size_t bytes_err = 0;

            // 현재 설정(app_name, dir)을 바탕으로 파일 경로 생성
            std::filesystem::path path_all() const { return cfg.dir / (cfg.app_name + ".log"); }
            std::filesystem::path path_err() const { return cfg.dir / (cfg.app_name + "_error.log"); }
        };

        // 전역 상태(State) 객체 생성
        State& S() { static State s; return s; }

        // 로그에 들어가게 문자열로 변환
        inline const char* LvlName(Level l) {
            switch (l) {
            case Level::Info:  return "INFO";
            case Level::Warn:  return "WARN";
            case Level::Error: return "ERROR";
            case Level::Fatal: return "FATAL";
            }
            return "INFO";
        }

        // 로그 디렉토리 (파일 경로) 생성
        void ensure_dir(const std::filesystem::path& p) {
            std::error_code ec;
            std::filesystem::create_directories(p, ec);
        }

        // 파일 회전 (용량이 다 채워지면 파일 순서 뒤로 물러나게 하고 새롭게 생성)
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
                    std::filesystem::remove(dst, ec); // 이미 있다면 덮어쓰기 위해 삭제
                }
                if (std::filesystem::exists(src, ec)) {
                    std::filesystem::rename(src, dst, ec); // 실패해도 예외 던지지 않음
                }
            }
        }

        // 파일 열고, 파일 크기 기록
        void open_stream(std::ofstream& f, const std::filesystem::path& p, size_t& bytes) {
            f.close();
            f.open(p, std::ios::out | std::ios::app | std::ios::binary);

            std::error_code ec;
            bytes = std::filesystem::exists(p, ec) ? std::filesystem::file_size(p, ec) : 0;

            if (!f.is_open()) {
                // 디스크 권한/경로 문제 대비
                // 나중에 오류 떠도 서버 안멈추게 여기서 수정
            }
        }

        // 로그 적기, 용량 넘었는지 검사도 함
        void write_line(std::ofstream& f, const std::string& line, size_t& counter, 
            const std::filesystem::path& base, const Config& cfg) {
            if (!f.is_open()) return; // 파일이 열리지 않으면 스킵

            // 실제 쓰기
            f.write(line.data(), static_cast<std::streamsize>(line.size()));
            f.put('\n');

            // 누적 용량 증가
            counter += (line.size() + 1);

            // 회전 조건 검사
            if (counter >= cfg.rotate_bytes) {
                f.flush();
                f.close(); 

                // 기존 파일들을 뒤로 밀기
                rotate_files(base, cfg.keep_files);

                // base를 새로 생성 후 다시 열기(수정 가능 파일로)
                {
                    std::ofstream nf(base, std::ios::out | std::ios::trunc | std::ios::binary);
                }
                open_stream(f, base, counter);
            }
        }

        // (시간, 레벨, 텍스트)로 로그 적는 포맷
        std::string format_line(Level lvl, const std::string& msg) {
            std::ostringstream oss;

            oss << NowString() << " | " << LvlName(lvl) << " | " << msg;
            return oss.str();
        }

        // 파일 쓰기 전용 워커 스레드 루프
        void worker_loop() {
            auto& st = S();

            while (st.running.load(std::memory_order_acquire)) {
                // 1) 큐에서 배치로 빼오기
                std::deque<Record> batch;
                {
                    std::unique_lock<std::mutex> lk(st.mtx);

                    if (st.q.empty()) {
                        // 200ms 동안 대기, notify_one()을 받으면 즉시 깨어난다.
                        st.cv.wait_for(lk, std::chrono::milliseconds(200));
                    }

                    batch.swap(st.q); // 큐를 통째로 비우고 batch로 이동
                }

                // 2) 배치를 파일에 순차적으로 기록
                for (auto& r : batch) {
                    const std::string line = format_line(r.lvl, r.msg);

                    {
                        std::lock_guard<std::mutex> g(st.io_mtx);

                        // 모든 레벨은 일반 로그로
                        write_line(st.file_all, line, st.bytes_all, st.path_all(), st.cfg);

                        // Warn 이상은 에러 로그에도 중복 기록
                        if (r.lvl >= Level::Warn) {
                            write_line(st.file_err, line, st.bytes_err, st.path_err(), st.cfg);
                        }

                        // Fatal은 즉시 flush
                        if (r.lvl == Level::Fatal) {
                            st.file_all.flush();
                            st.file_err.flush();
                        }
                    }
                }
                {
                    // 3) 평소에도 주기적으로 flush
                    std::lock_guard<std::mutex> g(st.io_mtx);
                    st.file_all.flush();
                    st.file_err.flush();
                }
            }

            // 4) 종료 단계. 큐에 남은 레코드 적기
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

    // 로컬 타임스탬프 문자열 생성
    std::string NowString() {
        using clock = std::chrono::system_clock;
        const auto tp = clock::now();
        const auto t = clock::to_time_t(tp);
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()) % std::chrono::seconds(1);

        std::tm tm{};
#ifdef _WIN32
        // Windows 안전 함수
        localtime_s(&tm, &t);
#else
        // POSIX 예시 (주석 처리)
        // localtime_r(&t, &tm);
#endif

        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);

        std::ostringstream oss;
        oss << buf << '.' << std::setw(3) << std::setfill('0') << ms.count();
        return oss.str();
    }

    // 로거 초기화
    void Init(const Config& cfg) {
        auto& st = S();
        st.cfg = cfg;

        // 로그 루트 폴더 생성 (없으면 생성)
        ensure_dir(cfg.dir);

        // 기본 파일들 오픈 (append 모드)
        open_stream(st.file_all, st.path_all(), st.bytes_all);
        open_stream(st.file_err, st.path_err(), st.bytes_err);

        // 워커 동작 시작
        st.running.store(true, std::memory_order_release);
        st.worker = std::thread(worker_loop);

        // 초기화 로그 1줄 (여기서부터 로깅 사용 가능)
        Log(Level::Info, "Logger init app=" + cfg.app_name);
    }

    // 로깅 정상 종료
    void Shutdown() {
        auto& st = S();

        // 종료 시작 로그
        Log(Level::Info, "Logger shutdown begin");

        // 워커 종료 플래그 설정, 깨우기
        st.running.store(false, std::memory_order_release);
        st.cv.notify_all();

        // 워커 종료 대기
        if (st.worker.joinable()) st.worker.join();

        // 파일 스트림 정리
        st.file_all.flush(); st.file_all.close();
        st.file_err.flush(); st.file_err.close();
    }

    // 현재 열린 스트림 버퍼를 OS로 밀어 넣기
    void Flush() {
        auto& st = S();
        std::lock_guard<std::mutex> g(st.io_mtx);

        st.file_all.flush();
        st.file_err.flush();
    }

    void Log(Level lvl, const std::string& msg) {
        auto& st = S();

        // 로거가 켜져 있지 않으면 바로 반환
        if (!st.running.load(std::memory_order_acquire)) return; // Init 이전/Shutdown 이후 보호

        // 큐에 넣을 때 뮤텍스 보호
        {
            std::lock_guard<std::mutex> lk(st.mtx);
            st.q.push_back(Record{ lvl, msg });
        }

        // 워커 깨움
        st.cv.notify_one();
    }

}