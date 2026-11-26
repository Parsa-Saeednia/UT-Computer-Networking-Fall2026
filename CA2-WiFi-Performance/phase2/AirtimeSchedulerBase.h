// CA2 phase 2 (provided; do not modify).
#ifndef __CA2_AIRTIMESCHEDULERBASE_H
#define __CA2_AIRTIMESCHEDULERBASE_H

#include "inet/queueing/base/PacketSchedulerBase.h"
#include "inet/queueing/contract/IPacketCollection.h"
#include "RateTableSelection.h"

namespace ca2 {

using namespace inet;

/**
 * Base class for the AP's per-station scheduler. Input gate i is fed by the
 * queue of one station. A subclass only implements schedulePacket(), which is
 * called exactly once each time the MAC takes the next frame, and returns the
 * index of the queue to serve (or -1 if all queues are empty).
 */
class AirtimeSchedulerBase : public queueing::PacketSchedulerBase, public virtual queueing::IPacketCollection
{
  protected:
    std::vector<queueing::IPacketCollection *> collections;
    RateTableSelection *rateSelection = nullptr;

    virtual void initialize(int stage) override;

    /** Number of per-station queues. */
    int getNumQueues() const { return (int)collections.size(); }

    /** True if queue i holds at least one frame. */
    bool hasPacket(int i) const { return collections[i]->getNumPackets() > 0; }

    /**
     * Airtime of the frame at the head of queue i: the duration of the data
     * PPDU at its receiver's bitrate (preamble and header included; SIFS and
     * ACK not included). Queue i must not be empty.
     */
    simtime_t headAirtime(int i) const;

  public:
    // IPacketCollection: the scheduler looks like one big queue to the MAC
    virtual int getMaxNumPackets() const override { return -1; }
    virtual int getNumPackets() const override;
    virtual b getMaxTotalLength() const override { return b(-1); }
    virtual b getTotalLength() const override;
    virtual bool isEmpty() const override { return getNumPackets() == 0; }
    virtual Packet *getPacket(int index) const override;
    virtual void removePacket(Packet *packet) override;
    virtual void removeAllPackets() override;
};

} // namespace ca2

#endif
