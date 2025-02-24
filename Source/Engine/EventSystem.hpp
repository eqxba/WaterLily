#pragma once

#include "Engine/Events.hpp"

namespace ES
{
    enum class Priority
    {
        eLow,
        eDefault,
        eHigh,
    };
}

// TODO: Improve structuring in this class (it looks like shit, i guess, but don't care for now)
// method implementations are also shit
class EventSystem
{
public:
    using EventHandler = std::tuple<void*, std::function<void(std::any)>, ES::Priority>;

    EventSystem() = default;
    ~EventSystem();

    EventSystem(const EventSystem&) = delete;
    EventSystem& operator=(const EventSystem&) = delete;

    EventSystem(EventSystem&&) = delete;
    EventSystem& operator=(EventSystem&&) = delete;

    template<class T, class S>
    void Subscribe(S* subscriber, void (S::* function)(const T&), ES::Priority priority = ES::Priority::eDefault);

    template<class T, class S>
    void Subscribe(S* subscriber, void (S::* function)(), ES::Priority priority = ES::Priority::eDefault);

    template<class T>
    void Subscribe(void (*function)(const T&), ES::Priority priority = ES::Priority::eDefault);

    template<class T>
    void Subscribe(void (*function)(), ES::Priority priority = ES::Priority::eDefault);

    template<class T>
    void Unsubscribe(void* subscriber);

    template<class T>
    void Unsubscribe(void (*function)(const T&));

    template<class T>
    void Unsubscribe(void (*function)());

    void UnsubscribeAll(void* subscriber);

    template<class T>
    void Fire(const T& argument);

    template<class T>
    void Fire();

private:
    void SubscribeImpl(std::type_index typeIndex, void* subscriber, const std::function<void(std::any)>& handler, 
        ES::Priority priority);
    void UnsubscribeImpl(std::type_index typeIndex, void* subscriber);

    std::unordered_map<std::type_index, std::vector<EventHandler>> subscriptions;
};

template<class T, class S>
void EventSystem::Subscribe(S* subscriber, void (S::* function)(const T&), 
    ES::Priority priority /* = ES::Priority::eDefault */)
{
    auto handler = [subscriber, function](std::any argument) {
        (subscriber->*function)(std::any_cast<T>(argument));
    };

    SubscribeImpl(typeid(T), subscriber, handler, priority);
}

template<class T, class S>
void EventSystem::Subscribe(S* subscriber, void (S::* function)(), ES::Priority priority /* = ES::Priority::eDefault */)
{
    auto handler = [subscriber, function](std::any argument) {
        (subscriber->*function)();
    };

    SubscribeImpl(typeid(T), subscriber, handler, priority);
}

template<class T>
void EventSystem::Subscribe(void (*function)(const T&), ES::Priority priority /* = ES::Priority::eDefault */)
{
    auto handler = [function](std::any argument) {
        function(std::any_cast<T>(argument));
    };

    SubscribeImpl(typeid(T), function, handler, priority);
}

template<class T>
void EventSystem::Subscribe(void (*function)(), ES::Priority priority /* = ES::Priority::eDefault */)
{
    auto handler = [function](std::any argument) {
        function();
    };

    SubscribeImpl(typeid(T), function(), handler, priority);
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
   UnsubscribeImpl(typeid(T), reinterpret_cast<void*>(function));
}

template<class T>
void EventSystem::Fire(const T& argument)
{
    if (!subscriptions.contains(typeid(T)))
    {
        return;
    }

    for (const auto& subscription : subscriptions[typeid(T)])
    {
        std::get<1>(subscription)(argument);
    }
}

template<class T>
void EventSystem::Fire()
{
    if (!subscriptions.contains(typeid(T)))
    {
        return;
    }

    T argument{};

    for (const auto& subscription : subscriptions[typeid(T)])
    {
        std::get<1>(subscription)(argument);
    }
}
