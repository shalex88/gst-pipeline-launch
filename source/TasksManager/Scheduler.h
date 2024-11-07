#ifndef PERIPHERY_MANAGER_SCHEDULER_H
#define PERIPHERY_MANAGER_SCHEDULER_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <utility>
#include <vector>
#include <memory>
#include "TasksManager/CommandInterface.h"
#include "AppInputs/InputInterface.h"

class Scheduler {
public:
    explicit Scheduler(size_t thread_count = 1);
    ~Scheduler();
    void init();
    void deinit();
    void enqueueTask(const std::shared_ptr<CommandInterface>& command);
    void enqueueTask(std::shared_ptr<InputInterface::Requester> requester, const std::shared_ptr<CommandInterface>& command);
    size_t getRunningThreadCount() const;

private:
    struct Task {
        std::shared_ptr<InputInterface::Requester> requester;
        std::shared_ptr<CommandInterface> command;
        Task(std::shared_ptr<InputInterface::Requester> cmd_requester, std::shared_ptr<CommandInterface> cmd)
                : requester(std::move(cmd_requester)), command(std::move(cmd)) {}
    };
    std::queue<std::shared_ptr<Task>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable task_available_condition_;
    bool stop_ {false};
    std::vector<std::thread> worker_threads_;
    void workerFunction();
    size_t thread_count_{};
};

#endif //PERIPHERY_MANAGER_SCHEDULER_H
