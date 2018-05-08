#include "./utils.hpp"
#include "algorithm"
#include "cctype"

#include "task/task.hpp"
#if defined (__linux__)
#include "dirent.h"
#endif
#include <assert.h>
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
extern SimulationParameters simulationParameters;
namespace utils{

using namespace std;

Lock::~Lock(){
    ;
}

LockPthreadMutex::LockPthreadMutex(){
    if(pthread_mutex_init(&_lock, NULL) != 0){
        throw runtime_error("LockPthreadMutex: Couldn't initialize mutex");
    }
}

LockPthreadMutex::~LockPthreadMutex(){
    assert(pthread_mutex_destroy(&_lock) == 0);
}

void LockPthreadMutex::lock(){
    if(pthread_mutex_lock(&_lock) != 0){
        throw runtime_error("Error while locking.");
    }
}

void LockPthreadMutex::unlock(){
    if(pthread_mutex_unlock(&_lock) != 0){
        throw runtime_error("Error while unlocking.");
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

Thread::Thread():_thread(0), _running(false){
    ;
}

Thread::~Thread(){
    ;
}

void* Thread::threadDispatcher(void* arg){
    Thread* t = static_cast<Thread*>(arg);
    t->_pid = getpid();
    t->_tid = gettid();
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
            throw runtime_error("Thread: pthread_create failed. "
                                     "Error code: " + utils::intToString(rc));
        }
        _pidSet.wait();
    }else{
        throw runtime_error("Thread: Multiple start. It must be joined "
                                 "before starting it again.");
    }
}

std::pair<task::TaskId, task::TaskId> Thread::getPidAndTid() const{
    return std::pair<task::TaskId, task::TaskId>(_pid, _tid);
}

bool Thread::running(){
    return _running;
}

void Thread::join(){
    int rc = pthread_join(_thread, NULL);
    if(rc != 0){
        throw runtime_error("Thread: join failed. Error code: " +
                                 utils::intToString(rc));
    }
    _pid = 0;
    _tid = 0;
}

Monitor::Monitor():_mutex(){
    int rc = pthread_cond_init(&_condition, NULL);
    if(rc != 0){
        throw runtime_error("Monitor: couldn't initialize condition. "
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
        throw runtime_error("Monitor: error in pthread_cond_wait. Error code: " + utils::intToString(rc));
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
        throw runtime_error("Error: " + utils::intToString(rc) + " in pthread_cond_timedwait.");
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

string getModuleNameFromMessageId(const string& messageId){
    vector<string> tokens = split(messageId, '.');
    if(tokens.size() != 3 || tokens.at(0).compare("mammut")){
        throw runtime_error("Wrong message id: " + messageId);
    }
    return tokens.at(1);
}

#ifdef MAMMUT_REMOTE
string getModuleNameFromMessage(::google::protobuf::MessageLite* const message){
    return getModuleNameFromMessageId(message->GetTypeName());
}

bool setMessageFromData(const ::google::protobuf::MessageLite* outData, string& messageIdOut, string& messageOut){
    messageIdOut = outData->GetTypeName();
    string tmp;
    if(!outData->SerializeToString(&tmp)){
        return false;
    }
    messageOut = tmp;
    return true;
}
#endif

#if defined (__linux__)
bool existsDirectory(const string& dirName){
    struct stat sb;
    return (stat(dirName.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode));
}
#endif

bool existsFile(const string& fileName){
    ifstream f(fileName.c_str());
    return f.good();
}

#if (__linux__)
int executeCommand(const string& command, bool waitResult){
    int status = system((command + " > /dev/null 2>&1" + (!waitResult?" &":"")).c_str());
    if(status == -1){
        throw runtime_error("Impossible to execute command " + command);
    }
    return WEXITSTATUS(status);
}
#else
int executeCommand(const string& command, bool waitResult){
    throw runtime_error("executeCommand not supported on this OS.");
}
#endif

vector<string> getCommandOutput(const string& command){
    vector<string> r;
    FILE *commandFile = popen(command.c_str(), "r");
    char *line = NULL;
    size_t len = 0;

    if(commandFile == NULL){
        throw runtime_error("Impossible to execute command " + command);
    }

    while(getline(&line, &len, commandFile) != -1){
        r.push_back(line);
    }
    free(line);
    pclose(commandFile);
    return r;
}

int stringToInt(const string& s){
    return atoi(s.c_str());
}

uint stringToUint(const string& s){
    return atoll(s.c_str());
}

ulong stringToUlong(const string& s){
    return atoll(s.c_str());
}


double stringToDouble(const string& s){
    return atof(s.c_str());
}

string readFirstLineFromFile(const string& fileName){
    string r;
    ifstream file(fileName.c_str());
    if(file){
        getline(file, r);
        file.close();
    }else{
        throw runtime_error("Impossible to open file " + fileName);
    }
    return r;
}

vector<string> readFile(const string& fileName){
    vector<string> r;
    ifstream file(fileName.c_str());
    if(file){
        string curLine;
        while(getline(file, curLine)){
            r.push_back(curLine);
        }
        file.close();
    }else{
        throw runtime_error("Impossible to open file " + fileName);
    }
    return r;
}

void writeFile(const string& fileName, const vector<string>& lines){
    ofstream file;
    file.open(fileName.c_str());
    if(!file.is_open()){
        throw runtime_error("Impossible to open file: " + fileName);
    }
    for(size_t i = 0; i < lines.size(); i++){
        file << lines.at(i) << "\n";
    }
    file.close();
}

void writeFile(const string& fileName, const string& line){
    ofstream file;
    file.open(fileName.c_str());
    if(!file.is_open()){
        throw runtime_error("Impossible to open file: " + fileName);
    }
    file << line << "\n";
    file.close();
}

void dashedRangeToIntegers(const string& dashedRange, int& rangeStart, int& rangeStop){
    size_t dashPos = dashedRange.find_first_of("-");
    rangeStart = stringToInt(dashedRange.substr(0, dashPos));
    rangeStop = stringToInt(dashedRange.substr(dashPos + 1));
}

string intToString(int x){
    stringstream out;
    out << x;
    return out.str();
}

vector<string>& split(const string& s, char delim, vector<string>& elems){
    stringstream ss(s);
    string item;
    while(getline(ss, item, delim)){
        elems.push_back(item);
    }
    return elems;
}


vector<string> split(const string& s, char delim){
    vector<string> elems;
    split(s, delim, elems);
    return elems;
}

string& ltrim(string& s){
    s.erase(s.begin(), find_if(s.begin(), s.end(), not1(ptr_fun<int, int>(isspace))));
    return s;
}

string& rtrim(string& s) {
    s.erase(find_if(s.rbegin(), s.rend(), not1(ptr_fun<int, int>(isspace))).base(), s.end());
    return s;
}

string& trim(string& s) {
        return ltrim(rtrim(s));
}

string errnoToStr(){
    return strerror(errno);
}

#if defined (__linux__)
vector<string> getFilesNamesInDir(const string& path, bool files, bool directories){
    vector<string> filesNames;
    DIR* dir;
    if((dir = opendir(path.c_str())) != NULL){
        /* print all the files and directories within directory */
        struct dirent* ent;
        while((ent = readdir(dir)) != NULL){
            // Don't consider "." and ".."
            if(string(".").compare(ent->d_name) && string("..").compare(ent->d_name)){
                struct stat st;
                // Get realpath from relative
                char resolved_path[PATH_MAX];
                if(!realpath((std::string(path) + "/" + std::string(ent->d_name)).c_str(), resolved_path)){
                    if(access(resolved_path, F_OK ) != -1){
                        throw std::runtime_error("realpath failed.");
                    }else{
                        // File disappeared between readdir and realpath, just skip
                        continue;
                    }
                }
                if(lstat(resolved_path, &st)){
                    if(access(resolved_path, F_OK ) != -1){
                        throw std::runtime_error("lstat failed.");
                    }else{
                        // File disappeared between readdir and lstat, just skip
                        continue;
                    }
                }
                if((S_ISDIR(st.st_mode) && directories) ||
                   (!S_ISDIR(st.st_mode) && files)){
                    filesNames.push_back(ent->d_name);
                }
            }
        }
        closedir(dir);
    }else{
        throw runtime_error("getFilesList: " + errnoToStr());
    }
    return filesNames;
}
#endif

bool isNumber(const string& s){
    string::const_iterator it = s.begin();
    while (it != s.end() && isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

uint getClockTicksPerSecond(){
    return sysconf(_SC_CLK_TCK);
}

Msr::Msr(uint32_t id){
    string msrFileName = "/dev/cpu/" + intToString(id) + "/msr";
    _fd = open(msrFileName.c_str(), O_RDWR);
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
    if(r != sizeof(value)){
        return false;
    }else{
        return true;
    }
}

bool Msr::write(uint32_t which, uint64_t value){
    if(pwrite(_fd, &value, sizeof(value), which) != sizeof value){
        return false;
    }else{
        return true;
    }
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

bool Msr::writeBits(uint32_t which, unsigned int highBit,
                    unsigned int lowBit, uint64_t value){
    value = value << lowBit;
    uint64_t oldValue;
    read(which, oldValue);

    // Clear all the bits to be set in the old value and all the bits 
    // not to be set in the new value. Then do the OR.
    for(uint i = 0; i < 64; i++){
        if(i >= lowBit && i <= highBit){
            oldValue &= ~(1UL << i);
        } else {
            value  &= ~(1UL << i);
        }
    }
    return write(which, oldValue | value);
}

#ifndef AMESTER_ROOT
#define AMESTER_ROOT simulationParameters.sysfsRootPrefix + "/tmp/amester"
#endif

AmesterSensor::AmesterSensor(string name):
    _name(name){
    ;
}

AmesterSensor::~AmesterSensor(){
    ;
}

vector<string> AmesterSensor::readFields() const{
    ifstream file((string(AMESTER_ROOT) + string("/") + _name).c_str(), ios_base::in);
    string line;
    getline(file, line);
    return split(line, ',');
}

bool AmesterSensor::exists() const{
    ifstream file((string(AMESTER_ROOT) + string("/") + _name).c_str(), ios_base::in);
    return file.is_open();
}

AmesterResult AmesterSensor::readSum() const{
    AmesterResult ar;
    vector<string> fields = readFields();
    double sum = 0;
    // Skip first field (timestamp).
    for(size_t i = 1; i < fields.size(); i++){
        sum += stringToDouble(fields[i]);
    }
    ar.timestamp = stringToUlong(fields[0]);
    ar.value = sum;
    return ar;
}


AmesterResult AmesterSensor::readAme(uint ameId) const{
    AmesterResult ar;
    vector<string> fields = readFields();
    if(1 + ameId >= fields.size()){
        throw runtime_error("Nonexisting ameId.");
    }
    ar.timestamp = stringToUlong(fields[0]);
    ar.value = stringToDouble(fields[1 + ameId]);
    return ar;
}

pid_t gettid(){
#ifdef SYS_gettid
    return syscall(SYS_gettid);
#else
    throw runtime_error("gettid() not available.");
#endif
}

double getMillisecondsTime(){
    struct timespec spec;
    if(clock_gettime(CLOCK_MONOTONIC, &spec) == -1){
        throw runtime_error(string("clock_gettime failed: ") + string(strerror(errno)));
    }
    return spec.tv_sec * 1000.0 + spec.tv_nsec / 1.0e6;
}


CpuIdAsm::CpuIdAsm(unsigned i) {
#ifdef _WIN32
    __cpuid((int *)regs, (int)i);
#else
    asm volatile
    ("cpuid" : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3])
    : "a" (i), "c" (0));
    // ECX is set to zero for CPUID function 4
#endif
}

const uint32_t& CpuIdAsm::EAX() const {return regs[0];}
const uint32_t& CpuIdAsm::EBX() const {return regs[1];}
const uint32_t& CpuIdAsm::ECX() const {return regs[2];}
const uint32_t& CpuIdAsm::EDX() const {return regs[3];}

}
}
