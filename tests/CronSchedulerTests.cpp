#define BOOST_TEST_MODULE CronSchedulerTests

#include <boost/test/unit_test.hpp>

#include <iostream>

#include "CronSchedulerTestFixture.h"

using namespace tests;

const unsigned kWaitForWorkerMs = 5;
const unsigned loadTestDurationSec = 1;

std::atomic<unsigned> amountOfExecutedTasks;
std::atomic<unsigned> amountOfScheduledTasks;

using namespace cron;

void onTimeEventCaller(std::function<void(const struct timeval&)> onNewTime)
{
    auto starterAt = std::time(0);

    while (std::time(0) < starterAt + loadTestDurationSec + 1)
    {
        // update time evert 20 ms
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        onNewTime(CronSchedulerTestFixture::getCurrentTimeval());
    }
}

void taskCreator(std::shared_ptr<cron::CronScheduler> schedulerPtr)
{
    auto starterAt = std::time(0);
    while (std::time(0) < starterAt + loadTestDurationSec)
    {
        auto task = [](const ContextCPtr& ctx) { amountOfExecutedTasks++; };
        
        auto tval = CronSchedulerTestFixture::getCurrentTimeval();
        tval.tv_usec += CronSchedulerTestFixture::getRandNum(0, 50) * 1000;

        // set a task up to 50 milliseconds in the future
        schedulerPtr->scheduleAt(tval, task);
        amountOfScheduledTasks++ ;
        // sleep for up to 50 ms
        std::this_thread::sleep_for(std::chrono::milliseconds(CronSchedulerTestFixture::getRandNum(0, 50)));
    }    
}

BOOST_AUTO_TEST_CASE( GenericMultithreadTest )
{
    CronSchedulerTestFixture fixture;
    std::vector<std::shared_ptr< std::thread>> threads(16);

    auto timerTask = [scheduler = fixture.getScheduler()] (const struct timeval& time)
        { scheduler->onNewTime(time); };

    threads[0] = std::make_shared<std::thread>(onTimeEventCaller, timerTask);

    for(size_t i = 1; i < threads.size(); i++)
        threads[i] = std::make_shared<std::thread>(taskCreator, fixture.getScheduler());

    for(size_t i = 0; i < threads.size(); i++)
        threads[i]->join();

    BOOST_CHECK_EQUAL(amountOfScheduledTasks, amountOfExecutedTasks);
}

BOOST_AUTO_TEST_CASE( ShouldExecuteTaskAtPlannedTime )
{
    CronSchedulerTestFixture  fixture;
    bool taskFinished = false;

    auto task = [&taskFinished](const ContextCPtr& ctx) { 
         taskFinished = true;    
    };

    struct timeval tval;
    gettimeofday (&tval, NULL);
    fixture.getScheduler()->onNewTime(tval);

    tval.tv_sec += 10;
    fixture.getScheduler()->scheduleAt(tval, task);

    tval.tv_sec -= 1;
    fixture.getScheduler()->onNewTime(tval);
    // to ensure worker thread will finish the task
    std::this_thread::sleep_for(std::chrono::milliseconds(kWaitForWorkerMs));
    BOOST_CHECK(!taskFinished);

    tval.tv_sec++;
    fixture.getScheduler()->onNewTime(tval);
    // to ensure worker thread will finish the task
    std::this_thread::sleep_for(std::chrono::milliseconds(kWaitForWorkerMs));
    BOOST_CHECK(taskFinished);
 }
 
BOOST_AUTO_TEST_CASE( ShouldExecuteTaskWithContext )
{
    const int storedKey = 999;
    const std::string storedValue = "StoredValue";
    const std::string componentKey = "DataStorage";
    
    std::shared_ptr<DataStorage> storagePtr = std::make_shared<DataStorage>();
    storagePtr->set(storedKey, storedValue);
     
    std::shared_ptr<cron::Context> contextPtr = std::make_shared<cron::Context>();
    contextPtr->set(storagePtr, componentKey);
 
    CronSchedulerTestFixture  fixture;
    std::string result;
    auto task = [&result, &componentKey](const ContextCPtr& ctx) {
         // synchronization of the data storage is out of the scope of this test
        if (ctx)   
            result = ctx->get<DataStorage>(componentKey)->get(storedKey);
    };
    
    struct timeval tval;
    gettimeofday (&tval, NULL);
    fixture.getScheduler()->onNewTime(tval);

    tval.tv_sec += 10;
    fixture.getScheduler()->scheduleAt(tval, task, false, contextPtr);

    fixture.getScheduler()->onNewTime(tval);
    std::this_thread::sleep_for(std::chrono::milliseconds(kWaitForWorkerMs));
    BOOST_CHECK_EQUAL(storedValue, result);
}

BOOST_AUTO_TEST_CASE( ShouldRepeatTask )
{
    CronSchedulerTestFixture  fixture;
    
    unsigned executedTimes = 0;
    auto task = [&executedTimes](const ContextCPtr& ctx) { executedTimes++; };
        
    auto tval = fixture.getCurrentTimeval();
    fixture.getScheduler()->onNewTime(tval);

    tval.tv_sec++;
    fixture.getScheduler()->scheduleAt(tval, task, true);
    
    fixture.getScheduler()->onNewTime(tval);
    std::this_thread::sleep_for(std::chrono::milliseconds(kWaitForWorkerMs));
    BOOST_CHECK_EQUAL(executedTimes, 1);
    
    tval.tv_sec++;
    fixture.getScheduler()->onNewTime(tval);
    std::this_thread::sleep_for(std::chrono::milliseconds(kWaitForWorkerMs));
    BOOST_CHECK_EQUAL(executedTimes, 2);
}

BOOST_AUTO_TEST_CASE( ShouldRepeatIntervalTask )
{
    CronSchedulerTestFixture  fixture;
    unsigned executedTimes = 0;
    
    auto task = [&executedTimes](const ContextCPtr& ctx) { executedTimes++; };
        
    struct timeval tval = fixture.getCurrentTimeval();
    fixture.getScheduler()->onNewTime(tval);

    tval.tv_sec += 2;
    fixture.getScheduler()->repeatEvery(std::chrono::seconds(2), task);
    
    fixture.getScheduler()->onNewTime(tval);
    std::this_thread::sleep_for(std::chrono::milliseconds(kWaitForWorkerMs));
    BOOST_CHECK_EQUAL(executedTimes, 1);
    
    tval.tv_sec += 2;
    fixture.getScheduler()->onNewTime(tval);
    std::this_thread::sleep_for(std::chrono::milliseconds(kWaitForWorkerMs));
    BOOST_CHECK_EQUAL(executedTimes, 2);
    
    tval.tv_sec += 3;
    fixture.getScheduler()->onNewTime(tval);
    std::this_thread::sleep_for(std::chrono::milliseconds(kWaitForWorkerMs));
    BOOST_CHECK_EQUAL(executedTimes, 3);
}

BOOST_AUTO_TEST_CASE( ShouldNeverRunSecondTask )
{
    CronSchedulerTestFixture  fixture;
    unsigned taskIdentifier = 0;
    
    auto task1 = [&taskIdentifier](const ContextCPtr& ctx) { taskIdentifier = 1; };
    auto task2 = [&taskIdentifier](const ContextCPtr& ctx) { taskIdentifier = 2; };

    struct timeval tval = fixture.getCurrentTimeval();
    fixture.getScheduler()->onNewTime(tval);

    tval.tv_sec += 2;
    fixture. getScheduler()->scheduleAt(tval, task1, false);
    tval.tv_sec += 4;
    auto id2 = fixture.getScheduler()->scheduleAt(tval, task2, false);
    
    tval.tv_sec -= 4;
    fixture.getScheduler()->onNewTime(tval);
    std::this_thread::sleep_for(std::chrono::milliseconds(kWaitForWorkerMs));
    BOOST_CHECK_EQUAL(taskIdentifier, 1);
    
    fixture.getScheduler()->cancelTask(id2);

    tval.tv_sec += 20;
    fixture.getScheduler()->onNewTime(tval);
    std::this_thread::sleep_for(std::chrono::milliseconds(kWaitForWorkerMs));
    BOOST_CHECK_EQUAL(taskIdentifier, 1);
}

BOOST_AUTO_TEST_CASE( ShouldDeleteIntervalTask )
{
    CronSchedulerTestFixture  fixture;
    unsigned executedTimes = 0;

    auto task = [&executedTimes](const ContextCPtr& ctx) { executedTimes++; };

    struct timeval tval;
    gettimeofday (&tval, NULL);
    fixture.getScheduler()->onNewTime(tval);

    tval.tv_sec += 2;
    auto id = fixture.getScheduler()->repeatEvery(std::chrono::seconds(2), task);

    fixture.getScheduler()->onNewTime(tval);
    std::this_thread::sleep_for(std::chrono::milliseconds(kWaitForWorkerMs));
    BOOST_CHECK_EQUAL(executedTimes, 1);

    tval.tv_sec += 2;
    fixture.getScheduler()->onNewTime(tval);
    std::this_thread::sleep_for(std::chrono::milliseconds(kWaitForWorkerMs));
    BOOST_CHECK_EQUAL(executedTimes, 2);

    fixture.getScheduler()->cancelTask(id);

    tval.tv_sec += 10;
    fixture.getScheduler()->onNewTime(tval);
    std::this_thread::sleep_for(std::chrono::milliseconds(kWaitForWorkerMs));
    BOOST_CHECK_EQUAL(executedTimes, 2);
}
