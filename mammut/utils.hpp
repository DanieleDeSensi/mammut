/*
 * utils.hpp
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

#ifndef UTILS_HPP_
#define UTILS_HPP_

#include <algorithm>
#include <pthread.h>
#include <memory>
#include <vector>
#include <google/protobuf/message_lite.h>
#include <google/protobuf/repeated_field.h>

namespace mammut{
namespace utils{

#define COMPILE_FOR_CX11

#ifdef COMPILE_FOR_CX11 //TODO:
    #define mammut_auto_ptr ::std::unique_ptr
    #define mammut_move(x) ::std::move(x)
#else
    #define mammut_auto_ptr ::std::auto_ptr
    #define mammut_move(x) x
#endif

class NonCopyable {
private:
   NonCopyable(NonCopyable const &);
   NonCopyable& operator=(NonCopyable const &);
public:
   NonCopyable(){;}
};

template <class T>
class ScopedPtr: NonCopyable{
  public:
    ScopedPtr(T* p = 0):_ptr(p){;}
    ~ScopedPtr() throw(){delete _ptr;}
  private:
    T* _ptr;
};

template <class T>
class ScopedArrayPtr: NonCopyable{
  public:
    ScopedArrayPtr(T* p = 0):_ptr(p){;}
    ~ScopedArrayPtr() throw(){delete[] _ptr;}
  private:
    T* _ptr;
};

class Lock{
public:
    virtual ~Lock();
    virtual void lock() = 0;
    virtual void unlock() = 0;
};

class LockPthreadMutex: public Lock{
private:
    pthread_mutex_t _lock;
public:
    LockPthreadMutex();
    ~LockPthreadMutex();
    void lock();
    void unlock();
    pthread_mutex_t* getLock();
};

class LockEmpty: public Lock{
public:
    LockEmpty();
    ~LockEmpty();
    void lock();
    void unlock();
};

// A lock scoped wrapper. Locked when created, unlocked when destroyed.
class ScopedLock: NonCopyable{
private:
    Lock& _lock;
public:
    ScopedLock(Lock& lock);
    ~ScopedLock();
};


// A thread wrapper
class Thread: NonCopyable{
public:
    Thread();
    virtual ~Thread();

    /**
     * Starts this thread.
     */
    void start();

    /**
     * Checks if the thread finished its execution.
     */
    bool finished();

    /**
     * Joins this thread.
     */
    void join();

    /**
     * Cancel this thread.
     */
    void cancel();

    /**
     * This is the function that will be executed by the thread.
     * To create a custom thread, is sufficient to extend this class
     * and implement this pure member function.
     */
    virtual void run() = 0;
private:
    static void* threadDispatcher(void* arg);
    void setFinished();

    pthread_t* _thread;
    LockPthreadMutex _mutex;
    bool _finished;
};

class Monitor{
private:
    LockPthreadMutex _mutex;
    pthread_cond_t _condition;
    bool _predicate;
public:
    Monitor();
    bool predicate();
    void wait();
    bool timedWait(int milliSeconds);
    void notifyOne();
    void notifyAll();
};

/**
 * Given a message id, returns the identifier of the module that sent the message.
 * For example, if a messageId is mammut.cpufreq.setFrequency, then the identifier of
 * the module will be "cpufreq".
 * @param messageId The message id.
 * @return The identifier of the module that sent the message.
 */
std::string getModuleNameFromMessageId(const std::string& messageId);

/**
 * Given a message, returns the identifier of the module that sent the message.
 * @param message The message.
 * @return The identifier of the module that sent the message.
 */
std::string getModuleNameFromMessage(::google::protobuf::MessageLite* const message);

/**
 * By calling checkDerivedFrom<T,B>, it ensures at compile
 * time that T is derived from B (i.e. T is a subclass of B).
 */
template<class T, class B> struct assertDerivedFrom {
        static void constraints(T* p) { B* pb = p; (void)pb; }
        assertDerivedFrom() { void(*p)(T*) = constraints; (void)p;}
};

template <class T>
bool contains(const std::vector<T> &vec, const T &value){
    return std::find(vec.begin(), vec.end(), value) != vec.end();
}

/**
 * Tries to convert a string encoded message to the real message data.
 * @param messageIdIn The name of the message.
 * @param messageIn The string encoded message.
 * @param message The real converted message data.
 * @return true if conversion is successful, false if typename T does not match with messageIdIn or if an error occurred.
 */
template <typename T> bool getDataFromMessage(const std::string& messageIdIn, const std::string& messageIn, T& message){
    if(messageIdIn.compare(T::default_instance().GetTypeName()) == 0){
        return message.ParseFromString(messageIn);
    }else{
        return false;
    }
}

/**
 * Creates e string encoded message representation starting from the real message data.
 * @param outData The real message data.
 * @param messageIdOut The name of the message.
 * @param messageOut The real message date.
 * @return ture if conversion is successful, false otherwise.
 */
bool setMessageFromData(const ::google::protobuf::MessageLite* outData, std::string& messageIdOut, std::string& messageOut);

/**
 * Converts a protocol buffer repeated field to a std::vector.
 * @param pb The protocol buffer repeated field.
 * @param v A std::vector containing all the elements of the repeated field.
 * @return v
 */
template <typename T> std::vector<T> pbRepeatedToVector(const ::google::protobuf::RepeatedField<T>& pb, std::vector<T>& v){
    v.assign(pb.data(), pb.data() + pb.size());
    return v;
}

/**
 * Converts a std::vector to a protocol buffer repeated field.
 * @param v The std::vector.
 * @param pb The protocol buffer repeated field.
 * @return pb
 */
template <typename T> ::google::protobuf::RepeatedField<T>* vectorToPbRepeated(const std::vector<T>& v, ::google::protobuf::RepeatedField<T>* pb){
    pb->Clear();
    pb->Reserve(v.size());
    for(unsigned int i = 0; i < v.size(); i++){
        pb->Add(v.at(i));
    }
    return pb;
}

/**
 * Converts a vector of a type into one of a different type.
 * WARNING: It performs static_cast. Be sure to use it correctly
 * @param source Source vector.
 * @param destination Destination vector.
 */
template <typename T, typename V> void convertVector(const std::vector<T>& source, std::vector<V>& destination){
    unsigned int i;
    destination.clear();
    destination.reserve(source.size());
    for(i = 0; i < source.size(); i++){
        destination.push_back(static_cast<V>(source[i]));
    }
}

/**
 * Checks if a directory exists.
 * @param dirName The name of the directory.
 * @return true if the directory exists, false otherwise.
 */
bool existsDirectory(const std::string& dirName);

/**
 * Checks if a file exists.
 * @param fileName The name of the file.
 * @return true if the file exists, false otherwise.
 */
bool existsFile(const std::string& fileName);

/**
 * Executes a shell command.
 * @param command The command to be executed.
 */
void executeCommand(const std::string& command);

/**
 * Returns the output of a shell command.
 * @param command The command to be executed.
 * @return A vector of strings (one per output line).
 */
std::vector<std::string> getCommandOutput(const std::string& command);

/**
 * Converts a string representation of an integer to an integer.
 * @param s The string to be converted.
 * @return The integer represented by s.
 */
int stringToInt(const std::string& s);

/**
 * Converts an integer to a string.
 * @param x The integer to be converted.
 * @return The string representation of x.
 */
std::string intToString(int x);

/**
 * Reads the first line of a file.
 * @param fileName The name of the file.
 * @return The first line of file fileName.
 */
std::string readFirstLineFromFile(const std::string& fileName);

/**
 * Extracts the 2 range integer bounds A and B from a string of the form "A-B".
 * @param dashedRange The range string.
 * @param rangeStart The start of the range.
 * @param rangeStop The end of the range.
 */
void dashedRangeToIntegers(const std::string& dashedRange, int& rangeStart, int& rangeStop);

/**
 * Call a delete on all the elements of a vector and remove the element itself from the vector.
 * @param v The vector.
 */
template <typename T> void deleteVectorElements(std::vector<T>& v){
    while(!v.empty()) delete v.back(), v.pop_back();
}

/**
 * Splits a string on character delim.
 * @param s The string to split.
 * @param delim The delimiter.
 * @param elems The vector of tokens.
 * @return elems Filled with tokens of s. Ther may be empty tokens
 */
std::vector<std::string>& split(const std::string& s, char delim, std::vector<std::string>& elems);

/**
 * Splits a string on character delim.
 * @param s The string to split.
 * @param delim The delimiter.
 * @return A vector of tokens of s. There may be empty tokens.
 */
std::vector<std::string> split(const std::string &s, char delim);

/**
 * Trims a string from start.
 * @param s The string to be trimmed.
 * @return The trimmed string (modifies original string).
 */
std::string& ltrim(std::string& s);

/**
 * Trims a string from end.
 * @param s The string to be trimmed.
 * @return The trimmed string (modifies original string).
 */
std::string& rtrim(std::string& s);

/**
 * Trims a string from both ends.
 * @param s The string to be trimmed.
 * @return The trimmed string (modifies original string).
 */
std::string& trim(std::string& s);

/**
 * Returns a string representation of the last errno set.
 * @return A string representation of the last errno set.
 */
std::string errnoToStr();

/**
 * Returns the files names in a specified directory.
 * @param path The path of the directory.
 * @param files If true returns the files names in the directory.
 * @param directories If true returns the directories names in the directory.
 */
std::vector<std::string> getFilesNamesInDir(const std::string& path, bool files = true, bool directories = true);

/**
 * Checks if a string represent a number.
 * @param s The string.
 * @return True if the string represents a number, false otherwise.
 */
bool isNumber(const std::string& s);

}
}

#endif /* UTILS_HPP_ */
