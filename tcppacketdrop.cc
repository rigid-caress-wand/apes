#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

int main (int argc, char *argv[]) {
    NodeContainer nodes;
    nodes.Create (4);

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
    p2p.SetQueue ("ns3::DropTailQueue<Packet>", "MaxSize", QueueSizeValue (QueueSize ("5p")));

    NetDeviceContainer d01 = p2p.Install (nodes.Get (0), nodes.Get (1));
    NetDeviceContainer d12 = p2p.Install (nodes.Get (1), nodes.Get (2));
    NetDeviceContainer d23 = p2p.Install (nodes.Get (2), nodes.Get (3));

    InternetStackHelper stack;
    stack.Install (nodes);

    Ipv4AddressHelper address;
    address.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i01 = address.Assign (d01);

    address.SetBase ("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer i12 = address.Assign (d12);

    address.SetBase ("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer i23 = address.Assign (d23);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    uint16_t port = 9;
    BulkSendHelper source ("ns3::TcpSocketFactory", InetSocketAddress (i23.GetAddress (1), port));
    source.SetAttribute ("MaxBytes", UintegerValue (0));
    ApplicationContainer sourceApps = source.Install (nodes.Get (0));
    sourceApps.Start (Seconds (1.0));
    sourceApps.Stop (Seconds (10.0));

    PacketSinkHelper sink ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
    ApplicationContainer sinkApps = sink.Install (nodes.Get (3));
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
