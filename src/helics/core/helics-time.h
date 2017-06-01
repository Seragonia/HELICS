/*

Copyright (C) 2017, Battelle Memorial Institute
All rights reserved.

This software was co-developed by Pacific Northwest National Laboratory, operated by the Battelle Memorial Institute; the National Renewable Energy Laboratory, operated by the Alliance for Sustainable Energy, LLC; and the Lawrence Livermore National Laboratory, operated by Lawrence Livermore National Security, LLC.

*/
#ifndef _HELICS_TIME_
#define _HELICS_TIME_

#include "core-types.h"
#include "helics/common/timeRepresentation.hpp"
#include <cstdint>

namespace helics
{
/**
 * Simulation virtual time.
 *
 * Class represents time in the core.
 */

using Time = timeRepresentation<count_time<9>>;


constexpr Time timeZero = Time::zeroVal();
constexpr Time timeEpsilon = Time::epsilon();

constexpr Time operator "" _t(long double val)
{
	return Time(val);
}

}
#endif
