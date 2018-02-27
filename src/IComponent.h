#ifndef SECURITYMATTERS_ICOMPONENT_H
#define SECURITYMATTERS_ICOMPONENT_H

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

#endif // SECURITYMATTERS_ICOMPONENT_H
