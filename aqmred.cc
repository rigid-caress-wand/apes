#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/traffic-control-module.h"

using namespace ns3;

int main (int argc, char *argv[]) {
    NodeContainer nodes;
    nodes.Create (2);

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));

    NetDeviceContainer devices = p2p.Install (nodes);

    TrafficControlHelper tch;
    tch.Uninstall (devices);
    tch.SetRootQueueDisc ("ns3::RedQueueDisc", 
                          "MinTh", DoubleValue (5), 
                          "MaxTh", DoubleValue (15),
                          "LinkBandwidth", StringValue ("10Mbps"),
                          "LinkDelay", StringValue ("2ms"));
    tch.Install (devices);

    InternetStackHelper stack;
    stack.Install (nodes);

    Ipv4AddressHelper address;
    address.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign (devices);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    uint16_t port = 9;
    OnOffHelper source ("ns3::UdpSocketFactory", InetSocketAddress (interfaces.GetAddress (1), port));
    source.SetAttribute ("DataRate", StringValue ("5Mbps"));
    source.SetAttribute ("PacketSize", UintegerValue (1024));

    ApplicationContainer sourceApps = source.Install (nodes.Get (0));
    sourceApps.Start (Seconds (1.0));
    sourceApps.Stop (Seconds (10.0));

    PacketSinkHelper sink ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
    ApplicationContainer sinkApps = sink.Install (nodes.Get (1));
    sinkApps.Start (Seconds (0.0));
    sinkApps.Stop (Seconds (10.0));

    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

    Simulator::Stop (Seconds (10.0));
    Simulator::Run ();

    monitor->CheckForLostPackets ();
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();

    for (auto it = stats.begin (); it != stats.end (); ++it) {
        std::cout << "Flow " << it->first << " Lost: " << it->second.lostPackets << "\n";
        std::cout << "Rx Bytes: " << it->second.rxBytes << "\n";
        std::cout << "Delay: " << it->second.delaySum.GetSeconds() / it->second.rxPackets << "\n";
    }

    Simulator::Destroy ();
    return 0;
}
