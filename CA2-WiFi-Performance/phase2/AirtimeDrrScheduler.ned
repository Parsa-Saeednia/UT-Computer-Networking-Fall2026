package phase2;

import inet.queueing.base.PacketSchedulerBase;
import inet.queueing.contract.IPacketScheduler;

// The AP's per-station scheduler. Your code is in AirtimeDrrScheduler.cc.
simple AirtimeDrrScheduler extends AirtimeSchedulerBase like IPacketScheduler
{
    parameters:
        double quantum @unit(s) = default(500us);   // airtime added to a station's deficit per round
        @class(ca2::AirtimeDrrScheduler);
}
