// CA2 phase 2 (provided; do not modify).
#include "RateTableSelection.h"

#include "inet/networklayer/common/NetworkInterface.h"

namespace ca2 {

Define_Module(RateTableSelection);

void RateTableSelection::buildTable()
{
    // every wlan interface in the network that has an explicit bitrate
    cModule *network = getSimulation()->getSystemModule();
    for (cModule::SubmoduleIterator node(network); !node.end(); ++node) {
        for (cModule::SubmoduleIterator it(*node); !it.end(); ++it) {
            auto wlan = dynamic_cast<NetworkInterface *>(*it);
            if (wlan == nullptr || !wlan->hasPar("bitrate") || wlan->getMacAddress().isUnspecified())
                continue;
            double bitrate = wlan->par("bitrate");
            if (bitrate > 0)
                table[wlan->getMacAddress()] = modeSet->getMode(bps(bitrate));
        }
    }
    tableBuilt = true;
}

const physicallayer::IIeee80211Mode *RateTableSelection::getModeFor(const MacAddress& receiver)
{
    if (!tableBuilt)
        buildTable();
    auto it = table.find(receiver);
    return it != table.end() ? it->second : nullptr;
}

const physicallayer::IIeee80211Mode *RateTableSelection::computeDataOrMgmtFrameMode(const Ptr<const ieee80211::Ieee80211DataOrMgmtHeader>& header)
{
    if (dynamicPtrCast<const ieee80211::Ieee80211DataHeader>(header) && !header->getReceiverAddress().isMulticast()) {
        if (auto mode = getModeFor(header->getReceiverAddress()))
            return mode;
    }
    return RateSelection::computeDataOrMgmtFrameMode(header);
}

} // namespace ca2
