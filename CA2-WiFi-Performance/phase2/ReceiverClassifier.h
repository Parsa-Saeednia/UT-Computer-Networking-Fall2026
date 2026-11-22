// CA2 phase 2 (provided; do not modify).
#ifndef __CA2_RECEIVERCLASSIFIER_H
#define __CA2_RECEIVERCLASSIFIER_H

#include <map>

#include "inet/linklayer/common/MacAddress.h"
#include "inet/queueing/contract/IPacketClassifierFunction.h"

namespace ca2 {

using namespace inet;

/**
 * Puts the frames of each receiver in its own queue: the first receiver seen
 * gets queue 0, the next one queue 1, and so on.
 */
class ReceiverClassifier : public cObject, public virtual queueing::IPacketClassifierFunction
{
  protected:
    mutable std::map<MacAddress, int> queueOf;

  public:
    virtual int classifyPacket(Packet *packet) const override;
};

} // namespace ca2

#endif
