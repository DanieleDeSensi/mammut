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

#include "./utils.hpp"
#include "algorithm"
#include "cctype"

#include "task/task.hpp"
#if defined (__linux__)
#include "dirent.h"
#endif
#include "errno.h"
#include "fcntl.h"
#include "fstream"
#include "functional"
#include "locale"
#include "stdexcept"
#include "sstream"
#include "stdio.h"
#include "string.h"
#include "syscall.h"
#include "unistd.h"
#include "sys/syscall.h"
#include "sys/stat.h"
#include "sys/time.h"
#include "sys/types.h"

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

Thread::Thread():_thread(0), _running(false), _threadHandler(NULL),
                 _pm(mammut::task::TasksManager::local()){
    ;
}

Thread::~Thread(){
    mammut::task::TasksManager::release(_pm);
}

void* Thread::threadDispatcher(void* arg){
    Thread* t = static_cast<Thread*>(arg);
    t->_threadHandler = t->_pm->getThreadHandler(getpid(), gettid());
    t->_pidSet.notifyAll();

    t->_running = true;
    t->run();
    t->_running = false;
    return NULL;
}

void Thread::start(){
    if(!_running){
        int rc = pthread_create(&_thread, NULL, threadDispatcher, this);
        if(rc != 0){
            throw std::runtime_error("Thread: pthread_create failed. "
                                     "Error code: " + utils::intToString(rc));
        }
        _pidSet.wait();
    }else{
        throw std::runtime_error("Thread: Multiple start. It must be joined "
                                 "before starting it again.");
    }
}

mammut::task::ThreadHandler* Thread::getThreadHandler() const{
    return _threadHandler;
}

bool Thread::running(){
    return _running;
}

void Thread::join(){
    int rc = pthread_join(_thread, NULL);
    if(rc != 0){
        throw std::runtime_error("Thread: join failed. Error code: " +
                                 utils::intToString(rc));
    }
    _pm->releaseThreadHandler(_threadHandler);
    _threadHandler = NULL;
}

Monitor::Monitor():_mutex(){
    int rc = pthread_cond_init(&_condition, NULL);
    if(rc != 0){
        throw std::runtime_error("Monitor: couldn't initialize condition. "
                                 "Error code: " + utils::intToString(rc));
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
        throw std::runtime_error("Monitor: error in pthread_cond_wait. Error code: " + utils::intToString(rc));
    }
}

bool Monitor::timedWait(int milliSeconds){
    struct timeval now;
    gettimeofday(&now, NULL);

    struct timespec delay;
    delay.tv_sec  = now.tv_sec + milliSeconds / MAMMUT_MILLISECS_IN_SEC;
    delay.tv_nsec = now.tv_usec*1000 + (milliSeconds % MAMMUT_MILLISECS_IN_SEC) * MAMMUT_NANOSECS_IN_MSEC;
    if(delay.tv_nsec >= MAMMUT_NANOSECS_IN_SEC){
        delay.tv_nsec -= MAMMUT_NANOSECS_IN_SEC;
        delay.tv_sec++;
    }
    int rc = 0;
    _mutex.lock();
    while(!_predicate && rc != ETIMEDOUT){
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

#ifdef MAMMUT_REMOTE
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
#endif

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

#if (__linux__)
int executeCommand(const std::string& command, bool waitResult){
    int status = system((command + " > /dev/null 2>&1" + (!waitResult?" &":"")).c_str());
    if(status == -1){
        throw std::runtime_error("Impossible to execute command " + command);
    }
    return WEXITSTATUS(status);
}
#else
int executeCommand(const std::string& command, bool waitResult){
    throw std::runtime_error("executeCommand not supported on this OS.");
}
#endif

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

uint stringToUint(const std::string& s){
    return atoll(s.c_str());
}

ulong stringToUlong(const std::string& s){
    return atoll(s.c_str());
}


double stringToDouble(const std::string& s){
    return atof(s.c_str());
}

std::string readFirstLineFromFile(const std::string& fileName){
    std::string r;
    std::ifstream file(fileName.c_str());
    if(file){
        getline(file, r);
        file.close();
    }else{
        throw std::runtime_error("Impossible to open file " + fileName);
    }
    return r;
}

std::vector<std::string> readFile(const std::string& fileName){
    std::vector<std::string> r;
    std::ifstream file(fileName.c_str());
    if(file){
        std::string curLine;
        while(getline(file, curLine)){
            r.push_back(curLine);
        }
        file.close();
    }else{
        throw std::runtime_error("Impossible to open file " + fileName);
    }
    return r;
}

void writeFile(const std::string& fileName, const std::vector<std::string>& lines){
    std::ofstream file;
    file.open(fileName.c_str());
    if(!file.is_open()){
        throw std::runtime_error("Impossible to open file: " + fileName);
    }
    for(size_t i = 0; i < lines.size(); i++){
        file << lines.at(i) << "\n";
    }
    file.close();
}

void writeFile(const std::string& fileName, const std::string& line){
    std::ofstream file;
    file.open(fileName.c_str());
    if(!file.is_open()){
        throw std::runtime_error("Impossible to open file: " + fileName);
    }
    file << line << "\n";
    file.close();
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

Msr::Msr(uint32_t id){
    std::string msrFileName = "/dev/cpu/" + intToString(id) + "/msr";
    _fd = open(msrFileName.c_str(), O_RDONLY);
}

Msr::~Msr(){
    if(available()){
        close(_fd);
    }
}

bool Msr::available() const{
    return _fd != -1;
}

bool Msr::read(uint32_t which, uint64_t& value) const{
    ssize_t r = pread(_fd, (void*) &value, sizeof(value), (off_t) which);
    if(r == 0){
        return false;
    }else if(r != sizeof(value)){
        throw std::runtime_error("Error while reading msr register: " + utils::errnoToStr());
    }
    return true;
}

bool Msr::readBits(uint32_t which, unsigned int highBit,
                      unsigned int lowBit, uint64_t& value) const{
    bool r = read(which, value);
    if(!r){
        return false;
    }

    int bits = highBit - lowBit + 1;
    if(bits < 64){
        /* Show only part of register */
        value >>= lowBit;
        value &= ((uint64_t)1 << bits) - 1;
    }

    /* Make sure we get sign correct */
    if (value & ((uint64_t)1 << (bits - 1))){
        value &= ~((uint64_t)1 << (bits - 1));
        value = -value;
    }
    return true;
}

pid_t gettid(){
#ifdef SYS_gettid
    return syscall(SYS_gettid);
#else
    throw std::runtime_error("gettid() not available.");
#endif
}

double getMillisecondsTime(){
    struct timespec spec;
    if(clock_gettime(CLOCK_MONOTONIC, &spec) == -1){
        throw std::runtime_error(std::string("clock_gettime failed: ") + std::string(strerror(errno)));
    }
    return spec.tv_sec * 1000.0 + spec.tv_nsec / 1.0e6;
}


}
}
