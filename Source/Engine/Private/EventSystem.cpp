#include "Engine/EventSystem.hpp"

EventSystem::~EventSystem()
{
    Assert(subscriptions.empty());
}

void EventSystem::SubscribeImpl(std::type_index typeIndex, void* subscriber, 
    const std::function<void(std::any)> &handler, ES::Priority priority)
{
    const auto it = subscriptions.find(typeIndex);
    if (it == subscriptions.end())
    {
        subscriptions.emplace(typeIndex, std::vector<EventHandler>());
    }

    const auto insertPos = std::ranges::find_if(subscriptions[typeIndex], [priority](const EventHandler& e) {
        return static_cast<int>(std::get<2>(e)) < static_cast<int>(priority);
    });

    subscriptions[typeIndex].insert(insertPos, { subscriber, handler, priority });
}

void EventSystem::UnsubscribeImpl(std::type_index typeIndex, void* subscriber)
{
    const auto pred = [subscriber](const EventHandler& handler) {
        return std::get<0>(handler) == subscriber;
    };

    std::erase_if(subscriptions[typeIndex], pred);

    if (subscriptions[typeIndex].empty())
    {
        subscriptions.erase(typeIndex);
    }
}