#include "Engine/EventSystem.hpp"

EventSystem::EventSystem() = default;
EventSystem::~EventSystem() = default;

void EventSystem::SubscribeImpl(std::type_index typeIndex, void* subscriber, 
    const std::function<void(std::any)> &handler)
{
    const auto it = subscriptions.find(typeIndex);
    if (it == subscriptions.end())
    {
        subscriptions.emplace(typeIndex, std::vector<EventHandler>());
    }

    subscriptions[typeIndex].emplace_back(subscriber, handler);
}

void EventSystem::UnsubscribeImpl(std::type_index typeIndex, void* subscriber)
{
    const auto pred = [subscriber](const EventHandler& handler)
    {
        return handler.first == subscriber;
    };

    std::erase_if(subscriptions[typeIndex], pred);
}