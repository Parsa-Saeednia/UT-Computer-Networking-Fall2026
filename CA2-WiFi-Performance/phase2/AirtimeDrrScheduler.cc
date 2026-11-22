#include "AirtimeDrrScheduler.h"

namespace ca2 {

Define_Module(AirtimeDrrScheduler);

void AirtimeDrrScheduler::initialize(int stage)
{
    AirtimeSchedulerBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        quantum = par("quantum");
        deficit.assign(getNumQueues(), SIMTIME_ZERO);
        WATCH_VECTOR(deficit);
    }
}

// Called once each time the MAC takes a frame. Return the index of the queue
// to serve, or -1 if every queue is empty. Available helpers (see
// AirtimeSchedulerBase.h): getNumQueues(), hasPacket(i), headAirtime(i).
int AirtimeDrrScheduler::schedulePacket()
{
    // Starter code: plain round robin, one frame per non-empty queue in turn.
    // TODO: turn this into airtime deficit round robin (see the assignment).
    int n = getNumQueues();
    for (int k = 0; k < n; k++) {
        int i = (current + k) % n;
        if (hasPacket(i)) {
            current = (i + 1) % n;
            return i;
        }
    }
    return -1;
}

} // namespace ca2
