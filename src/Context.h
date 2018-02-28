#ifndef CONTEXT_H_
#define CONTEXT_H_

#include <memory>
#include <map>

#include "IComponent.h"

namespace cron
{

class Context
{
public:
    using Key = IComponent::ComponentId;
    using ComponentPtr = std::shared_ptr<IComponent>;
    using Container = std::map<Key, ComponentPtr>;

public:
    template <class CType> void set(const std::shared_ptr<CType>& component, const Key& key)
    {
        container_.insert(std::make_pair(key, component));
    }

    template <class CType> std::shared_ptr<CType> get(const Key& key) const
    {
        auto it = container_.find(key);
        return it == container_.end() ? nullptr : std::static_pointer_cast<CType>(it->second);
    }

private:
    Container container_;
};

typedef std::shared_ptr<Context> ContextPtr;
typedef std::shared_ptr<const Context> ContextCPtr;

} // namespace cron

#endif // CONTEXT_H_
