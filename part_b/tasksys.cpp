#include "tasksys.h"


IRunnable::~IRunnable() {}

ITaskSystem::ITaskSystem(int num_threads) {}
ITaskSystem::~ITaskSystem() {}

/*
 * ================================================================
 * Serial task system implementation
 * ================================================================
 */

const char* TaskSystemSerial::name() {
    return "Serial";
}

TaskSystemSerial::TaskSystemSerial(int num_threads): ITaskSystem(num_threads) {
}

TaskSystemSerial::~TaskSystemSerial() {}

void TaskSystemSerial::run(IRunnable* runnable, int num_total_tasks) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemSerial::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                          const std::vector<TaskID>& deps) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemSerial::sync() {
    return;
}

/*
 * ================================================================
 * Parallel Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelSpawn::name() {
    return "Parallel + Always Spawn";
}

TaskSystemParallelSpawn::TaskSystemParallelSpawn(int num_threads): ITaskSystem(num_threads) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemParallelSpawn::sync() {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Spinning Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSpinning::name() {
    return "Parallel + Thread Pool + Spin";
}

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads): ITaskSystem(num_threads) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Sleeping Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSleeping::name() {
    return "Parallel + Thread Pool + Sleep";
}

TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int num_threads): ITaskSystem(num_threads),_num_threads{num_threads}{
    start(num_threads);
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
    terminate = true;

    while(true) {
        std::unique_lock<std::mutex> guard{queue_mutex};
        if (sleepThreadNum == _num_threads) break;
    }

    // Notify all the threads to return 
    producer.notify_all();

    for (int i = 0; i < _num_threads; ++ i) {
        threads[i].join();
    }

    // free the memory
    for (auto task: finished) {
        delete task.second;
    }
}

void TaskSystemParallelThreadPoolSleeping::start(int num_threads) {
    threads.resize(num_threads);
    for(int i = 0; i < num_threads; ++ i) {
        threads[i] = std::move(std:: thread(&TaskSystemParallelThreadPoolSleeping::threadLoop, this, i));
    }
}

void TaskSystemParallelThreadPoolSleeping::threadLoop(int id_) {
    while (1) {
        int index = -1;
        Task* task = nullptr;
        {
            std::unique_lock<std::mutex> guard{queue_mutex};
            if (ready.empty()) {
                // we should check to move the blocked to the ready
                if (!blocked.empty()) moveBlockTaskToReady();
            }

            if (ready.empty()) {
                // Ready is still empty, we should sleep the thread.
                sleepThreadNum ++;
                producer.wait(guard);
                sleepThreadNum --;
            }
        }
        /*
            we should tell whether the ready is empty
            but it should not be like this: rand() % 0, so we need handle
            this corner case.
        */
        if(!ready.empty()) {
            index = rand() % ready.size();
            // Use random to choose the task for each thread for simplicity.
            task = ready[index];
        }

        if (terminate) return;

        if (task == nullptr) continue;

        int processing = -1, finished = -1;
        {
            std::unique_lock<std::mutex> guard{task->task_mutex};
            processing = task->processing;
            // There are some situations `processing` will exceed
            // the total number, because we don't know when the
            // `deleteFinishedTask` is finished. We may choose the
            // task which is actually finished (or just only one)
            if (processing >= task->total_tasks) continue;
            task->processing ++;
        }

        if (processing < task->total_tasks) {
            task->runnable->runTask(processing, task->total_tasks);
            std::unique_lock<std::mutex> guard{task->task_mutex};
            task->finished++;
            finished = task->finished;
        }

        if (finished == task->total_tasks) {
            std::unique_lock<std::mutex> guard{queue_mutex};
            deleteFinishedTask(task);
            // When we signalSync, there are may be some threads which
            // are processing useless. So it may just return to the
            // destructor. So in the destructor we must wait for all
            // the thread going to sleep. And we call `notify_all` to
            // make all the threads stop. The design here should be
            // optimized. However, I don't have enough time...
            signalSync();
        }
    }
}

void TaskSystemParallelThreadPoolSleeping::deleteFinishedTask(Task* task) {
    size_t i = 0;
    for (; i < ready.size(); ++ i) {
        if (ready[i] == task) break;
    }

    finished.insert({ready[i]->id, ready[i]});
    ready.erase(ready.begin() + i);

    // 依赖这个task的所有task中，它的task依赖项数目减一。
    if (depencency.count(task->id)) {
        for (auto t: depencency[task->id])
            t->dependencies --;
    }
}

/*
    Move blocked task to the ready when the task's dependency is all finished.
*/
void TaskSystemParallelThreadPoolSleeping::moveBlockTaskToReady() {
    // 遍历所有阻塞task，查看是否可以存在不阻塞的task，以拿出来执行。
    std::vector<Task*> moved{};
    for (auto task: blocked) {
        if (task->dependencies == 0) {
            ready.push_back(task);
            moved.push_back(task);
        }
    }
    for (auto task: moved) blocked.erase(task);
}

/*
    All the tasks are finished, which means `ready` and `blocked`
    are empty, we could signal the ONLY ONE consumer.
*/
void TaskSystemParallelThreadPoolSleeping::signalSync() {
    // ready和block为空了
    if (ready.empty() && blocked.empty()) 
        consumer.notify_one();
}
     
/*
    It is easy for us to simulate the `run`. Just call the `runAsyncWithDeps` and
    use `sync` for synchronization. This is the most easy part in part B.
*/

void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {
    runAsyncWithDeps(runnable, num_total_tasks, {});
    sync();
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                    const std::vector<TaskID>& deps) {
    // 新建一个task，放进阻塞队列。
    Task* task = new Task(id, runnable, num_total_tasks, deps.size());
    {
        std::unique_lock<std::mutex> guard{queue_mutex};

        blocked.insert(task);

        // Record dependency information for later processing
        for (TaskID dep: deps) {
            if (depencency.count(dep)) {
                depencency[dep].insert(task);
            } else {
                depencency[dep] = std::unordered_set<Task*>{task};
            }
        }
        producer.notify_all();
    }
    return id ++;
}

/*
    This function is provided to the user for waiting for
    all tasks finished. We can just use a singel condition
    variable to achieve the functionality.
*/
void TaskSystemParallelThreadPoolSleeping::sync() {
    std::unique_lock<std::mutex> lock{queue_mutex};
    if(!ready.empty() || !blocked.empty()) {
        consumer.wait(lock);
    }
}
