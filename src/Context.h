#ifndef SECURITYMATTERS_CONTEXT_H_
#define SECURITYMATTERS_CONTEXT_H_

#include <memory>
#include <map>

#include "IComponent.h"

namespace cron
{

class IComponent;

class Context
{
public:
    using Key = IComponent::ComponentId;
    using ComponentPtr = std::shared_ptr<IComponent>;
    using Container = std::map<Key, ComponentPtr>;

public:
    template <class CType> void set(const std::shared_ptr<CType>& object, const Key& name)
    {
        container_.insert(std::make_pair(name, object));
    }

    template <class CType> std::shared_ptr<CType> get(const Key& name) const
    {
        auto pos = container_.find(name);
        return pos == container_.end() ?
            nullptr : std::static_pointer_cast<CType>(pos->second);
    }

private:
    Container container_;
};

typedef std::shared_ptr<Context> ContextPtr;
typedef std::shared_ptr<const Context> ContextCPtr;

} // namespace cron

#endif // SECURITYMATTERS_CONTEXT_H_
