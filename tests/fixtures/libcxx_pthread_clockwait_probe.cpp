#include <chrono>
#include <condition_variable>
#include <mutex>

void libcxx_pthread_clockwait_probe(std::condition_variable& condition,
	                            std::unique_lock<std::mutex>& lock)
{
	condition.wait_until(lock, std::chrono::steady_clock::time_point());
}
