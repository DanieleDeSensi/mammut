/*
 * module.hpp
 *
 * Created on: 12/11/2014
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

#ifndef MODULE_HPP_
#define MODULE_HPP_

#include <mammut/communicator.hpp>
#include <mammut/utils.hpp>

#include <string>

#define MAMMUT_MODULE_DECL(ModuleType)  friend class ::mammut::Servant;                                                        \
                                      private:                                                                                \
                                          static std::string getModuleName();                                                 \
                                          bool processMessage(const std::string& messageIdIn, const std::string& messageIn,   \
                                                              std::string& messageIdOut, std::string& messageOut);            \
                                      public:                                                                                 \
                                          static ModuleType* local();                                                         \
                                          static ModuleType* remote(mammut::Communicator* const communicator);                  \
                                          static void release(ModuleType* topology);                                          \

namespace mammut{

class Module;
#if 0
typedef Module* (*local)();
typedef Module* (*remote)(mammut::Communicator* const communicator);
typedef void (*release)(Module* topology);
typedef bool (*MessageProcessingFun)(const std::string&, const std::string&,
                                     std::string&, std::string&);
template <typename T>
typedef struct{
    T module;
}ModuleDescriptor;

#include "energy/energy.hpp"

static std::vector<ModuleDescriptor> buildModulesDescriptors(){
    std::vector<ModuleDescriptor> v;
    v.push_back(ModuleDescriptor(&mammut::energy::Energy::processMessage, mammut::energy::Energy::getModuleId()));
    return v;
}

static const std::vector<ModuleDescriptor> modulesDescriptors = buildModulesDescriptors();
#endif

class Mammut;
class Servant;

class Module: public utils::NonCopyable{
    friend class ::mammut::Servant;
public:
    virtual inline ~Module(){;}
private:
    virtual bool processMessage(const std::string& messageIdIn, const std::string& messageIn,
                                std::string& messageIdOut, std::string& messageOut) = 0;
};


}






#endif /* MODULE_HPP_ */
