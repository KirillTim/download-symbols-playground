#ifndef _TIMER_H
#define _TIMER_H

#include <chrono>
#include <stdexcept>

class CodeExecutionTimer {
public:
  CodeExecutionTimer &start() {
    if (_running) throw std::logic_error("CodeExecutionTimer is already running");
    _start = std::chrono::steady_clock::now();
    _running = true;
    _stopped = false;
    return *this;
  }

  const CodeExecutionTimer &stop() {
    if (!_running) throw std::logic_error("CodeExecutionTimer is not running");
    if (_stopped) throw std::logic_error("CodeExecutionTimer is already stopped");
    _end = std::chrono::steady_clock::now();
    _running = false;
    _stopped = true;
    return *this;
  }

  auto elapsed_time_nanoseconds() const {
    auto end_point = _stopped ? _end : std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end_point - _start).count();
  }

  auto elapsed_time_milliseconds() const {
    auto end_point = _stopped ? _end : std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(end_point - _start).count();
  }

  auto elapsed_time_seconds() const {
    auto end_point = _stopped ? _end : std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(end_point - _start).count();
  }

private:
  bool _running = false;
  bool _stopped = false;
  std::chrono::time_point<std::chrono::steady_clock> _start;
  std::chrono::time_point<std::chrono::steady_clock> _end;
};

#endif //_TIMER_H
