#pragma once

#include "Engine/Events.hpp"

// TODO: Improve structuring in this class (it looks like shit, i guess, but don't care for now)
// method implementations are also shit
class EventSystem
{
public:
    using EventHandler = std::pair<void*, std::function<void(std::any)>>;

    EventSystem();
    ~EventSystem();

    template<class T, class S>
    void Subscribe(S* subscriber, void (S::* function)(const T&));

    template<class T, class S>
    void Subscribe(S* subscriber, void (S::* function)());

    template<class T>
    void Subscribe(void (*function)(const T&));

    template<class T>
    void Subscribe(void (*function)());

    // TODO: Do i need 3 of these?
    template<class T>
    void Unsubscribe(void* subscriber);

    template<class T>
    void Unsubscribe(void (*function)(const T&));

    template<class T>
    void Unsubscribe(void (*function)());

    template<class T>
    void Fire(const T& argument);

    template<class T>
    void Fire();

private:
    void SubscribeImpl(std::type_index typeIndex, void* subscriber, const std::function<void(std::any)>& handler);
    void UnsubscribeImpl(std::type_index typeIndex, void* subscriber);

    std::unordered_map<std::type_index, std::vector<EventHandler>> subscriptions;
};

template<class T, class S>
void EventSystem::Subscribe(S* subscriber, void (S::* function)(const T&))
{
    auto handler = [subscriber, function](std::any argument)
    {
        (subscriber->*function)(std::any_cast<T>(argument));
    };

    SubscribeImpl(typeid(T), subscriber, handler);
}

template<class T, class S>
void EventSystem::Subscribe(S* subscriber, void (S::* function)())
{
    auto handler = [subscriber, function](std::any argument)
    {
        (subscriber->*function)();
    };

    SubscribeImpl(typeid(T), subscriber, handler);
}

template<class T>
void EventSystem::Subscribe(void (*function)(const T&))
{
    auto handler = [function](std::any argument)
    {
        function(std::any_cast<T>(argument));
    };

    SubscribeImpl(typeid(T), function, handler);
}

template<class T>
void EventSystem::Subscribe(void (*function)())
{
    auto handler = [function](std::any argument)
    {
        function();
    };

    SubscribeImpl(typeid(T), function(), handler);
}

template<class T>
void EventSystem::Unsubscribe(void* subscriber)
{
    UnsubscribeImpl(typeid(T), subscriber);
}

template<class T>
void EventSystem::Unsubscribe(void (*function)(const T&))
{
    UnsubscribeImpl(typeid(T), function);
}

template<class T>
void EventSystem::Unsubscribe(void (*function)())
{
   UnsubscribeImpl(typeid(T), function);
}

template<class T>
void EventSystem::Fire(const T& argument)
{
    for (const auto& subscription : subscriptions[typeid(T)])
    {
        subscription.second(argument);
    }
}

template<class T>
void EventSystem::Fire()
{
    for (const auto& subscription : subscriptions[typeid(T)])
    {
        subscription.second(std::any());
    }
}