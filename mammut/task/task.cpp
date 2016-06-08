#include "task.hpp"
#include "task-linux.hpp"

namespace mammut{
namespace task{

TasksManager* TasksManager::local(){
#if defined (__linux__)
    return new ProcessesManagerLinux();
#else
    throw new std::runtime_error("Process: OS not supported.");
#endif
}

TasksManager* TasksManager::remote(Communicator* const communicator){
    throw new std::runtime_error("Remote process manager not yet supported.");
}

void TasksManager::release(TasksManager* pm){
    if(pm){
        delete pm;
    }
}

}
}
