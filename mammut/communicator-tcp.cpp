#ifdef MAMMUT_REMOTE

#include "./communicator-tcp.hpp"

#include "errno.h"
#include "stdexcept"
#include "string.h"
#include "unistd.h"
#include "arpa/inet.h"
#include "netinet/in.h"
#include "sys/socket.h"
#include "sys/types.h"

namespace mammut{

CommunicatorTcp::CommunicatorTcp(std::string serverAddress, uint16_t serverPort):_lock(utils::LockPthreadMutex()){
    if((_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        throw std::runtime_error("CommunicatorTcp: Impossible to open the socket.");
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(serverPort);
    if(inet_pton(AF_INET, serverAddress.c_str(), &serv_addr.sin_addr)<=0){
        close(_socket);
        throw std::runtime_error("CommunicatorTcp: Impossible to convert the address: " + serverAddress);
    }

    if(connect(_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        close(_socket);
        throw std::runtime_error("CommunicatorTcp: Impossible to connect to the server.");
    }
}

CommunicatorTcp::CommunicatorTcp(const ServerTcp& serverTcp){
    _socket = serverTcp.accept();
}

CommunicatorTcp::~CommunicatorTcp(){
    close(_socket);
}

void CommunicatorTcp::send(const char* message, size_t messageLength) const{
    if(write(_socket, message, messageLength) != (ssize_t) messageLength){
        throw std::runtime_error("CommunicatorTcp: Write failed: " + utils::errnoToStr());
    }
}

bool CommunicatorTcp::receive(char* message, size_t messageLength) const{
    int result;
    size_t bytes_read = 0;
    while (bytes_read < messageLength){
        result = read(_socket, message + bytes_read, messageLength - bytes_read);
        if(messageLength != 0 && result == 0){
            return false;
        }else if(result < 0){
            throw std::runtime_error("CommunicatorTcp: Read failed: " + utils::errnoToStr());
        }
        bytes_read += result;
    }
    return true;
}

utils::Lock& CommunicatorTcp::getLock() const{
    return _lock;
}

ServerTcp::ServerTcp(uint16_t listeningPort){
    struct sockaddr_in serv_addr;
    _listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(_listenSocket == -1){
        throw std::runtime_error("ServerTcp: Impossible to open the listen socket.");
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(listeningPort);

    if(bind(_listenSocket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1){
        close(_listenSocket);
        throw std::runtime_error("ServerTcp: Impossible to bind the socket.");
    }

    if(listen(_listenSocket, 10) == -1){
        close(_listenSocket);
        throw std::runtime_error("ServerTcp: Impossible to listen on the socket.");
    }
}

ServerTcp::~ServerTcp(){
    close(_listenSocket);
}

int ServerTcp::accept() const{
    int socket = ::accept(_listenSocket, (struct sockaddr*)NULL, NULL);
    if(socket == -1){;
        throw std::runtime_error("ServerTcp: Impossible to accept on the socket: " + utils::errnoToStr());
    }
    return socket;
}

}

#endif
