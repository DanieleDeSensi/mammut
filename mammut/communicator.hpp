/*
 * communicator.hpp
 *
 * Created on: 06/11/2014
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

#ifndef MAMMUT_COMMUNICATOR_HPP_
#define MAMMUT_COMMUNICATOR_HPP_

#include "./utils.hpp"

#include "pthread.h"

#ifdef MAMMUT_REMOTE
#include "google/protobuf/message_lite.h"
#endif

namespace mammut{

class Communicator{
public:
    virtual ~Communicator();

#ifdef MAMMUT_REMOTE
    /**
     * Returns a pointer to a lock associated with the channel.
     * This will be used to ensure no reordering of message and to
     * associate a response to the correct request. Basically it will be used
     * to ensure that, at each time, there will be at most one message traveling
     * on the channel (one message from client to server or one message from server
     * to client).
     * @return A reference to a lock associated with the channel.
     */
    virtual utils::Lock& getLock() const = 0;

    void send(const std::string& messageId, const std::string& message) const;
    /**
     * Reads a message of type messageId.
     * @param messageId The type of the message.
     * @param message The received message.
     * @return true if the message have been received, false if the communication was closed.
     */
    bool receive(std::string& messageId, std::string& message) const;
    void remoteCall(const ::google::protobuf::MessageLite& request,
                    ::google::protobuf::MessageLite& response) const;
protected:
    virtual void send(const char* message, size_t messageLength) const = 0;
    /**
     * Reads messageLength bytes and stores the in message.
     * @param message The received message.
     * @param messageLength The number of bytes to receive.
     * @return true if the bytes have been received, false if the communication was closed.
     */
    virtual bool receive(char* message, size_t messageLength) const = 0;
private:
    void send(const ::google::protobuf::MessageLite& message) const;
    void sendHeader(const std::string& messageId, size_t messageLength) const;
    bool receiveHeader(std::string& messageId, size_t& messageLength) const;
#endif
};
}

#endif /* MAMMUT_COMMUNICATOR_HPP_ */
