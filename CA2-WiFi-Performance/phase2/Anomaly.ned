package phase2;

// CA2 phase 2: a wired server behind an AP, and n Wi-Fi stations on a circle
// around the AP. Traffic goes either up (stations -> server) or down
// (server -> stations); see omnetpp.ini.

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.node.ethernet.Eth1G;
import inet.node.inet.StandardHost;
import inet.node.inet.WirelessHost;
import inet.node.wireless.AccessPoint;
import inet.physicallayer.wireless.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;
import inet.visualizer.contract.IIntegratedVisualizer;

network Anomaly
{
    parameters:
        int n;
        double r @unit(m) = default(10m);
        @display("bgb=400,400");
    submodules:
        visualizer: <default(firstAvailableOrEmpty("IntegratedCanvasVisualizer"))> like IIntegratedVisualizer if typename != "" {
            @display("p=40,40;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=40,100;is=s");
        }
        radioMedium: Ieee80211ScalarRadioMedium {
            @display("p=40,160;is=s");
        }
        server: StandardHost {
            @display("p=60,300");
        }
        ap: AccessPoint {
            @display("p=200,200");
        }
        host[n]: WirelessHost {
            mobility.initialX = default(200m + parent.r * cos(2 * 3.14159265 * index / parent.n));
            mobility.initialY = default(200m + parent.r * sin(2 * 3.14159265 * index / parent.n));
        }
    connections:
        server.ethg++ <--> Eth1G <--> ap.ethg++;
}
