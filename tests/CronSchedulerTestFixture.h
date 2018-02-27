#ifndef CRONSCHEDULERTESTFIXTURE_H_
#define CRONSCHEDULERTESTFIXTURE_H_

#include <random>

#include <CronScheduler.h>

namespace tests
{

class CronSchedulerTestFixture
{
public:
    CronSchedulerTestFixture() : CronSchedulerTestFixture(4) 
    {}
    
    CronSchedulerTestFixture(unsigned threadsAmount) : 
        schedulerPtr(new cron::CronScheduler(threadsAmount))
    {
        // initial tick
        struct timeval tval = getCurrentTimeval();
        gettimeofday (&tval, NULL);
        schedulerPtr->onNewTime(tval);
    }
 
    std::shared_ptr<cron:: CronScheduler> getScheduler()
    {
        return schedulerPtr;
    }
    
    static unsigned getRandNum(unsigned from, unsigned to)
    {
        std::mt19937 rng;
        rng.seed(std::random_device()());
        std::uniform_int_distribution<std::mt19937::result_type> dist(1, 300);
        return dist(rng);
    }
    
    static struct timeval getCurrentTimeval()
    {
        struct timeval tval;
        gettimeofday (&tval, NULL);
        return tval;
    }
   
private:
   std::shared_ptr<cron:: CronScheduler> schedulerPtr;
};

class DataStorage : public cron::IComponent
{
public:
    void initialize() override {}
    void release() override {}
    
    std::string get(unsigned key) const
    {
        auto search = storage_.find(key);
        if (search != storage_.end())
            return search->second;
        return "";
    }
    
    std::string set(unsigned key, const std::string& value)
    {
        return storage_[key]  = value;
    }
    
private:
    std::map<int, std::string> storage_;
};

} // namespace tests

#endif // CRONSCHEDULERTESTFIXTURE_H_