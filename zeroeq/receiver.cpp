
/* Copyright (c) 2014, Human Brain Project
 *                     Daniel Nachbaur <daniel.nachbaur@epfl.ch>
 *                     Stefan.Eilemann@epfl.ch
 */

#define NOMINMAX // otherwise std::min/max below don't work on VS

#include "receiver.h"
#include "log.h"
#include "detail/socket.h"

#include <algorithm>
#include <chrono>
#include <deque>
#include <stdexcept>

namespace zeroeq
{
namespace detail
{
class Receiver
{
public:
    Receiver()
        : _context( zmq_ctx_new( ))
    {}

    ~Receiver()
    {
        zmq_ctx_destroy( _context );
    }

    void add( ::zeroeq::Receiver* receiver )
    {
        _shared.push_back( receiver );
    }

    void remove( ::zeroeq::Receiver* receiver )
    {
        _shared.erase( std::remove( _shared.begin(), _shared.end(), receiver ),
                       _shared.end( ));
    }

    bool receive( const uint32_t timeout )
    {
        if( timeout == TIMEOUT_INDEFINITE )
            return _blockingReceive();

        // Never fully block. Give receivers a chance to update, e.g., to check
        // for new connections from zeroconf (#20)
        const uint32_t block = std::min( 1000u, timeout / 10 );

        const auto startTime = std::chrono::high_resolution_clock::now();
        while( true )
        {
            for( ::zeroeq::Receiver* receiver : _shared )
                receiver->update();

            const auto endTime = std::chrono::high_resolution_clock::now();
            const uint32_t elapsed =
                std::chrono::nanoseconds( endTime - startTime ).count() /
                1000000;
            uint32_t wait = 0;
            if( elapsed < timeout )
                wait = std::min( timeout - uint32_t( elapsed ), block );

            if( _receive( wait ))
                return true;

            if( elapsed >= timeout )
                return false;
        }
    }

    void* getZMQContext() { return _context; }

private:
    void* _context;
    typedef std::vector< ::zeroeq::Receiver* > Receivers;
    typedef Receivers::iterator ReceiversIter;

    Receivers _shared;

    bool _blockingReceive()
    {
        while( true )
        {
            for( ::zeroeq::Receiver* receiver : _shared )
                receiver->update();

            // Never fully block. Give receivers a chance to update, e.g., to
            // check for new connections from zeroconf (#20)
            if( _receive( 1000 ))
                return true;
        }
    }

    bool _receive( uint32_t timeout )
    {
        // ZMQ notifications on its sockets is edge-triggered, hence we have
        // to receive all pending POLLIN events to not 'loose' notifications
        // from the socket descriptors (c.f. HTTP server).
        // For reference:
        // https://funcptr.net/2012/09/10/zeromq---edge-triggered-notification
        bool haveData = false;
        do
        {
            std::vector< Socket > sockets;
            std::deque< size_t > intervals;
            for( ::zeroeq::Receiver* receiver : _shared )
            {
                const size_t before = sockets.size();
                receiver->addSockets( sockets );
                intervals.push_back( sockets.size() - before );
            }

            switch( zmq_poll( sockets.data(), int( sockets.size( )), timeout ))
            {
            case -1: // error
                ZEROEQTHROW( std::runtime_error( std::string( "Poll error: " ) +
                                                 zmq_strerror( zmq_errno( ))));
            case 0: // timeout; no events signaled during poll
                return haveData;

            default:
            {
                // For each event, find the subscriber which supplied the socket
                // and inform it in case there is data on the socket. We saved
                // #sockets for each subscriber above and track them down here
                // as we iterate over all sockets:
                ReceiversIter i = _shared.begin();
                size_t interval = intervals.front();
                intervals.pop_front();

                // prepare for potential next poll; from now on continue
                // non-blocking to fullfil edge-triggered contract
                haveData = false;
                timeout = 0;

                for( Socket& socket : sockets )
                {
                    while( interval == 0 || interval-- == 0 )
                    {
                        ++i;
                        interval = intervals.front();
                        intervals.pop_front();
                    }

                    if( socket.revents & ZMQ_POLLIN )
                    {
                        (*i)->process( socket, timeout );
                        haveData = true;
                    }
                }
            }
            }
        }
        while( haveData );

        // if we didn't return earlier, we had at least one socket with data
        return true;
    }
};
}

Receiver::Receiver()
    : _impl( new detail::Receiver )
{
    _impl->add( this );
}

Receiver::Receiver( Receiver& shared )
    : _impl( shared._impl )
{
    _impl->add( this );
}

Receiver::~Receiver()
{
    _impl->remove( this );
}

bool Receiver::receive( const uint32_t timeout )
{
    return _impl->receive( timeout );
}

void* Receiver::getZMQContext()
{
    return _impl->getZMQContext();
}

}
