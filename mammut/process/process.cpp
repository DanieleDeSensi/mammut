/*
 * process.cpp
 *
 * Created on: 09/01/2015
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

#include <mammut/process/process.hpp>
#include <mammut/process/process-linux.hpp>

namespace mammut{
namespace process{

ProcessesManager* ProcessesManager::local(){
#if defined (__linux__)
    return new ProcessesManagerLinux();
#else
    throw new std::runtime_error("Process: OS not supported.");
#endif
}

ProcessesManager* ProcessesManager::remote(Communicator* const communicator){
    throw new std::runtime_error("Remote process manager not yet supported.");
}

void ProcessesManager::release(ProcessesManager* pm){
    if(pm){
        delete pm;
    }
}

}
}
