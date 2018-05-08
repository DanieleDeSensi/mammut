#ifndef MAMMUT_UTILS_HPP_
#define MAMMUT_UTILS_HPP_

#include "pthread.h"
#include "algorithm"
#include "iostream"
#include "iterator"
#include "memory"
#include "string"
#include "sstream"
#include "vector"
#ifdef MAMMUT_REMOTE
#include "google/protobuf/message_lite.h"
#include "google/protobuf/repeated_field.h"
#endif

#include <fstream>

#define MAMMUT_NANOSECS_IN_MSEC 1000000
#define MAMMUT_NANOSECS_IN_SEC 1000000000
#define MAMMUT_MILLISECS_IN_SEC 1000
#define MAMMUT_MICROSECS_IN_MILLISEC 1000
#define MAMMUT_MICROSECS_IN_SEC 1000000

/* Ticks counter */
#define MSR_TSC 0x10

#define MSR_RAPL_POWER_UNIT 0x606
/*
 * Platform specific RAPL Domains.
 * Note that PP1 RAPL Domain is supported on 062A only
 * And DRAM RAPL Domain is supported on 062D only
 */
/* Package RAPL Domain */
#define MSR_PKG_RAPL_POWER_LIMIT 0x610
#define MSR_PKG_ENERGY_STATUS 0x611
#define MSR_PKG_PERF_STATUS 0x613
#define MSR_PKG_POWER_INFO 0x614

/* PP0 RAPL Domain */
#define MSR_PP0_POWER_LIMIT 0x638
#define MSR_PP0_ENERGY_STATUS 0x639
#define MSR_PP0_POLICY 0x63A
#define MSR_PP0_PERF_STATUS 0x63B

/* PP1 RAPL Domain, may reflect to uncore devices */
#define MSR_PP1_POWER_LIMIT 0x640
#define MSR_PP1_ENERGY_STATUS 0x641
#define MSR_PP1_POLICY 0x642

/* DRAM RAPL Domain */
#define MSR_DRAM_POWER_LIMIT 0x618
#define MSR_DRAM_ENERGY_STATUS 0x619
#define MSR_DRAM_PERF_STATUS 0x61B
#define MSR_DRAM_POWER_INFO 0x61C

/* Voltage */
#define MSR_PERF_STATUS 0x198

/* C states */
#define MSR_PKG_C2_RESIDENCY 0x60D
#define MSR_PKG_C3_RESIDENCY 0x3F8
#define MSR_PKG_C6_RESIDENCY 0x3F9
#define MSR_PKG_C7_RESIDENCY 0x3FA
#define MSR_PKG_C8_RESIDENCY 0x630
#define MSR_PKG_C9_RESIDENCY 0x631
#define MSR_PKG_C10_RESIDENCY 0x632
#define MSR_CORE_C1_RESIDENCY 0x660
#define MSR_CORE_C3_RESIDENCY 0x3FC
#define MSR_CORE_C6_RESIDENCY 0x3FD
#define MSR_CORE_C7_RESIDENCY 0x3FE

/* Clock modulation */
#define MSR_CLOCK_MODULATION 0x19A

namespace mammut{

namespace task{class TasksManager; class ProcessHandler; class ThreadHandler;}
namespace task{
    using TaskId = pid_t;
}

namespace utils{

#define COMPILE_FOR_CX11 0

#if COMPILE_FOR_CX11
    #define CX11_KEYWORD(x) x
    #define mammut_auto_ptr ::std::unique_ptr
    #define mammut_move(x) ::std::move(x)
#else
    #define CX11_KEYWORD(x)
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
    explicit ScopedPtr(T* p = 0):_ptr(p){;}
    ~ScopedPtr() throw(){delete _ptr;}
  private:
    T* _ptr;
};

template <class T>
class ScopedArrayPtr: NonCopyable{
  public:
    explicit ScopedArrayPtr(T* p = 0):_ptr(p){;}
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
    explicit ScopedLock(Lock& lock);
    ~ScopedLock();
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

// A thread wrapper
class Thread: NonCopyable{
public:
    Thread();
    virtual ~Thread();

    /**
     * Starts this thread.
     * ATTENTION: When the object is deallocated the thread will keep running.
     *            It must EXPLICITLY be joined before the object goes out of
     *            scope.
     */
    void start();

    /**
     * Returns the process and thread identifiers of this thread.
     * @return The process and thread identifiers of this thread.
     */
    std::pair<task::TaskId, task::TaskId> getPidAndTid() const;

    /**
     * Checks if the thread is still running.
     * @return True if the thread is still running, false otherwise.
     */
    bool running();

    /**
     * Joins this thread. After this operation, the handler obtained
     * with getThreadHandler() call is no more valid.
     */
    void join();

    /**
     * This is the function that will be executed by the thread.
     * To create a custom thread, is sufficient to extend this class
     * and implement this pure member function.
     */
    virtual void run() = 0;

private:
    static void* threadDispatcher(void* arg);

    pthread_t _thread;
    bool _running;
    Monitor _pidSet;
    task::TaskId _pid, _tid;
};

#ifdef MAMMUT_REMOTE
/**
 * Given a message id, returns the identifier of the module that sent the message.
 * For example, if a messageId is mammut.cpufreq.setFrequency, then the
 * identifier of the module will be "cpufreq".
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
#endif

/**
 * By calling checkDerivedFrom<T,B>, it ensures at compile
 * time that T is derived from B (i.e. T is a subclass of B).
 */
template<class T, class B> struct assertDerivedFrom {
        static void constraints(T* p) { B* pb = p; (void)pb; }
        assertDerivedFrom() { void(*p)(T*) = constraints; (void)p;}
};

/**
 * Checks if a vector contains a specified value.
 * @param vec The vector.
 * @param value The value.
 * @return True if the vector contains the value, false otherwise.
 */
template <class T>
bool contains(const std::vector<T> &vec, const T &value){
    return std::find(vec.begin(), vec.end(), value) != vec.end();
}

/**
 * Checks if a vector contains all the values of another vector.
 * @param x The first vector.
 * @param y The second vector.
 * @return True if all the value of x are contained in y.
 */
template <class T>
bool contains(const std::vector<T> &x, const std::vector<T> &y){
    for(size_t i = 0; i < x.size(); i++){
        if(!contains(y, x.at(i))){
            return false;
        }
    }
    return true;
}

/**
 * Inserts the first 'n' elements of 'a' to the end of 'b'.
 * @param a The first vector.
 * @param b The second vector.
 * @param n The number of elements of 'a' to insert in 'b'.
 */
template <class T>
void insertFrontToEnd(const std::vector<T>& a, std::vector<T>& b, size_t n){
    b.insert(b.end(), a.begin(), a.begin() + n);
}

/**
 * Inserts the elements of 'a' to the end of 'b'.
 * @param a The first vector.
 * @param b The second vector.
 */
template <class T>
void insertToEnd(const std::vector<T>& a, std::vector<T>& b){
    insertFrontToEnd(a, b, a.size());
}

/**
 * Inserts the first 'n' elements of 'a' to the end of 'b',
 * then deletes the elements from 'a'.
 * @param a The first vector.
 * @param b The second vector.
 * @param n The number of elements of 'a' to insert in 'b'.
 */
template <class T>
void moveFrontToEnd(std::vector<T>& a, std::vector<T>& b, size_t n){
    insertFrontToEnd(a, b, n);
    a.erase(a.begin(), a.begin() + n);
}

/**
 * Inserts the elements of 'a' to the end of 'b',
 * then deletes the elements from 'a'.
 * @param a The first vector.
 * @param b The second vector.
 */
template <class T>
void moveToEnd(std::vector<T>& a, std::vector<T>& b){
    moveFrontToEnd(a, b, a.size());
}

/**
 * Inserts the last 'n' elements of 'a' to the front of 'b'.
 * @param a The first vector.
 * @param b The second vector.
 * @param n The number of elements of 'a' to insert in 'b'.
 */
template <class T>
void insertEndToFront(const std::vector<T>& a, std::vector<T>& b, size_t n){
    size_t x = a.size() - n;
    b.insert(b.begin(), a.begin() + x, a.end());
}

/**
 * Inserts the elements of 'a' to the front of 'b'.
 * @param a The first vector.
 * @param b The second vector.
 */
template <class T>
void insertToFront(const std::vector<T>& a, std::vector<T>& b){
    insertEndToFront(a, b, a.size());
}

/**
 * Inserts the last 'n' elements of 'a' to the front of 'b',
 * then deletes the elements from 'a'.
 * @param a The first vector.
 * @param b The second vector.
 * @param n The number of elements of 'a' to insert in 'b'.
 */
template <class T>
void moveEndToFront(std::vector<T>& a, std::vector<T>& b, size_t n){
    insertEndToFront(a, b, n);
    size_t x = a.size() - n;
    a.erase(a.begin() + x, a.end());
}

/**
 * Inserts the elements of 'a' to the front of 'b',
 * then deletes the elements from 'a'.
 * @param a The first vector.
 * @param b The second vector.
 */
template <class T>
void moveToFront(std::vector<T>& a, std::vector<T>& b){
    moveEndToFront(a, b, a.size());
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
 * @param waitResult If true, the call will wait for the result of the command,
 *        otherwise it will be executed in background.
 */
int executeCommand(const std::string& command, bool waitResult);

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
 * Converts a string representation of an unsigned integer to an
 * unsigned integer.
 * @param s The string to be converted.
 * @return The unsigned integer represented by s.
 */
uint stringToUint(const std::string& s);

/**
 * Converts a string representation of an unsigned long to an
 * unsigned long.
 * @param s The string to be converted.
 * @return The unsigned long represented by s.
 */
ulong stringToUlong(const std::string& s);

/**
 * Converts a string representation of a double to a double.
 * @param s The string to be converted.
 * @return The double represented by s.
 */
double stringToDouble(const std::string& s);

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
 * Reads a file and loads it into a vector of strings.
 * @param fileName The name of the file.
 * @return A vector of strings, one for each line in the file.
 */
std::vector<std::string> readFile(const std::string& fileName);

/**
 * Writes a vector of lines to a file (overwrites).
 * @param fileName The name of the file.
 * @param lines The vector of lines (one string for each line).
 */
void writeFile(const std::string& fileName, const std::vector<std::string>& lines);

/**
 * Writes a line to a file (overwrites).
 * @param fileName The name of the file.
 * @param line The line to be written.
 */
void writeFile(const std::string& fileName, const std::string& line);

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

/**
 * Returns the number of clock ticks per second.
 * @return The number of clock ticks per second.
 */
uint getClockTicksPerSecond();

/** Represents Intel MSR registers of a specific virtual core. **/
class Msr{
private:
    int _fd;

public:
    /**
     * @param id The identifier of the virtual core.
     */
    explicit Msr(uint32_t id);
    ~Msr();
    /**
     * Returns true if the registers are available.
     * @return True if the registers are available,
     */
    bool available() const;

    /**
     * Reads a specific register.
     * @param which The register.
     * @param value The value of the register.
     * @return True if the register is present, false otherwise.
     */
    bool read(uint32_t which, uint64_t& value) const;
    
    /**
     * Writes specified bits of a specified register.
     * @param which The register.
     * @param value The bits of the specified register.
     * @return True if the register is present, false otherwise.
     */
    bool write(uint32_t which, uint64_t value);
    
    /**
     * Reads specified bits of a specified register.
     * @param which The register.
     * @param highBit The highest bit to read.
     * @param lowBit The lowest bit to read.
     * @param value The bits of the specified register.
     * @return True if the register is present, false otherwise.
     */
    bool readBits(uint32_t which, unsigned int highBit,
                  unsigned int lowBit, uint64_t& value) const;

    /**
     * Writes specified bits of a specified register.
     * @param which The register.
     * @param highBit The highest bit to write.
     * @param lowBit The lowest bit to write.
     * @param  The bits of the specified register.
     * @return True if the register is present, false otherwise.
     */
    bool writeBits(uint32_t which, unsigned int highBit,
                   unsigned int lowBit, uint64_t value);
};

typedef struct{
    ulong timestamp;
    double value;
}AmesterResult;

/** Represents a generic Amester sensor. **/
class AmesterSensor{
private:
    std::string _name;
    std::vector<std::string> readFields() const;
public:
    explicit AmesterSensor(std::string name);
    ~AmesterSensor();

    bool exists() const;

    /**
     * Reads the sum of the sensor value for all the AMEs.
     * @return The sum of the sensor value for all the AMEs.
     */
    AmesterResult readSum() const;

    /**
     * Reads the value of the sensor for a specific AME.
     * @param ameId The identifier of the AME.
     * @return The value of the sensor for a specific AME.
     */
    AmesterResult readAme(uint ameId) const;
};
/**
 * Returns the thread identifier of the calling thread.
 * @return The thread identifier of the calling thread.
 */
pid_t gettid();

/**
 * Returns monotonic milliseconds time.
 * @return Monotonic milliseconds time.
 */
double getMillisecondsTime();

/**
 * Str<->Enum mappings
 * Code from http://codereview.stackexchange.com/a/14315
 */

// This is the type that will hold all the strings.
// Each enumerate type will declare its own specialization.
// Any enum that does not have a specialization will generate a compiler error
// indicating that there is no definition of this variable (as there should be
// be no definition of a generic version).
template<typename T>
struct enumStrings{
    static char const* data[];
};

// This is a utility type.
// Creted automatically. Should not be used directly.
template<typename T>
struct enumRefHolder{
    T& enumVal;
    explicit enumRefHolder(T& enumVal): enumVal(enumVal) {}
};

template<typename T>
struct enumConstRefHolder{
    T const& enumVal;
    explicit enumConstRefHolder(T const& enumVal): enumVal(enumVal) {}
};

// The next too functions do the actual work of reading/writtin an
// enum as a string.
template<typename T>
std::ostream& operator<<(std::ostream& str, enumConstRefHolder<T> const& data){
   return str << enumStrings<T>::data[data.enumVal];
}

template<typename T>
std::istream& operator>>(std::istream& str, enumRefHolder<T> const& data){
    std::string value;
    str >> value;

    // These two can be made easier to read in C++11
    // using std::begin() and std::end()
    //
    static auto begin  = std::begin(enumStrings<T>::data);
    static auto end    = std::end(enumStrings<T>::data);

    auto find = std::find(begin, end, value);
    if (find != end){
        data.enumVal = static_cast<T>(std::distance(begin, find));
    }
    return str;
}


// This is the public interface:
// use the ability of function to deuce their template type without
// being explicitly told to create the correct type of enumRefHolder<T>
template<typename T>
enumConstRefHolder<T> enumToStringInternal(T const& e){
    return enumConstRefHolder<T>(e);
}

template<typename T>
enumRefHolder<T> enumFromStringInternal(T& e){
    return enumRefHolder<T>(e);
}

template<typename T>
std::string enumToString(T const& e){
    std::stringstream s;
    s << utils::enumToStringInternal(e);
    return s.str();
}

template<typename T>
T stringToEnum(const std::string& s, T& e){
    std::stringstream ss(s);
    ss >> utils::enumFromStringInternal(e);
    return e;
}

#ifdef _WIN32
#include <limits.h>
#include <intrin.h>
typedef unsigned __int32  uint32_t;

#else
#include <stdint.h>
#endif

class CpuIdAsm {
  uint32_t regs[4];

public:
  explicit CpuIdAsm(unsigned i);
  const uint32_t &EAX() const;
  const uint32_t &EBX() const;
  const uint32_t &ECX() const;
  const uint32_t &EDX() const;
};

}

// Some parameters to simulate Mammut execution.
// Only intended for testing purposes.
typedef struct SimulationParameters{
    std::string sysfsRootPrefix;
}SimulationParameters;

}

#endif /* MAMMUT_UTILS_HPP_ */
