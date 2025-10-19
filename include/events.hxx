#pragma once

#include <cstddef>
#include <typeinfo>
#include <unordered_map>
#include <vector>

class Event {
  public:
    using IdType = std::size_t;
};

class Handler {
  public:
    virtual void operator()(const Event& event) = 0;
};

class EventBus {
  public:
    template<typename E>
    void register_handler(Handler* handler) {
        const auto event_id = typeid(E).hash_code();

        if (!handlers.contains(event_id)) {
            handlers[event_id] = std::vector<Handler*>();
        }

        handlers.at(event_id).push_back(handler);
    }

    template<typename E>
    void dispatch(const Event& event) {
        const auto event_id = typeid(E).hash_code();

        if (!handlers.contains(event_id)) {
            return;
        }

        for (const auto& handler : handlers[event_id]) {
            handler->operator()(event);
        }
    }

  private:
    std::unordered_map<Event::IdType, std::vector<Handler*>> handlers;
};
