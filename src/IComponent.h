#ifndef ICOMPONENT_H_
#define ICOMPONENT_H_

namespace cron
{

class IComponent
{
public:
    using  ComponentId = std::string;

public:
    virtual void initialize() = 0;
    virtual void release() = 0;
};

} // namespace cron

#endif // ICOMPONENT_H_
