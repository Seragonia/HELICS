/*

Copyright (C) 2017-2018, Battelle Memorial Institute
All rights reserved.

This software was co-developed by Pacific Northwest National Laboratory, operated by the Battelle Memorial
Institute; the National Renewable Energy Laboratory, operated by the Alliance for Sustainable Energy, LLC; and the
Lawrence Livermore National Laboratory, operated by Lawrence Livermore National Security, LLC.

*/
#include "MessageFederateManager.hpp"
#include "../core/Core.hpp"
namespace helics
{
MessageFederateManager::MessageFederateManager (Core *coreOb, Core::federate_id_t id)
    : coreObject (coreOb), fedID (id)
{
}
MessageFederateManager::~MessageFederateManager () = default;

void MessageFederateManager::disconnect ()
{
    // checks for the calls are handled in the MessageFederate itself
    coreObject = nullptr;
}
endpoint_id_t MessageFederateManager::registerEndpoint (const std::string &name, const std::string &type)
{
	auto handle=coreObject->registerEndpoint(fedID, name, type);
    std::lock_guard<std::mutex> eLock (endpointLock);
    endpoint_id_t id = static_cast<identifier_type> (local_endpoints.size ());
    local_endpoints.insert(name,handle,name, type,id,handle);

    return id;
}

void MessageFederateManager::registerKnownCommunicationPath (endpoint_id_t localEndpoint,
                                                             const std::string &remoteEndpoint)
{
    if (localEndpoint.value () < local_endpoints.size ())
    {
        coreObject->registerFrequentCommunicationsPair (local_endpoints[localEndpoint.value ()].name,
                                                        remoteEndpoint);
    }
}

void MessageFederateManager::subscribe (endpoint_id_t endpoint, const std::string &name, const std::string &type)
{
    if (endpoint.value () < local_endpoints.size ())
    {
        auto h = coreObject->registerSubscription (fedID, name, type, "", handle_check_mode::optional);
        std::lock_guard<std::mutex> eLock (endpointLock);
        subHandleLookup.emplace (h, std::make_pair (endpoint, name));
        hasSubscriptions = true;
    }
    else
    {
        throw (std::invalid_argument ("endpoint id is invalid"));
    }
}

bool MessageFederateManager::hasMessage () const
{
    for (auto &mq : messageQueues)
    {
        if (!mq.empty ())
        {
            return true;
        }
    }
    return false;
}

bool MessageFederateManager::hasMessage (endpoint_id_t id) const
{
    return (id.value () < local_endpoints.size ()) ? (!messageQueues[id.value ()].empty ()) : false;
}

/**
 * Returns the number of pending receives for the specified destination endpoint.
 */
uint64_t MessageFederateManager::receiveCount (endpoint_id_t id) const
{
    return (id.value () < local_endpoints.size ()) ? (messageQueues[id.value ()].size ()) : 0;
}
/**
* Returns the number of pending receives for the specified destination endpoint.
@details this function is not preferred in multi-threaded contexts due to the required locking
prefer to just use getMessage until it returns an invalid Message.
*/
uint64_t MessageFederateManager::receiveCount () const
{
    uint64_t sz = 0;
    for (auto &mq : messageQueues)
    {
        sz += mq.size ();
    }
    return sz;
}

std::unique_ptr<Message> MessageFederateManager::getMessage (endpoint_id_t endpoint)
{
    if (endpoint.value () < local_endpoints.size ())
    {
        auto mv = messageQueues[endpoint.value ()].pop ();
        if (mv)
        {
            return std::move (*mv);
        }
    }
    return nullptr;
}

std::unique_ptr<Message> MessageFederateManager::getMessage ()
{
    // just start with the first endpoint and check until a queue isn't empty
    for (auto &mq : messageQueues)
    {
        if (!mq.empty ())
        {
            auto ms = mq.pop ();
            if (ms)
            {
                return std::move (*ms);
            }
        }
    }
    return nullptr;
}

void MessageFederateManager::sendMessage (endpoint_id_t source, const std::string &dest, data_view message)
{
    if (source.value () < local_endpoints.size ())
    {
        coreObject->send (local_endpoints[source.value ()].handle, dest, message.data (), message.size ());
    }
    else
    {
        throw (std::invalid_argument ("endpoint id is invalid"));
    }
}

void MessageFederateManager::sendMessage (endpoint_id_t source,
                                          const std::string &dest,
                                          data_view message,
                                          Time sendTime)
{
    if (source.value () < local_endpoints.size ())
    {
        coreObject->sendEvent (sendTime, local_endpoints[source.value ()].handle, dest, message.data (),
                               message.size ());
    }
    else
    {
        throw (std::invalid_argument ("endpoint id is invalid"));
    }
}

void MessageFederateManager::sendMessage (endpoint_id_t source, std::unique_ptr<Message> message)
{
    if (source.value () < local_endpoints.size ())
    {
        coreObject->sendMessage (local_endpoints[source.value ()].handle, std::move (message));
    }
    else
    {
        throw (std::invalid_argument ("endpoint id is invalid"));
    }
}

void MessageFederateManager::updateTime (Time newTime, Time /*oldTime*/)
{
    CurrentTime = newTime;
    auto epCount = coreObject->receiveCountAny (fedID);
    // lock the data updates
    std::unique_lock<std::mutex> eplock (endpointLock);
    Core::handle_id_t endpoint_id;
    for (size_t ii = 0; ii < epCount; ++ii)
    {
        auto message = coreObject->receiveAny (fedID, endpoint_id);
        if (!message)
        {
            break;
        }

        /** find the id*/
        auto fid = local_endpoints.find (endpoint_id);
        if (fid != local_endpoints.end ())
        {  // assign the data

            auto localEndpointIndex = fid->id.value ();
            messageQueues[localEndpointIndex].emplace (std::move (message));
            if (local_endpoints[localEndpointIndex].callbackIndex >= 0)
            {
                auto cb = callbacks[local_endpoints[localEndpointIndex].callbackIndex];
                eplock.unlock ();
                cb (fid->id, CurrentTime);
                eplock.lock ();
            }
            else if (allCallbackIndex >= 0)
            {
                auto ac = callbacks[allCallbackIndex];
                eplock.unlock ();
                ac (fid->id, CurrentTime);
                eplock.lock ();
            }
        }
    }
    if (hasSubscriptions)
    {
        auto handles = coreObject->getValueUpdates (fedID);
        for (auto handle : handles)
        {
            auto sfnd = subHandleLookup.find (handle);
            if (sfnd != subHandleLookup.end ())
            {
                auto mv = std::make_unique<Message> ();
                mv->source = sfnd->second.second;
                auto localEndpointIndex = sfnd->second.first.value ();
                mv->dest = local_endpoints[localEndpointIndex].name;
                mv->original_source = mv->source;
                // get the data value
                auto data = coreObject->getValue (handle);

                mv->data = *data;
                mv->time = CurrentTime;
                messageQueues[localEndpointIndex].push (std::move (mv));
                if (local_endpoints[localEndpointIndex].callbackIndex >= 0)
                {
                    // make sure the lock is not engaged for the callback
                    auto cb = callbacks[local_endpoints[localEndpointIndex].callbackIndex];
                    eplock.unlock ();
                    cb (sfnd->second.first, newTime);
                    eplock.lock ();
                }
                else if (allCallbackIndex >= 0)
                {
                    // make sure the lock is not engaged for the callback
                    auto ac = callbacks[allCallbackIndex];
                    eplock.unlock ();
                    ac (sfnd->second.first, CurrentTime);
                    eplock.lock ();
                }
            }
        }
    }
}

void MessageFederateManager::startupToInitializeStateTransition ()
{
    messageQueues.resize (local_endpoints.size ());
}

void MessageFederateManager::initializeToExecuteStateTransition () {}

static const std::string nullStr;

std::string MessageFederateManager::getEndpointName (endpoint_id_t id) const
{
    std::lock_guard<std::mutex> eLock (endpointLock);
    return (id.value () < local_endpoints.size ()) ? local_endpoints[id.value ()].name : nullStr;
}

endpoint_id_t MessageFederateManager::getEndpointId (const std::string &name) const
{
    std::lock_guard<std::mutex> eLock (endpointLock);
    auto sub = local_endpoints.find (name);
    return (sub != local_endpoints.end ()) ? sub->id : 0;
}

std::string MessageFederateManager::getEndpointType (endpoint_id_t id) const
{
    std::lock_guard<std::mutex> eLock (endpointLock);
    return (id.value () < local_endpoints.size ()) ? local_endpoints[id.value ()].type : nullStr;
}

int MessageFederateManager::getEndpointCount () const
{
    std::lock_guard<std::mutex> eLock (endpointLock);
    return static_cast<int> (local_endpoints.size ());
}

void MessageFederateManager::registerCallback (std::function<void(endpoint_id_t, Time)> callback)
{
    std::lock_guard<std::mutex> eLock (endpointLock);
    if (allCallbackIndex < 0)
    {
        allCallbackIndex = static_cast<int> (callbacks.size ());
        callbacks.emplace_back (std::move (callback));
    }
    else
    {
        callbacks[allCallbackIndex] = std::move (callback);
    }
}

void MessageFederateManager::registerCallback (endpoint_id_t id, std::function<void(endpoint_id_t, Time)> callback)
{
    if (id.value () < local_endpoints.size ())
    {
        std::lock_guard<std::mutex> eLock (endpointLock);
        local_endpoints[id.value ()].callbackIndex = static_cast<int> (callbacks.size ());
        callbacks.emplace_back (std::move (callback));
    }
    else
    {
        throw (std::invalid_argument ("endpoint id is invalid"));
    }
}

void MessageFederateManager::registerCallback (const std::vector<endpoint_id_t> &ids,
                                               std::function<void(endpoint_id_t, Time)> callback)
{
    std::lock_guard<std::mutex> eLock (endpointLock);

    int ind = static_cast<int> (callbacks.size ());
    callbacks.emplace_back (std::move (callback));
    for (auto id : ids)
    {
        if (id.value () < local_endpoints.size ())
        {
            local_endpoints[id.value ()].callbackIndex = ind;
        }
    }
}

void MessageFederateManager::removeOrderedMessage (unsigned int index)
{
    auto handle = messageOrder.lock ();
    if (index == handle->back ())
    {
        handle->pop_back ();
    }
    else
    {
        auto term = handle->rend ();
        for (auto ri = handle->rbegin () + 1; ri != term; ++ri)
        {
            if (*ri == index)
            {
                handle->erase (ri.base ());
                break;
            }
        }
    }
}
}  // namespace helics