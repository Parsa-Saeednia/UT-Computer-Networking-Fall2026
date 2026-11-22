// CA2 phase 2 (provided; do not modify).
package phase2;

import inet.queueing.base.PacketSchedulerBase;

// Base module for the AP's per-station scheduler.
simple AirtimeSchedulerBase extends PacketSchedulerBase
{
    parameters:
        string rateSelectionModule = default("^.^.^.rateSelection");
        @class(ca2::AirtimeSchedulerBase);
}
