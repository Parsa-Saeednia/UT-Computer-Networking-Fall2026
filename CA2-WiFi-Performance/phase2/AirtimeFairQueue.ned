// CA2 phase 2 (provided; do not modify).
package phase2;

import inet.queueing.classifier.PacketClassifier;
import inet.queueing.contract.IPacketQueue;
import inet.queueing.contract.IPacketScheduler;
import inet.queueing.queue.CompoundPacketQueueBase;

// The AP's MAC queue, one sub-queue per receiving station. It replaces the
// single FIFO "pendingQueue" of the MAC; the scheduler decides which station
// is served next.
module AirtimeFairQueue extends CompoundPacketQueueBase
{
    parameters:
        int numQueues = default(16);        // at least the number of stations
        int queueCapacity = default(50);    // frames per station queue (drop-tail)
    submodules:
        classifier: PacketClassifier {
            classifierClass = "ca2::ReceiverClassifier";
            @display("p=100,100");
        }
        queue[numQueues]: <default("DropTailQueue")> like IPacketQueue {
            packetCapacity = parent.queueCapacity;
            @display("p=300,100,column,90");
        }
        scheduler: <default("AirtimeDrrScheduler")> like IPacketScheduler {
            @display("p=500,100");
        }
    connections:
        in --> { @display("m=w"); } --> classifier.in;
        for i=0..numQueues-1 {
            classifier.out++ --> queue[i].in;
            queue[i].out --> scheduler.in++;
        }
        scheduler.out --> { @display("m=e"); } --> out;
}
