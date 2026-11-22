// CA2 phase 2 (provided; do not modify).
#include "AirtimeSchedulerBase.h"

#include "inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h"

namespace ca2 {

void AirtimeSchedulerBase::initialize(int stage)
{
    PacketSchedulerBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        for (auto provider : providers)
            collections.push_back(check_and_cast<queueing::IPacketCollection *>(provider.get()));
        rateSelection = check_and_cast<RateTableSelection *>(getModuleByPath(par("rateSelectionModule")));
    }
}

simtime_t AirtimeSchedulerBase::headAirtime(int i) const
{
    auto packet = collections[i]->getPacket(0);
    auto header = packet->peekAtFront<ieee80211::Ieee80211DataOrMgmtHeader>();
    auto mode = rateSelection->getModeFor(header->getReceiverAddress());
    if (mode == nullptr)
        throw cRuntimeError("No bitrate known for receiver %s", header->getReceiverAddress().str().c_str());
    return mode->getDuration(packet->getDataLength());
}

int AirtimeSchedulerBase::getNumPackets() const
{
    int size = 0;
    for (auto collection : collections)
        size += collection->getNumPackets();
    return size;
}

b AirtimeSchedulerBase::getTotalLength() const
{
    b totalLength(0);
    for (auto collection : collections)
        totalLength += collection->getTotalLength();
    return totalLength;
}

Packet *AirtimeSchedulerBase::getPacket(int index) const
{
    int origIndex = index;
    for (auto collection : collections) {
        int numPackets = collection->getNumPackets();
        if (index < numPackets)
            return collection->getPacket(index);
        index -= numPackets;
    }
    throw cRuntimeError("Index %i out of range", origIndex);
}

void AirtimeSchedulerBase::removePacket(Packet *packet)
{
    Enter_Method("removePacket");
    for (auto collection : collections) {
        for (int j = 0; j < collection->getNumPackets(); j++) {
            if (collection->getPacket(j) == packet) {
                collection->removePacket(packet);
                return;
            }
        }
    }
    throw cRuntimeError("Cannot find packet");
}

void AirtimeSchedulerBase::removeAllPackets()
{
    Enter_Method("removeAllPackets");
    for (auto collection : collections)
        collection->removeAllPackets();
}

} // namespace ca2
