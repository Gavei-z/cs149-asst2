# 实验二 partB记录
``` cpp
TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks, const std::vector<TaskID>& deps)
```
此函数新建一个Task，然后加入任务队列里面，随即就返回任务id。（异步）

```cpp
void TaskSystemParallelThreadPoolSleeping::threadLoop(int id_)
```
此函数内部死循环，一直访问查询是否有任务已经准备好（这个任务赖的任务已完成）。若没有：查询已经阻塞的任务中有无准备好的任务，有则拿出来运行这个任务，同时更新维护的数据结构；若还是没有可执行的任务，则阻塞睡眠，记录睡眠线程的变量+1。有准备好的任务的话，就随机选取一个执行即可。

对于每一个任务，都有一定数量的子任务，线程池每次执行一个子任务，若一个大任务完成了，就更新存储任务的数据结构，把这个任务从任务列表中删去, 同时条件变量通知阻塞的地方(调用run的地方会阻塞等待任务的完成)。

```cpp
void TaskSystemParallelThreadPoolSleeping::deleteFinishedTask(Task* task)
```
功能：上述中，要把已完成的任务从任务列表中删去，并通知依赖此任务的其他任务，这个任务已完成了，也可以从这些任务的依赖列表中删除目前这个已完成的任务。
