

// Общий контракт сообщения в шине (Message Envelope)
#include <cstdint>
#include <string>
#include <optional>

template <typename DomainEventType>
struct MessageEnvelope {
    uint64_t timestamp;
    std::string raw_json_payload; // Исходный сырой JSON (для логгеров)
    
    // Ленивый или уже готовый доменный объект (для логики)
    // Используем std::optional, чтобы не парсить доменную модель, если она никому не нужна
    mutable std::optional<DomainEventType> cached_domain_event;

    const DomainEventType& getDomainEvent() const {
        if (!cached_domain_event.has_value()) {
            // cached_domain_event.emplace(parseFromJson(raw_json_payload));
        }
        return *cached_domain_event.value();
    }
};