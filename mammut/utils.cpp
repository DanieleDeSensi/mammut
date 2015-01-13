/*
 * utils.cpp
 *
 * Created on: 14/11/2014
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

#include <mammut/utils.hpp>

#include <algorithm>
#include <cctype>
#if defined (__linux__)
#include <dirent.h>
#endif
#include <errno.h>
#include <fstream>
#include <functional>
#include <locale>
#include <stdexcept>
#include <sstream>
#include <stdio.h>
#include <syscall.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>

namespace mammut{
namespace utils{

Lock::~Lock(){
    ;
}

LockPthreadMutex::LockPthreadMutex(){
    if(pthread_mutex_init(&_lock, NULL) != 0){
        throw std::runtime_error("LockPthreadMutex: Couldn't initialize mutex");
    }
}

LockPthreadMutex::~LockPthreadMutex(){
    if(pthread_mutex_destroy(&_lock) != 0){
        throw std::runtime_error("LockPthreadMutex: Couldn't destroy mutex");
    }
}

void LockPthreadMutex::lock(){
    if(pthread_mutex_lock(&_lock) != 0){
        throw std::runtime_error("Error while locking.");
    }
}

void LockPthreadMutex::unlock(){
    if(pthread_mutex_unlock(&_lock) != 0){
        throw std::runtime_error("Error while unlocking.");
    }
}

pthread_mutex_t* LockPthreadMutex::getLock(){
    return &_lock;
}

LockEmpty::LockEmpty(){
    ;
}

LockEmpty::~LockEmpty(){
    ;
}

void LockEmpty::lock(){
    ;
}

void LockEmpty::unlock(){
    ;
}

ScopedLock::ScopedLock(Lock& lock):_lock(lock){
    _lock.lock();
}

ScopedLock::~ScopedLock(){
    _lock.unlock();
}

Thread::Thread():_thread(NULL), _mutex(), _finished(false){
    ;
}

Thread::~Thread(){
    if(_thread){
        if(!_finished){
            cancel();
        }else{
            delete _thread;
            _thread = NULL;
        }
    }
}

void Thread::start(){
    if(_thread == NULL){
        _thread = new pthread_t;
        int rc = pthread_create(_thread, NULL, threadDispatcher, this);
        if(rc != 0){
            throw std::runtime_error("Thread: pthread_create failed.");
        }
    }else{
        throw std::runtime_error("Thread: Multiple start");
    }
}

bool Thread::finished(){
    bool f;
    _mutex.lock();
    f = _finished;
    _mutex.unlock();
    return f;
}

void Thread::join(){
    if(_thread){
        int rc = pthread_join(*_thread, NULL);
        if(rc != 0){
            throw std::runtime_error("Thread: join failed");
        }
        delete _thread;
        _thread = NULL;
    }
}

void Thread::cancel(){
    if(_thread){
        int rc = pthread_cancel(*_thread);
        if(rc){
            throw std::runtime_error("Thread: cancel failed.");
        }
        delete _thread;
        _thread = NULL;
    }
}

void* Thread::threadDispatcher(void* arg){
    Thread* t = static_cast<Thread*>(arg);
    t->run();
    t->setFinished();
    return NULL;
}

void Thread::setFinished(){
    _mutex.lock();
    _finished = true;
    _mutex.unlock();
}

Monitor::Monitor():_mutex(){
    if(pthread_cond_init(&_condition, NULL) != 0){
        throw std::runtime_error("Monitor: couldn't initialize condition");
    }
    _predicate = false;
}

bool Monitor::predicate(){
    bool r;
    _mutex.lock();
    r = _predicate;
    _mutex.unlock();
    return r;

}
void Monitor::wait(){
    int rc = 0;
    _mutex.lock();
    while(!_predicate){
        rc = pthread_cond_wait(&_condition, _mutex.getLock());
    }
    _predicate = false;
    _mutex.unlock();
    if(rc != 0){
        throw std::runtime_error("Monitor: error in pthread_cond_wait");
    }
}

#define NSEC_IN_MSEC 1000000
#define NSEC_IN_SEC 1000000000
#define MSEC_IN_SEC 1000

bool Monitor::timedWait(int milliSeconds){
    struct timeval now;
    gettimeofday(&now, NULL);
    struct timespec delay;
    delay.tv_sec  = now.tv_sec + milliSeconds / MSEC_IN_SEC;
    delay.tv_nsec = now.tv_usec*1000 + (milliSeconds % MSEC_IN_SEC) * NSEC_IN_MSEC;
    if(delay.tv_nsec >= NSEC_IN_SEC){
        delay.tv_nsec -= NSEC_IN_SEC;
        delay.tv_sec++;
    }
    int rc = 0;
    _mutex.lock();
    while(!_predicate){
        rc = pthread_cond_timedwait(&_condition, _mutex.getLock(), &delay);
    }
    _predicate = false;
    _mutex.unlock();
    if(rc == 0){
        return true;
    }else if(rc == ETIMEDOUT){
        return false;
    }else{
        throw std::runtime_error("Error: " + utils::intToString(rc) + " in pthread_cond_timedwait.");
    }
}

void Monitor::notifyOne(){
    _mutex.lock();
    _predicate = true;
    pthread_cond_signal(&_condition);
    _mutex.unlock();
}

void Monitor::notifyAll(){
    _mutex.lock();
    _predicate = true;
    pthread_cond_broadcast(&_condition);
    _mutex.unlock();
}

std::string getModuleNameFromMessageId(const std::string& messageId){
    std::vector<std::string> tokens = split(messageId, '.');
    if(tokens.size() != 3 || tokens.at(0).compare("mammut")){
        throw std::runtime_error("Wrong message id: " + messageId);
    }
    return tokens.at(1);
}

std::string getModuleNameFromMessage(::google::protobuf::MessageLite* const message){
    return getModuleNameFromMessageId(message->GetTypeName());
}

bool setMessageFromData(const ::google::protobuf::MessageLite* outData, std::string& messageIdOut, std::string& messageOut){
    messageIdOut = outData->GetTypeName();
    std::string tmp;
    if(!outData->SerializeToString(&tmp)){
        return false;
    }
    messageOut = tmp;
    return true;
}

#if defined (__linux__)
bool existsDirectory(const std::string& dirName){
    struct stat sb;
    return (stat(dirName.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode));
}
#endif

bool existsFile(const std::string& fileName){
    std::ifstream f(fileName.c_str());
    return f.good();
}

void executeCommand(const std::string& command){
    if(system(command.c_str()) == -1){
        throw std::runtime_error("Impossible to execute command " + command);
    }
}

std::vector<std::string> getCommandOutput(const std::string& command){
    std::vector<std::string> r;
    FILE *commandFile = popen(command.c_str(), "r");
    char *line = NULL;
    size_t len = 0;

    if(commandFile == NULL){
        throw std::runtime_error("Impossible to execute command " + command);
    }

    while(getline(&line, &len, commandFile) != -1){
        r.push_back(line);
    }
    free(line);
    pclose(commandFile);
    return r;
}

int stringToInt(const std::string& s){
    return atoi(s.c_str());
}

std::string readFirstLineFromFile(const std::string& fileName){
    std::string r;
    std::ifstream file(fileName.c_str());
    if(file.is_open()){
        getline(file, r);
        file.close();
    }else{
        throw std::runtime_error("Impossible to open file " + fileName);
    }
    return r;
}

void dashedRangeToIntegers(const std::string& dashedRange, int& rangeStart, int& rangeStop){
    size_t dashPos = dashedRange.find_first_of("-");
    rangeStart = stringToInt(dashedRange.substr(0, dashPos));
    rangeStop = stringToInt(dashedRange.substr(dashPos + 1));
}

std::string intToString(int x){
    std::string s;
    std::stringstream out;
    out << x;
    return out.str();
}

std::vector<std::string>& split(const std::string& s, char delim, std::vector<std::string>& elems){
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim)){
        elems.push_back(item);
    }
    return elems;
}


std::vector<std::string> split(const std::string& s, char delim){
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

std::string& ltrim(std::string& s){
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

std::string& rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

std::string& trim(std::string& s) {
        return ltrim(rtrim(s));
}

std::string errnoToStr(){
    return strerror(errno);
}

#if defined (__linux__)
std::vector<std::string> getFilesNamesInDir(const std::string& path, bool files, bool directories){
    std::vector<std::string> filesNames;
    DIR* dir;
    struct dirent* ent;
    if((dir = opendir(path.c_str())) != NULL){
        /* print all the files and directories within directory */
        while((ent = readdir(dir)) != NULL){
            struct stat st;
            lstat(ent->d_name, &st);
            if((S_ISDIR(st.st_mode) && directories) ||
               (!S_ISDIR(st.st_mode) && files)){
                if(std::string(".").compare(ent->d_name) && std::string("..").compare(ent->d_name)){
                    filesNames.push_back(ent->d_name);
                }
            }
        }
        closedir(dir);
    }else{
        throw std::runtime_error("getFilesList: " + errnoToStr());
    }
    return filesNames;
}
#endif

bool isNumber(const std::string& s){
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

uint getClockTicksPerSecond(){
    return sysconf(_SC_CLK_TCK);
}

}
}
