#include "AirtimeDrrScheduler.h"

namespace ca2 {

Define_Module(AirtimeDrrScheduler);

void AirtimeDrrScheduler::initialize(int stage)
{
    AirtimeSchedulerBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        quantum = par("quantum");
        if (quantum <= SIMTIME_ZERO)
            throw cRuntimeError("quantum must be positive, got %s", quantum.str().c_str());
        deficit.assign(getNumQueues(), SIMTIME_ZERO);
        WATCH_VECTOR(deficit);
    }
}

bool AirtimeDrrScheduler::hasAnyPacket() const
{
    int n = getNumQueues();
    for (int i = 0; i < n; ++i) {
        if (hasPacket(i))
            return true;
    }
    return false;
}

// Called once each time the MAC takes a frame. Return the index of the queue
// to serve, or -1 if every queue is empty. Available helpers (see
// AirtimeSchedulerBase.h): getNumQueues(), hasPacket(i), headAirtime(i).
int AirtimeDrrScheduler::schedulePacket()
{
    int n = getNumQueues();
    if (n <= 0 || !hasAnyPacket())
        return -1;

    while (true) {
        if (!hasPacket(current)) {
            // Reset deficit so idle stations cannot hoard airtime credit.
            deficit[current] = SIMTIME_ZERO;
            current = (current + 1) % n;
        }
        else if (deficit[current] <= SIMTIME_ZERO) {
            deficit[current] += quantum;
            current = (current + 1) % n;
        }
        else {
            deficit[current] -= headAirtime(current);
            return current;
        }
    }
}

} // namespace ca2
