// CA2 phase 2 (provided; do not modify).
#ifndef __CA2_RATETABLESELECTION_H
#define __CA2_RATETABLESELECTION_H

#include <map>

#include "inet/linklayer/ieee80211/mac/rateselection/RateSelection.h"

namespace ca2 {

using namespace inet;

/**
 * Rate selection for the AP: every unicast data frame goes out at the bitrate
 * its receiver uses itself (the receiver's wlan "bitrate" parameter). A link
 * is as good in both directions, so a slow station is slow both ways.
 */
class RateTableSelection : public ieee80211::RateSelection
{
  protected:
    std::map<MacAddress, const physicallayer::IIeee80211Mode *> table;
    bool tableBuilt = false;

    void buildTable();
    virtual const physicallayer::IIeee80211Mode *computeDataOrMgmtFrameMode(const Ptr<const ieee80211::Ieee80211DataOrMgmtHeader>& header) override;

  public:
    /** Mode of unicast data frames sent to this receiver. */
    const physicallayer::IIeee80211Mode *getModeFor(const MacAddress& receiver);
};

} // namespace ca2

#endif
