/* Copyright 2013-2017 Sathya Laufer
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Homegear.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

#ifndef BIDCOSQUEUE_H
#define BIDCOSQUEUE_H

#include <homegear-base/BaseLib.h>
#include "BidCoSPacket.h"
#include "PhysicalInterfaces/IBidCoSInterface.h"

#include <iostream>
#include <string>
#include <list>
#include <queue>
#include <thread>
#include <mutex>

namespace BidCoS
{
class BidCoSPeer;
class BidCoSMessage;
class HomeMaticCentral;
class PendingBidCoSQueues;

enum class QueueEntryType { UNDEFINED, MESSAGE, PACKET };

class CallbackFunctionParameter
{
public:
	std::vector<int64_t> integers;
	std::vector<std::string> strings;

	CallbackFunctionParameter() {}
	virtual ~CallbackFunctionParameter() {}
};

class BidCoSQueueEntry {
protected:
	QueueEntryType _type = QueueEntryType::UNDEFINED;
	std::shared_ptr<BidCoSMessage> _message;
	std::shared_ptr<BidCoSPacket> _packet;
public:
	bool stealthy = false;

	BidCoSQueueEntry() {}
	virtual ~BidCoSQueueEntry() {}
	QueueEntryType getType() { return _type; }
	void setType(QueueEntryType type) { _type = type; }
	std::shared_ptr<BidCoSPacket> getPacket() { return _packet; }
	void setPacket(std::shared_ptr<BidCoSPacket> packet, bool setQueueEntryType) { _packet = packet; if(setQueueEntryType) _type = QueueEntryType::PACKET; }
	std::shared_ptr<BidCoSMessage> getMessage() { return _message; }
	void setMessage(std::shared_ptr<BidCoSMessage> message, bool setQueueEntryType) { _message = message; if(setQueueEntryType) _type = QueueEntryType::MESSAGE; }
};

enum class BidCoSQueueType { EMPTY, DEFAULT, CONFIG, PAIRING, PAIRINGCENTRAL, UNPAIRING, PEER, SETAESKEY, GETVALUE };

class BidCoSQueue
{
    protected:
		std::atomic_bool _disposing;
		bool _setWakeOnRadioBit = false;
		//I'm using list, so iterators are not invalidated
        std::list<BidCoSQueueEntry> _queue;
        std::shared_ptr<IBidCoSInterface> _physicalInterface;
        std::shared_ptr<PendingBidCoSQueues> _pendingQueues;
        std::mutex _queueMutex;
        BidCoSQueueType _queueType;
        std::thread _sendThread;
        std::mutex _sendThreadMutex;
        std::thread _startResendThread;
        std::mutex _startResendThreadMutex;
        std::thread _pushPendingQueueThread;
        std::mutex _pushPendingQueueThreadMutex;
        std::atomic_bool _workingOnPendingQueue;
        int64_t _lastPop = 0;
        void (HomeMaticCentral::*_queueProcessed)() = nullptr;
        void pushPendingQueue();
        void nextQueueEntry();
    public:
        uint32_t id = 0;
        uint32_t pendingQueueID = 0;
        std::shared_ptr<int64_t> lastAction;
        std::atomic_bool noSending;
        std::shared_ptr<BidCoSPeer> peer;
        std::shared_ptr<CallbackFunctionParameter> callbackParameter;
        std::function<void(std::shared_ptr<CallbackFunctionParameter>)> queueEmptyCallback;
        BidCoSQueueType getQueueType() { return _queueType; }
        std::list<BidCoSQueueEntry>* getQueue() { return &_queue; }
        void setQueueType(BidCoSQueueType queueType) {  _queueType = queueType; }
        std::shared_ptr<IBidCoSInterface> getPhysicalInterface() { return _physicalInterface; }
        void setPhysicalInterface(std::shared_ptr<IBidCoSInterface> interface) { _physicalInterface = interface; }
        std::string parameterName;
        int32_t channel = -1;

        void push(std::shared_ptr<BidCoSMessage> message);
        void pushFront(std::shared_ptr<BidCoSPacket> packet, bool stealthy = false, bool popBeforePushing = false);
        void push(std::shared_ptr<BidCoSPacket> packet, bool stealthy = false);
        void push(std::shared_ptr<PendingBidCoSQueues>& pendingBidCoSQueues);
        void push(std::shared_ptr<BidCoSQueue> pendingBidCoSQueue, bool popImmediately, bool clearPendingQueues);
        BidCoSQueueEntry* front() { return &_queue.front(); }
        BidCoSQueueEntry* second() { if(_queue.size() > 1) return &(*(_queue.begin()++)); else return nullptr; }
        void pop();
        bool isEmpty();
        bool pendingQueuesEmpty();
        void clear();
        void send(std::shared_ptr<BidCoSPacket> packet, bool stealthy);
        void keepAlive();
        void longKeepAlive();
        void setWakeOnRadioBit();
        void dispose();
        void serialize(std::vector<uint8_t>& encodedData);
        void unserialize(std::shared_ptr<std::vector<char>> serializedData, uint32_t position = 0);

        BidCoSQueue();
        BidCoSQueue(std::shared_ptr<IBidCoSInterface> physicalDevice);
        BidCoSQueue(std::shared_ptr<IBidCoSInterface> physicalDevice, BidCoSQueueType queueType);
        virtual ~BidCoSQueue();
};
}
#endif // BIDCOSQUEUE_H
