#include "ns3/buildings-module.h"
#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/q2ns-analysis.h"
#include "ns3/q2ns-netcontroller.h"
#include "ns3/q2ns-qnode.h"
#include "ns3/q2ns-qstate.h"
#include "ns3/q2ns-qubit.h"
#include "ns3/qdl-fso-helper.h"
#include "ns3/qdl-fso-optical-terminal.h"
#include "ns3/qdl-fso-propagation-loss-model.h"

#include <iostream>

using namespace ns3;
using namespace q2ns;

namespace
{
uint32_t nSent = 0;
uint32_t nReceived = 0;
double sumEta = 0.0;
double sumFidelity = 0.0;

// QdlFsoQChannel TxTrace: (qubit, transmissivity, dephasing prob, src node, dst node)
void
OnTx(std::shared_ptr<Qubit> q, double eta, double dephProb, Ptr<const Node> src, Ptr<const Node> dst)
{
    sumEta += eta;
}
} // namespace

int
main()
{
    RngSeedManager::SetSeed(42);
    RngSeedManager::SetRun(1);

    // Q2NS controller and quantum nodes
    NetController net;
    net.SetQStateBackend(QStateBackend::Ket);
    auto tx_node = net.CreateNode("Alice");
    auto rx_node = net.CreateNode("Bob");

    // Mobility: a 20 m high transmitter and a receiver 1 km away at 1.5 m
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(tx_node);
    mobility.Install(rx_node);
    Ptr<MobilityModel> tx_mob = tx_node->GetObject<MobilityModel>();
    Ptr<MobilityModel> rx_mob = rx_node->GetObject<MobilityModel>();
    tx_mob->SetPosition(Vector(0.0, 0.0, 20.0));
    rx_mob->SetPosition(Vector(1000.0, 0.0, 1.5));
    // Add building info to the mobility model (this is purely a "coding" necessity, it is not something logical)
    BuildingsHelper::Install(tx_node);
    BuildingsHelper::Install(rx_node);

    // Terminals
    Ptr<QdlFsoOpticalTerminal> tx_term = CreateObject<QdlFsoOpticalTerminal>();
    tx_term->SetWavelength(1550e-9);      // 1550 nm
    tx_term->SetTxBeamWaist(0.10);        // 10 cm radius
    tx_term->SetPointingJitter(1.0e-6);   // 1 μrad
    tx_term->SetRxApertureDiameter(0.3);  // 30 cm diameter
    tx_term->SetTrackingEfficiency(0.85); // 85% tracking efficiency
    tx_term->SetHardwareEfficiency(1.0);  // 100% hardware efficiency

    Ptr<QdlFsoOpticalTerminal> rx_term = CreateObject<QdlFsoOpticalTerminal>();
    rx_term->SetWavelength(1550e-9);
    rx_term->SetTxBeamWaist(0.10);
    rx_term->SetPointingJitter(1.0e-6);
    rx_term->SetRxApertureDiameter(0.3);
    rx_term->SetTrackingEfficiency(0.85);
    rx_term->SetHardwareEfficiency(1.0);

    // FSO quantum channel: default propagation loss model is created and installed through the helper
    Ptr<QdlFsoHelper> helper = CreateObject<QdlFsoHelper>();
    auto ch = helper->InstallQdlFsoQuantumLink(tx_node, rx_node, tx_term, rx_term);

    // Apply turbulence-induced dephasing to the flying qubits, in addition to loss
    ch->SetAttribute("EnableDephasing", BooleanValue(true));
    ch->TraceConnectWithoutContext("QubitTxedTrace", MakeCallback(&OnTx));

    // Reference Bell pair, used to compute the fidelity of the received state
    auto refNode = net.CreateNode("ReferenceNode");
    auto [qRefA, qRefB] = refNode->CreateBellPair();
    auto sRef = net.GetState(qRefA);

    // Bob's receive callback: compute the fidelity of the received state
    rx_node->SetRecvCallback([&net, sRef](std::shared_ptr<Qubit> q) {
        auto st = net.GetState(q);
        if (st)
        {
            sumFidelity += q2ns::analysis::Fidelity(*sRef, *st);
        }
        ++nReceived;
    });

    // Alice generates and sends one Bell pair every 10 ms (100 pairs/s), keeping one qubit
    const uint32_t nRuns = 1000;
    for (uint32_t i = 0; i < nRuns; ++i)
    {
        Simulator::Schedule(MilliSeconds(10 * i), [tx_node, rx_node]() {
            auto [qA, qB] = tx_node->CreateBellPair();
            tx_node->Send(qB, rx_node->GetId());
            ++nSent;
        });
    }

    Simulator::Stop(Seconds(11.0));
    Simulator::Run();
    Simulator::Destroy();

    std::cout << "Mean transmissivity:  " << sumEta / nSent << std::endl;
    std::cout << "Qubit loss:           " << 100.0 * (nSent - nReceived) / nSent << " %" << std::endl;
    std::cout << "Mean fidelity (RX):   " << (nReceived ? sumFidelity / nReceived : 0.0) << std::endl;
    return 0;
}