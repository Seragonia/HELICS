/*

Copyright (C) 2017, Battelle Memorial Institute
All rights reserved.

This software was co-developed by Pacific Northwest National Laboratory, operated by the Battelle Memorial Institute; the National Renewable Energy Laboratory, operated by the Alliance for Sustainable Energy, LLC; and the Lawrence Livermore National Laboratory, operated by Lawrence Livermore National Security, LLC.

*/
#ifndef MPI_BROKER_H_
#define MPI_BROKER_H_
#pragma once

#include "core/CoreBroker.h"

namespace helics
{

class MpiComms;

class MpiBroker :public CoreBroker
{
public:
	/** default constructor*/
	MpiBroker(bool isRoot_ = false) noexcept;
	MpiBroker(const std::string &broker_name);

	void InitializeFromArgs(int argc, char *argv[]) override;

	/**destructor*/
	virtual ~MpiBroker();
	virtual void transmit(int32_t route, const ActionMessage &command) override;

	virtual void addRoute(int route_id, const std::string &routeInfo) override;

	virtual std::string getAddress() const override;
private:
	virtual bool brokerConnect() override;
	virtual void brokerDisconnect() override;

private:
	std::atomic<bool> initialized_{ false };  //!< atomic protecting local initialization
	std::unique_ptr<MpiComms> comms;  //!< pointer to the comms object handling the actions connection
									  //std::unique_ptr<ZmqConnection> zmqConn;  //!< object containing the ZmqConnection Information for Pimpl 
};
}
#endif
