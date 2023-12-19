#pragma once

#include <thread>
#include <mutex>
#include <iostream>
#include <queue>
#include <vector>

class ThreadPool {
    public:
        ThreadPool(uint32_t thread_count){
            initialize_threads(thread_count);
        }

        ThreadPool(){
            auto thread_count = std::thread::hardware_concurrency();
            initialize_threads(thread_count);
        }

        ~ThreadPool(){
            stop_pool();
        }

        void stop_pool(){
            {
                std::lock_guard<std::mutex> lock(mutex);
                should_terminate = true;
            }
            condition.notify_all();
            for(std::thread& worker : workers){
                worker.join();
            }
            workers.clear();
        }

        bool has_work(){
            bool poolbusy;
            {
                std::lock_guard<std::mutex> lock(mutex);
                poolbusy = !tasks.empty();
            }

            return poolbusy;
        }

        void initialize_threads(uint32_t thread_count){
            for(uint32_t i = 0; i < thread_count; i++){
                workers.push_back(std::thread(&ThreadPool::worker, this));
            }
        }

        void worker(){
            while(true){
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(mutex);
                    condition.wait(lock, [this]{
                            return !tasks.empty() || should_terminate;
                    });
                    if(should_terminate){
                        return;
                    }
                    task = tasks.front();
                    tasks.pop();
                }
                task();
            }
        }

        void add_task(std::function<void()> task) {
            {
                std::lock_guard<std::mutex> lock(mutex);
                tasks.push(task);
            }
            condition.notify_one();
        }



    private:
        bool should_terminate = false;
        std::vector<std::thread> workers;
        std::queue<std::function<void()>> tasks;
        std::condition_variable condition;
        std::mutex mutex;


};
