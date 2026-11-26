package phase3;

// CA2 phase 3: a station walks from ap1 to ap2 while it streams voice-like
// frames to whichever AP it is associated with. The APs are on different
// channels; the station's agent (Ieee80211AgentSta) decides when to scan and
// which AP to join. With survey = true a measuring device walks the same path
// instead: it has one receive-only radio tuned to each AP's channel.
// The path (start, speed, length) is the turtle script in ../params.ini.

import inet.applications.ethernet.EtherAppClient;
import inet.linklayer.ieee80211.Ieee80211Interface;
import inet.mobility.single.TurtleMobility;
import inet.mobility.static.StationaryMobility;
import inet.networklayer.common.InterfaceTable;
import inet.physicallayer.wireless.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;
import inet.queueing.sink.PassivePacketSink;
import inet.visualizer.contract.IIntegratedVisualizer;

module MobileStation
{
    parameters:
        @display("i=device/cellphone");
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
        wlan: Ieee80211Interface {   // full STA management: Ieee80211MgmtSta and Ieee80211AgentSta
            @display("p=250,200;q=queue");
        }
        interfaceTable: InterfaceTable {
            @display("p=100,100;is=s");
        }
        mobility: TurtleMobility {
            @display("p=100,200;is=s");
        }
    connections allowunconnected:
        wlan.radioIn <-- radioIn;
        cli.out --> wlan.upperLayerIn;
}

module SurveyMeter
{
    parameters:
        @display("i=device/pocketpc");
        @networkNode();
        *.interfaceTableModule = "^.interfaceTable";
        wlan[*].radio.antenna.mobilityModule = default("^.^.^.mobility");
    gates:
        input radioIn[2] @directIn;
    submodules:
        wlan[2]: Ieee80211Interface {   // receive only: never scans, never transmits
            mgmt.typename = "Ieee80211MgmtStaSimplified";
            agent.typename = "";
            @display("p=250,200,row,100;q=queue");
        }
        interfaceTable: InterfaceTable {
            @display("p=100,100;is=s");
        }
        mobility: TurtleMobility {
            @display("p=100,200;is=s");
        }
    connections allowunconnected:
        for i=0..1 {
            wlan[i].radioIn <-- radioIn[i];
        }
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
            mgmt.typename = "Ieee80211MgmtAp";   // beacons, probe responses, authentication, association
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

network Roaming
{
    parameters:
        double D @unit(m);               // distance between the APs
        bool survey = default(false);    // true: the SurveyMeter walks instead of the MobileStation
        @display("bgb=600,200");
    submodules:
        visualizer: <default(firstAvailableOrEmpty("IntegratedCanvasVisualizer"))> like IIntegratedVisualizer if typename != "" {
            @display("p=40,40;is=s");
        }
        radioMedium: Ieee80211ScalarRadioMedium {
            @display("p=110,40;is=s");
        }
        ap1: AccessPoint {
            mobility.initialX = 50m;
            mobility.initialY = 100m;
        }
        ap2: AccessPoint {
            mobility.initialX = 50m + parent.D;
            mobility.initialY = 100m;
        }
        host: MobileStation if !survey;
        meter: SurveyMeter if survey;
}
