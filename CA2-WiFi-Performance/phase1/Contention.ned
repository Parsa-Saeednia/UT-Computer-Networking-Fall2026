package phase1;

// CA2 phase 1: n saturated stations send to one AP (802.11 DCF).
// Stations sit on a circle around the AP, so all hear each other and every
// frame reaches the AP with the same power (no capture effect).

import inet.applications.ethernet.EtherAppClient;
import inet.linklayer.ieee80211.Ieee80211Interface;
import inet.mobility.static.StationaryMobility;
import inet.networklayer.common.InterfaceTable;
import inet.physicallayer.wireless.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;
import inet.queueing.sink.PassivePacketSink;
import inet.visualizer.contract.IIntegratedVisualizer;

module Station
{
    parameters:
        @display("i=device/wifilaptop");
        @networkNode();
        *.interfaceTableModule = "^.interfaceTable";
        wlan.radio.antenna.mobilityModule = default("^.^.^.mobility");
    gates:
        input radioIn @directIn;
    submodules:
        cli: EtherAppClient {
            interface = "wlan";
            @display("p=300,100");
        }
        wlan: Ieee80211Interface {
            mgmt.typename = "Ieee80211MgmtStaSimplified";
            agent.typename = "";
            @display("p=250,200;q=queue");
        }
        interfaceTable: InterfaceTable {
            @display("p=100,100;is=s");
        }
        mobility: StationaryMobility {
            @display("p=100,200;is=s");
        }
    connections allowunconnected:
        wlan.radioIn <-- radioIn;
        cli.out --> wlan.upperLayerIn;
}

module AccessPoint
{
    parameters:
        @display("i=device/accesspoint");
        @networkNode();
        *.interfaceTableModule = "^.interfaceTable";
        wlan.radio.antenna.mobilityModule = default("^.^.^.mobility");
    gates:
        input radioIn @directIn;
    submodules:
        sink: PassivePacketSink {
            @display("p=300,100");
        }
        wlan: Ieee80211Interface {
            llc.typename = "Ieee80211Portal";
            mgmt.typename = "Ieee80211MgmtApSimplified";
            agent.typename = "";
            @display("p=250,200;q=queue");
        }
        interfaceTable: InterfaceTable {
            @display("p=100,100;is=s");
        }
        mobility: StationaryMobility {
            @display("p=100,200;is=s");
        }
    connections allowunconnected:
        radioIn --> wlan.radioIn;
        wlan.upperLayerOut --> sink.in;
}

network Contention
{
    parameters:
        int n;
        double r @unit(m) = default(10m);
    submodules:
        visualizer: <default(firstAvailableOrEmpty("IntegratedCanvasVisualizer"))> like IIntegratedVisualizer if typename != "" {
            @display("p=40,40;is=s");
        }
        radioMedium: Ieee80211ScalarRadioMedium {
            @display("p=40,100;is=s");
        }
        ap: AccessPoint {
            @display("p=200,200");
        }
        host[n]: Station {
            // stations on a circle of radius r around the AP
            mobility.initialX = default(200m + parent.r * cos(2 * 3.14159265 * index / parent.n));
            mobility.initialY = default(200m + parent.r * sin(2 * 3.14159265 * index / parent.n));
        }
}
