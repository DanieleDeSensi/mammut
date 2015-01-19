/*
 * energy-remote.hpp
 *
 * Created on: 03/12/2014
 *
 * =========================================================================
 *  Copyright (C) 2014-, Daniele De Sensi (d.desensi.software@gmail.com)
 *
 *  This file is part of mammut.
 *
 *  mammut is free software: you can redistribute it and/or
 *  modify it under the terms of the Lesser GNU General Public
 *  License as published by the Free Software Foundation, either
 *  version 3 of the License, or (at your option) any later version.

 *  mammut is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  Lesser GNU General Public License for more details.
 *
 *  You should have received a copy of the Lesser GNU General Public
 *  License along with mammut.
 *  If not, see <http://www.gnu.org/licenses/>.
 *
 * =========================================================================
 */

#ifndef MAMMUT_ENERGY_REMOTE_HPP_
#define MAMMUT_ENERGY_REMOTE_HPP_

#include <mammut/energy/energy.hpp>
#include <mammut/energy/energy-remote.pb.h>

namespace mammut{
namespace energy{

class CounterCpuRemote: public CounterCpu{
private:
    mammut::Communicator* const _communicator;
    Joules getJoules(CounterCpuType type);
public:
    CounterCpuRemote(mammut::Communicator* const communicator, topology::Cpu* cpu, bool hasGraphic, bool hasDram);
    void reset();
    Joules getJoules();
    Joules getJoulesCores();
    Joules getJoulesGraphic();
    Joules getJoulesDram();
};

}
}



#endif /* MAMMUT_ENERGY_REMOTE_HPP_ */
