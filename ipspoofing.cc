#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

// SPOOFING DETECTION FUNCTION
void IngressFilter (Ptr<const Packet> p, Ptr<Ipv4> ipv4, uint32_t interface) {
    Ipv4Header ip;
    p->PeekHeader (ip);
    Ipv4Address src = ip.GetSource ();
    Ipv4InterfaceAddress ifAddr = ipv4->GetAddress (interface, 0);
    
    // Check if Source IP matches the Interface Subnet
    if (src.CombineMask (ifAddr.GetMask ()) != ifAddr.GetLocal ().CombineMask (ifAddr.GetMask ())) {
        NS_LOG_UNCOND ("Spoofed packet detected from " << src);
    }
}

int main (int argc, char *argv[]) {
    // 1. Create Nodes
    NodeContainer nodes;
    nodes.Create (2);

    // 2. Configure Link
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
    NetDeviceContainer devices = p2p.Install (nodes);

    // 3. Install Internet Stack
    InternetStackHelper stack;
    stack.Install (nodes);

    // 4. Assign IP Addresses
    Ipv4AddressHelper address;
    address.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign (devices);

    // 5. Populate Routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    // 6. CONNECT INGRESS FILTER (The Security Part)
    Ptr<Ipv4> ipv4 = nodes.Get(1)->GetObject<Ipv4>();
    ipv4->TraceConnectWithoutContext("Rx", MakeCallback(&IngressFilter));

    // 7. Applications (UDP)
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

    // 8. Flow Monitor
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

    // 9. Run
    Simulator::Stop (Seconds (10.0));
    Simulator::Run ();

    // 10. Stats
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
