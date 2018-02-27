#ifndef CRONJOBSCHEDULER_H_
#define CRONJOBSCHEDULER_H_

#include <sys/time.h>

#include <chrono> 
#include <condition_variable>
#include <memory>
#include <mutex>
#include <set>

#include "Context.h"
#include "IScheduler.h"
#include "ThreadPool/ThreadPool.h"

namespace cron
{
 
class CronTask
{
public:
    using Callback =  IScheduler::Callback;
    using CronIdentifier = IScheduler::CronIdentifier;

public:
    CronTask() = delete;
    explicit CronTask(time_t planned, time_t current, Callback&& callback,
        bool repeat, unsigned id, const ContextCPtr& context);

public:
    bool expired(time_t timestamp) const;
    bool repeatable() const;
    void execute() const;
    void calculate_new_planned(time_t timestamp);
    time_t planned() const;
    CronIdentifier get_id() const;
 
private:
    bool repeat_;
    Callback callback_;
    ContextCPtr context_;
    CronIdentifier identifier_;
    time_t interval_;
    time_t planned_;
};

struct taskComparator {
    bool operator() (const std::shared_ptr<CronTask> &lhs, const std::shared_ptr<CronTask> &rhs)
    {
        return lhs->planned() < rhs->planned();
    }
};

class CronScheduler : public IScheduler
{
public:
    using TaskContainer = std::multiset<std::shared_ptr<CronTask>, taskComparator>;

public:
    CronScheduler(unsigned threadsAmount);
    CronScheduler(const CronScheduler&) = delete;
    CronScheduler& operator= (const CronScheduler&) = delete;
    virtual ~CronScheduler();

public:
    static time_t getTimestampInMs(const struct timeval& param);

public:
    void onNewTime(const struct  timeval& param) override;
    void cancelTask(CronIdentifier key) override;

    CronIdentifier scheduleAt(const struct timeval& tval , Callback&& callback) override;
    CronIdentifier scheduleAt(const struct timeval& tval, Callback&& callback,
        bool repeatable) override;
    CronIdentifier scheduleAt(const struct timeval& tval, Callback&& callback,
        bool repeatable, const ContextCPtr& ctxCPtr) override;

    template<class Rep, class Period>
    CronIdentifier repeatEvery(const std::chrono::duration<Rep, Period>& interval, Callback&& callback)
    {
        return repeatEvery(interval, std::move(callback), nullptr);
    }
    
     template<class Rep, class Period>
    CronIdentifier repeatEvery(const std::chrono::duration<Rep, Period>& interval,
        Callback&& callback, const ContextCPtr& ctx)
    {
        time_t intervalMs = std::chrono::duration_cast<std::chrono::milliseconds>(interval).count();
        std::lock_guard<std::mutex> locker(lock_);
        std::shared_ptr<CronTask> task = std::make_shared<CronTask>(
            currTimestampMs_ + intervalMs, currTimestampMs_, std::move(callback), true, lastTaskId_++, ctx);
        addTask(std::move(task));
        return lastTaskId_ - 1;
    }

private:
    void addTask(std::shared_ptr<CronTask>&& task);
    void proceedTasks();

private:
    bool finished_;
    bool updated_;
    std::atomic<unsigned> lastTaskId_;
    std::condition_variable condition_;
    std::mutex lock_;
    TaskContainer tasks_;
    threadpool::ThreadPool pool_;
    time_t currTimestampMs_;
};

} // namespace cron

#endif // CRONJOBSCHEDULER_H_