#include "Engine/EventSystem.hpp"

EventSystem::~EventSystem()
{
    const auto pred = [](const auto& subscription) {
        return subscription.second.empty();
    };

    Assert(std::ranges::all_of(subscriptions, pred));
}

void EventSystem::UnsubscribeAll(void* subscriber)
{
    std::ranges::for_each(subscriptions, [&](const auto& subscription) {
        UnsubscribeImpl(subscription.first, subscriber);
    });
}

void EventSystem::SubscribeImpl(std::type_index typeIndex, void* subscriber, 
    const std::function<void(std::any)> &handler, ES::Priority priority)
{
    if (const auto it = subscriptions.find(typeIndex); it == subscriptions.end())
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
}