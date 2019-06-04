#ifndef MAMMUT_MODULE_HPP_
#define MAMMUT_MODULE_HPP_

#include "./communicator.hpp"
#include "./utils.hpp"

#include "stdexcept"
#include "string"

#define MAMMUT_MODULE_DECL(ModuleType)  friend class ::mammut::Servant;                                                       \
                                      private:                                                                                \
                                          static std::string getModuleName();                                                 \
                                      public:                                                                                 \
                                          static ModuleType* getInstance(Communicator* const communicator = NULL){            \
                                              if(communicator){                                                               \
                                                  return remote(communicator);                                                \
                                              }else{                                                                          \
                                                  return local();                                                             \
                                              }                                                                               \
                                          }                                                                                   \
                                          static ModuleType* local();                                                         \
                                          static ModuleType* remote(mammut::Communicator* const communicator);                \
                                          static void release(ModuleType* module);                                            \

namespace mammut{

class Module;

class Mammut;
class Servant;

class Module: public utils::NonCopyable{
    friend class ::mammut::Servant;
public:
    virtual inline ~Module(){;}
private:
#ifdef MAMMUT_REMOTE
    virtual bool processMessage(const std::string& messageIdIn, const std::string& messageIn,
                                std::string& messageIdOut, std::string& messageOut){
        throw std::runtime_error("Remote module not implemented.");
    }
#endif
};

}

#endif /* MAMMUT_MODULE_HPP_ */
