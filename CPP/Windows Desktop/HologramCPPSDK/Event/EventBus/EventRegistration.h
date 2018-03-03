#ifndef _EVENT_REGISTRATION_H_
#define _EVENT_REGISTRATION_H_

#include "HandlerRegistration.h"
#include <list>
#include <typeinfo>
#include <unordered_map>

#include <stdio.h>
/**
 * \brief Registration class private to EventBus for registered event handlers
 */
class EventRegistration : public HandlerRegistration, public std::enable_shared_from_this<EventRegistration>
{
public:
    
    /**
     * \brief Represents a registration object for a registered event handler
     *
     * This object is stored in a collection with other handlers for the event type.
     *
     * @param handler The event handler
     * @param registrations The handler collection for this event type
     */
    EventRegistration(void * const handler, std::shared_ptr<std::list<std::shared_ptr<EventRegistration>>> const registrations) :
    handler(handler),
    ptrRegistrations(registrations),
    registered(true)
    { }
    
    
    /**
     * \brief Empty virtual destructor
     */
    virtual ~EventRegistration() { }
    
    
    /**
     * \brief Gets the event handler for this registration
     *
     * @return The event handler
     */
    void * const getHandler() {
        return handler;
    }
    
    
    /**
     * \brief Removes an event handler from the registration collection
     *
     * The event handler will no longer receive events for this event type
     */
    virtual void removeHandler() {
        if (registered) {
            ptrRegistrations->remove(shared_from_this());
            registered = false;
        }
    }
    
private:
    void * const handler;
    std::shared_ptr<std::list<std::shared_ptr<EventRegistration>>> const ptrRegistrations;
    
    bool registered;
};

typedef std::shared_ptr<EventRegistration> EventRegistrationPtr;

#endif /* _EVENT_REGISTRATION_H_ */
