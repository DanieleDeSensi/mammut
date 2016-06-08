#ifdef MAMMUT_REMOTE

#undef DEBUG
//#define DEBUG_COMMUNICATOR
#ifdef DEBUG_COMMUNICATOR
#include "iostream"
#define DEBUG(x) do { std::cerr << "[Communicator] " << x << std::endl; } while (0)
#else
#define DEBUG(x)
#endif

#include "./communicator.hpp"

#include "stdexcept"
#include "netinet/in.h"

namespace mammut{

Communicator::~Communicator(){
    ;
}

void Communicator::send(const ::google::protobuf::MessageLite& message) const{
    sendHeader(message.GetTypeName(), message.ByteSize());

    std::string serializedMsg;
    message.SerializeToString(&serializedMsg);
    assert(serializedMsg.length() == (size_t) message.ByteSize());
    send(serializedMsg.c_str(), message.ByteSize());
}

void Communicator::send(const std::string& messageId, const std::string& message) const{
    size_t messageLen = message.length();
    sendHeader(messageId, messageLen);
    send(message.c_str(), messageLen);
}

bool Communicator::receive(std::string& messageId, std::string& message) const{
    size_t messageLength;
    if(!receiveHeader(messageId, messageLength)){
        return false;
    }
    char* messageArray = new char[messageLength];
    utils::ScopedArrayPtr<char> scp(messageArray);
    if(!receive(messageArray, messageLength)){
        throw std::runtime_error("Communicator: Truncated receive.");
    }
    message.assign(messageArray, messageLength);
    return true;
}

void Communicator::remoteCall(const ::google::protobuf::MessageLite& request, ::google::protobuf::MessageLite& response) const{
    std::string responseMessageId;
    std::string responseMessage;

    {
        utils::ScopedLock scopedLock(getLock());
        send(request);
        if(!receive(responseMessageId, responseMessage)){
            throw std::runtime_error("Communicator: Server closed connection while receiving response.");
        }
    }

    /** The call generated an exception on server side. **/
    if(!responseMessageId.compare("")){
        throw std::runtime_error(responseMessage);
    }

    if(responseMessageId.compare(response.GetTypeName())){
        throw std::runtime_error("remoteCall: Expected message does not match with received one.");
    }

    if(!response.ParseFromString(responseMessage)){
        throw std::runtime_error("remoteCall: Impossible to parse received message.");
    }
}

void Communicator::sendHeader(const std::string& messageId, size_t messageLength) const{
    size_t messageIdLen = messageId.length();
    uint32_t outMessageIdLen = htonl(messageIdLen);
    uint32_t outMessageLength = htonl((uint32_t) messageLength);
    send((char*) &outMessageIdLen, sizeof(uint32_t));
    if(messageIdLen){
        send(messageId.c_str(), messageIdLen);
    }
    send((char*) &outMessageLength, sizeof(uint32_t));
    DEBUG("Sent header: " + utils::intToString(messageIdLen) + "|" + messageId + "|" + utils::intToString(messageLength));
}

bool Communicator::receiveHeader(std::string& messageId, size_t& messageLength) const{
    uint32_t inMessageIdLen, inMessageLength, messageIdLen;

    if(!receive((char*) &inMessageIdLen, sizeof(uint32_t))){
        return false;
    }
    messageIdLen = ntohl(inMessageIdLen);

    if(messageIdLen){
        char* messageIdArr = new char[messageIdLen];
        utils::ScopedArrayPtr<char> sap(messageIdArr);
        if(!receive(messageIdArr, messageIdLen)){
            throw std::runtime_error("Communicator: Truncated receive.");
        }
        messageId.assign(messageIdArr, messageIdLen);
    }

    if(!receive((char*) &inMessageLength, sizeof(uint32_t))){
        throw std::runtime_error("Communicator: Truncated receive.");
    }
    messageLength = ntohl(inMessageLength);
    DEBUG("Received header: " + utils::intToString(messageIdLen) + "|" + messageId + "|" + utils::intToString(messageLength));
    return true;
}

}

#endif
