/*
Copyright (C) 2017, Battelle Memorial Institute
All rights reserved.

This software was co-developed by Pacific Northwest National Laboratory, operated by the Battelle Memorial
Institute; the National Renewable Energy Laboratory, operated by the Alliance for Sustainable Energy, LLC; and the
Lawrence Livermore National Laboratory, operated by Lawrence Livermore National Security, LLC.

*/
#ifndef _HELICS_FILTER_H_
#define _HELICS_FILTER_H_
#pragma once

#include "MessageFilterFederate.h"

namespace helics
{
/** class wrapping a source filter*/
class SourceFilter
{
  private:
    MessageFilterFederate *fed;  //!< the MessageFederate to interact with
    filter_id_t id;  //!< the id as generated by the Federate
  public:
    /**constructor to build an source filter object
    @param[in] mFed  the MessageFederate to use
    @param[in] target the endpoint the filter is targeting
    @param[in] name the name of the filter
    @param[in] input_type the type of data the filter is expecting
    @param[in] output_type the type of data the filter is generating
    */
    SourceFilter (MessageFilterFederate *mFed,
                  const std::string &target,
                  const std::string &name = "",
                  const std::string &input_type = "",
                  const std::string &output_type = "")
        : fed (mFed)
    {
        id = fed->registerSourceFilter (name, target, input_type, output_type);
    }

    virtual ~SourceFilter () = default;

    auto getMessage () const { return fed->getMessageToFilter (id); }
    /** check if there is a message available*/
    bool hasMessage () const { return fed->hasMessageToFilter (id); }

    /** register a callback for an update notification
    @details the callback is called in the just before the time request function returns
    @param[in] callback a function with signature void(endpoint_id_t, Time)
    time is the time the value was updated  This callback is a notification callback and doesn't return the value
    */
    void setCallback (std::function<void(filter_id_t, Time)> callback)
    {
        fed->registerFilterCallback (id, callback);
    }
    /** set a message operator to process the message*/
    void setOperator (std::shared_ptr<MessageOperator> mo) { fed->registerMessageOperator (id, std::move (mo)); }
};

/** class wrapping a destination filter*/
class DestinationFilter
{
  private:
    MessageFilterFederate *fed;  //!< the MessageFederate to interact with
    filter_id_t id;  //!< the id as generated by the Federate

  public:
    /**constructor to build an destination filter object
    @param[in] mFed  the MessageFederate to use
    @param[in] target the endpoint the filter is targeting
    @param[in] name the name of the filter
    @param[in] input_type the type of data the filter is expecting
    @param[in] output_type the type of data the filter is generating
    */
    DestinationFilter (MessageFilterFederate *mFed,
                       const std::string &target,
                       const std::string &name = "",
                       const std::string &input_type = "",
                       const std::string &output_type = "")
        : fed (mFed)
    {
        id = fed->registerDestinationFilter (name, target, input_type, output_type);
    }
    virtual ~DestinationFilter () = default;

    /** set a message operator to process the message*/
    void setOperator (std::shared_ptr<MessageOperator> mo) { fed->registerMessageOperator (id, std::move (mo)); }
};

enum defined_filter_types
{
    custom = 0,
    delay = 1,
    randomDelay = 2,
    randomDrop = 3,
};

std::unique_ptr<DestinationFilter> make_destination_filter (defined_filter_types type,
                                                            MessageFilterFederate *mFed,
                                                            const std::string &target,
                                                            const std::string &name = "");
std::unique_ptr<DestinationFilter> make_Source_filter (defined_filter_types type,
                                                       MessageFilterFederate *mFed,
                                                       const std::string &target,
                                                       const std::string &name = "");

}  // namespace helics
#endif
