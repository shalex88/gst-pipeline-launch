#include "Scheduler.h"
#include <utility>

Scheduler::Scheduler(size_t thread_count) : thread_count_(thread_count) {}

Scheduler::~Scheduler() {
    deinit();
}

void Scheduler::init() {
    for (size_t i = 0; i < thread_count_; ++i) {
        worker_threads_.emplace_back(&Scheduler::workerFunction, this);
    }
}

void Scheduler::deinit() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        stop_ = true;
    }
    task_available_condition_.notify_all();
    for (auto& thread: worker_threads_) {
        if(thread.joinable()) {
            thread.join();
        }
    }

    worker_threads_.clear();
}

void Scheduler::workerFunction() {
    while (true) {
        std::shared_ptr<Task> task;

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            task_available_condition_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
            if (stop_ && tasks_.empty()) {
                return;
            }
            task = tasks_.front();
            tasks_.pop();
        }

        task->command->execute(task->requester);
    }
}

void Scheduler::enqueueTask(const std::shared_ptr<CommandInterface>& command) {
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        auto task = std::make_shared<Task>(nullptr, command);
        tasks_.push(task);
    }
    task_available_condition_.notify_one();
}

void Scheduler::enqueueTask(std::shared_ptr<InputInterface::Requester> requester, const std::shared_ptr<CommandInterface>& command) {
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        auto task = std::make_shared<Task>(std::move(requester), command);
        tasks_.push(task);
    }
    task_available_condition_.notify_one();
}

size_t Scheduler::getRunningThreadCount() const {
    return worker_threads_.size();
}

