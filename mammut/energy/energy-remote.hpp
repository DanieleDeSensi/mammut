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

#include "../energy/energy.hpp"
#ifdef MAMMUT_REMOTE
#include "../energy/energy-remote.pb.h"
#endif

namespace mammut{
namespace energy{

class CounterPlugRemote: public CounterPlug{
private:
    mammut::Communicator* const _communicator;
public:
    CounterPlugRemote(mammut::Communicator* const communicator);
    bool init();
    Joules getJoules();
    void reset();
};

class CounterCpusRemote: public CounterCpus{
private:
    mammut::Communicator* const _communicator;
    bool _hasDram;
    bool _hasGraphic;
public:
    CounterCpusRemote(mammut::Communicator* const communicator);

    JoulesCpu getJoulesComponents();
    JoulesCpu getJoulesComponents(topology::CpuId cpuId);
    Joules getJoulesCpu();
    Joules getJoulesCpu(topology::CpuId cpuId);
    Joules getJoulesCores();
    Joules getJoulesCores(topology::CpuId cpuId);
    Joules getJoulesGraphic();
    Joules getJoulesGraphic(topology::CpuId cpuId);
    Joules getJoulesDram();
    Joules getJoulesDram(topology::CpuId cpuId);
    bool hasJoulesDram();
    bool hasJoulesGraphic();
    bool init();
    void reset();
};

}
}



#endif /* MAMMUT_ENERGY_REMOTE_HPP_ */
