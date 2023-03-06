#pragma once

#include <vector>
#include <functional>

template<typename T>
using EventListener = std::function<bool(T& evt)>;

class EventSystem;

template<typename T>
class Event
{
    friend class EventSystem;

private:
    static inline std::vector<EventListener<T>> s_listeners;

    static void RegisterListener(const EventListener<T>& listener)
    {
        s_listeners.push_back(listener);
    }

};


class EventSystem
{
public:
    template<class T_Event>
    static bool FireEvent(T_Event& evt)
    {
        for (auto& listener : T_Event::s_listeners)
        {
            if (listener(evt))
                return true;
        }

        return false;
    }

    template <class T_Event>
    static void Subscribe(const EventListener<T_Event>& listener)
    {
        T_Event::RegisterListener(listener);
    }
};


template<class T>
class EventButton : public Event<T>
{
public:
    char m_char = 0;
};

class EventMouseInput : public EventButton<EventMouseInput>
{

};

class EventKeyInput : public EventButton<EventKeyInput>
{

};


#include <cstdio>

int TestEvent()
{
    EventListener<EventMouseInput> listener = [](EventMouseInput& evt) -> bool
    {
        printf("Input event: %d", evt.m_char);
        return false;
    };

    EventSystem::Subscribe(listener);

    EventKeyInput evt;
    evt.m_char = 69;

    EventSystem::FireEvent(evt);

    return 0;
}

