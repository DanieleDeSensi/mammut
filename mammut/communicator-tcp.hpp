#ifndef MAMMUT_COMMUNICATOR_TCP_HPP_
#define MAMMUT_COMMUNICATOR_TCP_HPP_

#ifdef MAMMUT_REMOTE

#include "./communicator.hpp"

#include "cstddef"

namespace mammut{

class ServerTcp;

class CommunicatorTcp: public Communicator{
private:
    int _socket;
    mutable utils::LockPthreadMutex _lock;
public:
    /**
     * Starts a TCP communicator to interact with a remote server.
     * @param serverAddress The address of the remote server.
     * @param serverPort The listening por of the remote server.
     */
    CommunicatorTcp(std::string serverAddress, uint16_t serverPort);

    /**
     * Starts a TCP communicator when a new connection request arrives to serverTcp.
     * @param serverTcp A TCP based server.
     */
    CommunicatorTcp(const ServerTcp& serverTcp);

    ~CommunicatorTcp();
    void send(const char* message, size_t messageLength) const;
    bool receive(char* message, size_t messageLength) const;
    utils::Lock& getLock() const;
};

// A TCP based server.
class ServerTcp{
private:
    int _listenSocket;
public:
    /**
     * Starts a server on the given port.
     * @param listeningPort The listening port that will be used by the server.
     */
    ServerTcp(uint16_t listeningPort);
    ~ServerTcp();

    /**
     * Accept a new connection.
     * @return The socket corresponding to the new connection.
     */
    int accept() const;
};

}

#endif

#endif /* MAMMUT_COMMUNICATOR_TCP_HPP_ */
