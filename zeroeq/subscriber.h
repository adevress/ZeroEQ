
/* Copyright (c) 2014-2016, Human Brain Project
 *                          Daniel Nachbaur <daniel.nachbaur@epfl.ch>
 *                          Stefan.Eilemann@epfl.ch
 */

#ifndef ZEROEQ_SUBSCRIBER_H
#define ZEROEQ_SUBSCRIBER_H

#include <zeroeq/receiver.h> // base class

#include <vector>

namespace zeroeq
{
/**
 * Subscribes to Publisher to receive events.
 *
 * If the subscriber is in the same session as discovered publishers, it
 * automatically subscribes to those publishers. Publishers from the same
 * application instance are not considered though.
 *
 * A subscription to a non-existing publisher is valid. It will start receiving
 * events once the other publisher(s) is(are) publishing.
 *
 * A receive on any Subscriber of a shared group will work on all subscribers
 * and call the registered handlers.
 *
 * Not thread safe.
 *
 * Example: @include tests/subscriber.cpp
 */
class Subscriber : public Receiver
{
public:
    /**
     * Create a default subscriber.
     *
     * Postconditions:
     * - discovers publishers on _zeroeq_pub._tcp ZeroConf service
     * - filters session \<username\> or ZEROEQ_SESSION from environment
     *
     * @throw std::runtime_error if ZeroConf is not available
     */
    ZEROEQ_API Subscriber();

    /**
     * Create a subscriber which subscribes to publisher(s) from the given
     * session.
     *
     * Postconditions:
     * - discovers publishers on _zeroeq_pub._tcp ZeroConf service
     * - filters for given session
     *
     * @param session session name used for filtering of discovered publishers
     * @throw std::runtime_error if ZeroConf is not available
     */
    ZEROEQ_API explicit Subscriber( const std::string& session );

    /**
     * Create a subscriber which subscribes to a specific publisher.
     *
     * Postconditions:
     * - connected to the publisher on the given URI once publisher is running
     *   on the URI
     *
     * @param uri publisher URI in the format [scheme://]*|host|IP|IF:port
     * @throw std::runtime_error if URI is not fully qualified
     */
    ZEROEQ_API explicit Subscriber( const URI& uri );

    /**
     * Create a subscriber which subscribes to publisher(s) on the given URI.
     *
     * The discovery and filtering by session is only used if the URI is not
     * fully qualified.
     *
     * Postconditions:
     * - discovers publishers on _zeroeq_pub._tcp ZeroConf service if URI is not
     *   fully qualified
     * - filters session \<username\> or ZEROEQ_SESSION from environment if
     *   zeroeq::DEFAULT_SESSION
     *
     * @param uri publisher URI in the format [scheme://][*|host|IP|IF][:port]
     * @param session session name used for filtering of discovered publishers
     * @throw std::runtime_error if ZeroConf is not available or if session name
     *                           is invalid
     */
    ZEROEQ_API Subscriber( const URI& uri, const std::string& session );

    /**
     * Create a default shared subscriber.
     *
     * @sa Subscriber()
     * @param shared another receiver to share data reception with
     */
    ZEROEQ_API Subscriber( Receiver& shared );

    /**
     * Create a shared subscriber which subscribes to publisher(s) from the
     * given session.
     *
     * @sa Subscriber( const std::string& )
     *
     * @param session only subscribe to publishers of the same session
     * @param shared another receiver to share data reception with
     */
    ZEROEQ_API Subscriber( const std::string& session, Receiver& shared );

    /**
     * Create a shared subscriber which subscribes to publisher(s) on the given
     * URI.
     *
     * @sa Subscriber( const URI& )
     *
     * @param uri publisher URI in the format [scheme://]*|host|IP|IF:port
     * @param shared another receiver to share data reception with
     */
    ZEROEQ_API Subscriber( const URI& uri, Receiver& shared );

    /**
     * Create a subscriber which subscribes to publisher(s) on the given URI.
     *
     * @sa Subscriber( const URI&, const std::string& )
     *
     * @param uri publisher URI in the format [scheme://][*|host|IP|IF][:port]
     * @param session session name used for filtering of discovered publishers
     * @param shared another receiver to share data reception with.
     */
    ZEROEQ_API Subscriber( const URI& uri, const std::string& session,
                        Receiver& shared );

    /** Destroy this subscriber and withdraw any subscriptions. */
    ZEROEQ_API ~Subscriber();

    /**
     * Subscribe a serializable object to receive updates from any connected
     * publisher.
     *
     * Every update will be directly applied on the object during receive(). To
     * track updates on the object, the serializable's updated function is
     * called accordingly.
     *
     * The subscribed object instance has to be valid until unsubscribe().
     *
     * @param serializable the object to update on receive()
     * @return true if subscription was successful, false otherwise
     */
    ZEROEQ_API bool subscribe( servus::Serializable& serializable );

    /**
     * Subscribe to an event from any connected publisher.
     *
     * Every receival of the event will call the registered callback function.
     *
     * @param event the event identifier to subscribe to
     * @param func the callback function called upon receival
     * @return true if subscription was successful, false otherwise
     */
    ZEROEQ_API bool subscribe( const uint128_t& event, const EventFunc& func );

    /**
     * Subscribe to an event with payload from any connected publisher.
     *
     * Every receival of the event will call the registered callback function.
     *
     * @param event the event identifier to subscribe to
     * @param func the callback function called upon receival
     * @return true if subscription was successful, false otherwise
     */
    ZEROEQ_API bool subscribe( const uint128_t& event,
                               const EventPayloadFunc& func );

    /**
     * Unsubscribe a serializable object to stop applying updates from any
     * connected publisher.
     *
     * @param serializable the object to stop updating on receive()
     * @return true if removal of subscription was successful, false otherwise
     */
    ZEROEQ_API bool unsubscribe( const servus::Serializable& serializable );

    ZEROEQ_API bool unsubscribe( const uint128_t& event );

    /** @return the session name that is used for filtering. */
    ZEROEQ_API const std::string& getSession() const;

private:
    class Impl;
    std::unique_ptr< Impl > _impl;

    // Receiver API
    void addSockets( std::vector< detail::Socket >& entries ) final;
    void process( detail::Socket& socket, uint32_t timeout ) final;
    void update() final;
    void addConnection( const std::string& uri ) final;
};

}

#endif
