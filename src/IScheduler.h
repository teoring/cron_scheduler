#ifndef SECURITYMATTERS_SCHEDULERINTERFACE
#define SECURITYMATTERS_SCHEDULERINTERFACE

#include "Context.h"

namespace cron
{
    
class IScheduler
{
public:
    using CronIdentifier = unsigned;
    using Callback = std::function<void(const ContextCPtr& ctx)> ;

public:
    virtual void onNewTime(const struct  timeval& param) = 0;
    virtual CronIdentifier scheduleAt(const struct timeval& executeAt, Callback&& callback) = 0;
    virtual CronIdentifier scheduleAt(const struct timeval& executeAt, Callback&& callback, bool repeat) = 0;
    virtual CronIdentifier scheduleAt(const struct timeval& , Callback&& , bool repeatable, const ContextCPtr& ctx) = 0;;
    virtual  void cancelTask(CronIdentifier key)= 0;
};

} // namespace cron

#endif // SECURITYMATTERS_SCHEDULERINTERFACE