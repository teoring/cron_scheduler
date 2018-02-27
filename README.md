## Description:

CronScheduler is a thread-safe scheduler designed to set and execute delayed or repeatable tasks at the specified time point. It is based on progschj`s implementation of ThreadPool which can be found using the following link: https://github.com/progschj/ThreadPool

CronScheduler is driven by the external clock which invokes onNewTime(const struct timeval&) function to notify scheduler about time shift which allows to use it in the nonreal-time environment like testing or imitation runs.

It supports task scheduling up to milliseconds (with reasonable threshold deviations due to clock accuracy and actual system load). Internal implementation works with Unix timestamps as it`s a simple, time-zone independent form of the time representation. Multithreading implemented using standard STL thread which makes it possible to compile the project on different platforms (may require minor modifications).

CronScheduler support task contexts. It gives flexibility in terms of the way how input data could be transferred to the task internal scope. Context pointer passed to the task on the task creation and should be valid during the period of the expected task execution. 
CronScheduler doesn't provide synchronization for context access.

## Basic usage:

1. Connect time source provider with scheduler:
```c++
    void onTimeEventCaller(std::function<void(const struct timeval&)> onNewTime)
    {
        struct timeval tval;
        while (!done)
        {
            // time resolution is flexible
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            gettimeofday (&tval, NULL);
            onNewTime(tval);
        }
    }

    ....
    // during configuration
    std::shared_ptr<cron::CronScheduler> scheduler (new cron::CronScheduler(WORKERS_AMOUNT));
    auto clockUpdater = [scheduler] (const struct timeval& time) 
            { scheduler->onNewTime(time); };
   
    auto clockThread = std::make_shared<std::thread>(onTimeEventCaller, clockUpdater);
    clockThread->join();
```
2. Set up a task:
```c++
    std::shared_ptr<cron::CronScheduler> scheduler;

    // create a task to be executed
    auto task = [] () {
        struct timeval tval;
        gettimeofday (&tval, NULL);
        std::cout << "Task executed at " << scheduler::getTimestamp(tval)
            <<" milliseconds."  << std::endl;
    };

    struct timeval tval;
    gettimeofday (&tval, NULL);

    std::cout << "Task scheduled at " << scheduler::getTimestamp(tval)
            <<" milliseconds."  << std::endl;

    // set execution time 10 seconds later
    tval.tv_sec += 10; 
    scheduler.scheduleAt(tval, task);
```


## Requirements to compile:

    -GNU 4.7 or 5.4 compiler
    -Boost unit test framework
    -gcov and lcov for coverage
    -CMake 2.8 or greater

To generate test coverage run:

    ./generate_coverage.sh

Report will be stored in ${PROJECT_DIR}/coverage folder

## To compile the project:
	cd build
	cmake ../
	make

Test executable will be stored in ${PROJECT_DIR}/bin



    
