#ifndef EventRegistration_hpp
#define EventRegistration_hpp

#include "HandlerRegistration.hpp"
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
     * @param sender The registered sender object
     */
    EventRegistration(void * const handler, std::shared_ptr<std::list<std::shared_ptr<EventRegistration>>> const registrations, ObjectPtr const sender ) :
    handler(handler),
    ptrRegistrations(registrations),
    ptrSender(sender),
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
     * \brief Gets the sender object for this registration
     *
     * @return The registered sender object
     */
    ObjectPtr getSender() {
        return ptrSender;
    }
    
    
    /**
     * \brief Removes an event handler from the registration collection
     *
     * The event handler will no longer receive events for this event type
     */
    virtual void removeHandler() {
        if (registered) {
            printf("has handlers count: %lu before remove handler.\n", ptrRegistrations->size());
            ptrRegistrations->remove(shared_from_this());
            printf("has handlers count: %lu after remove handler.\n", ptrRegistrations->size());
            registered = false;
        }
    }
    
private:
    void * const handler;
    std::shared_ptr<std::list<std::shared_ptr<EventRegistration>>> const ptrRegistrations;
    ObjectPtr ptrSender;
    
    bool registered;
};

typedef std::shared_ptr<EventRegistration> EventRegistrationPtr;

#endif /* EventRegistration_hpp */
