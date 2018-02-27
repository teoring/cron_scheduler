#include "CronScheduler.h"

#include <functional>

namespace cron
{

CronTask::CronTask(time_t planned, time_t current, Callback&& callback,
    bool repeat, unsigned id, const ContextCPtr& ctx) :
        repeat_(repeat),
        callback_(std::move(callback)),
        context_(ctx),
        identifier_(id),
        interval_(planned - current),
        planned_(planned)
{}
 
bool CronTask::expired(time_t current) const
{
    return planned_ <= current;
}

time_t CronTask::planned() const
{
    return planned_;
}

bool CronTask::repeatable() const
{
    return repeat_;
}

void CronTask::calculate_new_planned(time_t timestamp)
{
    planned_ = timestamp + interval_;
}

void CronTask::execute() const
{
    callback_(context_);
}

CronTask::CronIdentifier CronTask::get_id() const
{
    return identifier_;
}

CronScheduler::~CronScheduler()
{
    std::lock_guard<std::mutex> locker(lock_);
    finished_ = true;
    updated_ = true;
    condition_.notify_one();
}

CronScheduler::CronScheduler(unsigned threadsAmount) :
    finished_(false),
    updated_(false),
    lastTaskId_(0),
    pool_(threadsAmount)
{
    pool_.enqueue([this] {
        while(!finished_)
        {
            if (this->tasks_.empty())
            {
                std::this_thread::yield();
            }
            else
            {
                std::unique_lock<std::mutex> locker(lock_);
                updated_ = false;
                
                // to avoid race condition if task was removed in the meantime
                if (this->tasks_.empty())
                    continue;
                
                time_t planned = (*tasks_.begin())->planned();
                if (planned > currTimestampMs_)
                {    
                    this->condition_.wait_for(locker,
                        std::chrono::milliseconds(planned - currTimestampMs_),
                        [this, planned]
                            { return updated_ || currTimestampMs_ >= planned; }
                    );
                }

                proceedTasks();
            }
        }
    });
}

void CronScheduler::proceedTasks()
{
    std::vector<std::shared_ptr<CronTask>> tasksToRepeat;
    for (auto it = tasks_.begin(); it != tasks_.end();)
    {         
        if ((*it)->expired(currTimestampMs_))
        {
            pool_.enqueue([taskPtr = (*it)] () {
                taskPtr->execute();
            });

            if ((*it)->repeatable())
            {
                (*it)->calculate_new_planned(currTimestampMs_);
                tasksToRepeat.push_back(*it);
            }

            it = tasks_.erase(it);
        }
        else break;
    }
   
    for (auto&& taskPtr : tasksToRepeat)
        tasks_.insert(std::move(taskPtr));
}

void CronScheduler::onNewTime(const struct timeval& tval)
{
    {
        std::lock_guard<std::mutex> locker(lock_);
        currTimestampMs_ = getTimestampInMs(tval);
    }
    condition_.notify_one();
}

void CronScheduler::addTask(std::shared_ptr<CronTask>&& task)
{
    updated_ = true;
    condition_.notify_one();
    tasks_.insert(task);
}

void CronScheduler::cancelTask(CronTask::CronIdentifier key)
{
    std::lock_guard<std::mutex> locker(lock_);
    for (auto it = tasks_.begin(); it != tasks_.end();)
    {
        if ((*it)->get_id() == key)
        {
            tasks_.erase(it);
            updated_ = true;
            condition_.notify_one();
            break;
        }
    }
}

time_t CronScheduler::getTimestampInMs(const struct timeval& tval)
{
    return tval.tv_sec * 1000 + tval.tv_usec / 1000;
}

CronTask::CronIdentifier CronScheduler::scheduleAt(const struct timeval& plannedTval,
    Callback&& callback)
{
    return scheduleAt(plannedTval, std::move(callback), false);
}

CronTask::CronIdentifier CronScheduler::scheduleAt(const struct timeval& plannedTval,
    Callback&& callback, bool repeatable)
{
    return scheduleAt(plannedTval, std::move(callback), repeatable, nullptr);
}

CronTask::CronIdentifier CronScheduler::scheduleAt(const struct timeval& plannedTval, Callback&& callback,
    bool repeatable, const ContextCPtr& ctx)
{
    std::time_t planned = getTimestampInMs(plannedTval);
    std::lock_guard<std::mutex> locker(lock_);
    std::shared_ptr<CronTask> task = std::make_shared<CronTask>(
        planned, currTimestampMs_, std::move(callback), repeatable, lastTaskId_++, ctx);
    addTask(std::move(task));
    return lastTaskId_ - 1;
}

} // namespace cron