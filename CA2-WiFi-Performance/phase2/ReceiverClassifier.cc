// CA2 phase 2 (provided; do not modify).
#include "ReceiverClassifier.h"

#include "inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h"

namespace ca2 {

Register_Class(ReceiverClassifier);

int ReceiverClassifier::classifyPacket(Packet *packet) const
{
    auto header = packet->peekAtFront<ieee80211::Ieee80211DataOrMgmtHeader>();
    auto it = queueOf.find(header->getReceiverAddress());
    if (it != queueOf.end())
        return it->second;
    int index = queueOf.size();
    queueOf[header->getReceiverAddress()] = index;
    return index;
}

} // namespace ca2
