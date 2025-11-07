#include <iostream>
#include <queue>
#include <string>
#include <mutex>
#include <thread>
#include <chrono>
#include <vector>
#include <iomanip>
#include <algorithm>

// Global variables from your example
std::queue<std::string> g_messages;
std::mutex g_message_mutex;

// Constants for testing
const int NUM_THREADS = 8; // Number of handler threads
const int MESSAGES_PER_RUN = 100000; // Total number of messages to process
const int iterations = 1000000; // Number of iterations to simulate a long operation

volatile int dummy = 0; // To prevent compiler optimizations
void long_calc(const std::string &msg) {
(void)msg;
for ( int i = 0; i < iterations; i++ ) dummy += 1;
}

void handle_uniqiue_lock()
{
  std::unique_lock<std::mutex> lock(g_message_mutex);  
  if (!g_messages.empty())
    {
        std::string  msg_process = std::move(g_messages.front());
    	g_messages.pop();
  		lock.unlock();
        long_calc(msg_process);
    }
}


void handle_lock_guard()
{
    std::string msg_process;
    {
        std::lock_guard<std::mutex> lock(g_message_mutex);  
        if(g_messages.empty()) return;

        msg_process = std::move(g_messages.front());
        g_messages.pop();

    }
    long_calc(msg_process);
}

void handle_scoped_lock()
{
    std::string msg_process;
    {
        std::scoped_lock<std::mutex> lock(g_message_mutex);  
        if(g_messages.empty()) return;

        msg_process = std::move(g_messages.front());
        g_messages.pop();

    }
    long_calc(msg_process);
}

void populate_queue(int count) {
    std::lock_guard<std::mutex> lock(g_message_mutex);
    for (int i = 0; i < count; ++i) {
        g_messages.push("Message " + std::to_string(i));
    }
}

void worker_thread(void (*handler)()) {
    while (true) {
        bool processed = false;
        {
            std::unique_lock<std::mutex> lock(g_message_mutex);
            if (!g_messages.empty()) {
            } else {
                break; 
            }
        }

        handler();
    }
}

std::chrono::microseconds run_test(void (*handler)()) {
    populate_queue(MESSAGES_PER_RUN);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([handler]() {
            while (true) {
                bool has_more = false;
                {
                    std::lock_guard<std::mutex> lock(g_message_mutex);
                    if (!g_messages.empty()) {
                        has_more = true;
                    }
                }
                
                if (has_more) {
                    handler();
                } else {
                    break;
                }
            }
        });
    }

    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    
    {
        std::lock_guard<std::mutex> lock(g_message_mutex);
        while (!g_messages.empty()) {
            g_messages.pop();
        }
    }

    return std::chrono::duration_cast<std::chrono::microseconds>(end - start);
}

int main() {
    std::cout << "--- Message Handler Performance Test ---" << std::endl;
    std::cout << "Threads: " << NUM_THREADS << std::endl;
    std::cout << "Messages per run: " << MESSAGES_PER_RUN << std::endl;
    std::cout << "Long-running operation: " << iterations << " loop iterations" << std::endl;
    std::cout << "-----------------------------------------------------" << std::endl;
    
    // handle_unique_lock
    auto time1 = run_test(handle_uniqiue_lock);
    std::cout << "Handle_unique_lock(): " << time1.count() << " µs" << std::endl;
    
    // handle_lock_guard
    auto time2 = run_test(handle_lock_guard);
    std::cout << "Handle_lock_guard(): " << time2.count() << " µs" << std::endl;
    
    // handle_scoped_lock
    auto time3 = run_test(handle_scoped_lock);
    std::cout << "Handle_scoped_lock(): " << time3.count() << " µs" << std::endl;
    
    std::cout << "-----------------------------------------------------" << std::endl;
    std::cout << "=== PERFORMANCE COMPARISON ===" << std::endl;
    std::cout << "-----------------------------------------------------" << std::endl;
    
    // Знаходимо найкращий результат
    auto min_time = std::min({time1, time2, time3});
    
    std::cout << std::fixed << std::setprecision(2);  // Precision 2 знаків
    
    std::cout << "unique_lock: " << time1.count() << " µs ";
    if (time1 == min_time) {
        std::cout << "🏆 WINNER!" << std::endl;
    } else {
        double diff_percent = ((time1.count() - min_time.count()) * 100.0 / (double)min_time.count());
        std::cout << "(+" << diff_percent << "% slower)" << std::endl;
    }
    
    std::cout << "lock_guard:  " << time2.count() << " µs ";
    if (time2 == min_time) {
        std::cout << "🏆 WINNER!" << std::endl;
    } else {
        double diff_percent = ((time2.count() - min_time.count()) * 100.0 / (double)min_time.count());
        std::cout << "(+" << diff_percent << "% slower)" << std::endl;
    }
    
    std::cout << "scoped_lock: " << time3.count() << " µs ";
    if (time3 == min_time) {
        std::cout << "WINNER!" << std::endl;
    } else {
        double diff_percent = ((time3.count() - min_time.count()) * 100.0 / (double)min_time.count());
        std::cout << "(+" << diff_percent << "% slower)" << std::endl;
    }
    
    std::cout << "-----------------------------------------------------" << std::endl;
    
    // Додаткова статистика
    auto max_time = std::max({time1, time2, time3});
    double range_percent = ((max_time.count() - min_time.count()) / (double)min_time.count()) * 100.0;
    
    std::cout << "Performance range: " << range_percent << "%" << std::endl;
    std::cout << "Best time: " << min_time.count() << " µs" << std::endl;
    std::cout << "Worst time: " << max_time.count() << " µs" << std::endl;
    std::cout << "-----------------------------------------------------" << std::endl;
    
    return 0;
}
