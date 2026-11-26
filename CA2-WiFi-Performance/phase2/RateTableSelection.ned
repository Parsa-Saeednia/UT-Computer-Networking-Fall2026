// CA2 phase 2 (provided; do not modify).
package phase2;

import inet.linklayer.ieee80211.mac.rateselection.RateSelection;

// Sends every unicast data frame at its receiver's own bitrate.
simple RateTableSelection extends RateSelection
{
    @class(ca2::RateTableSelection);
}
