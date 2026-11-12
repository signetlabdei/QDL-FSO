#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/qdl-fso-propagation-loss-model.h"
#include "ns3/qdl-fso-optical-terminal.h"
#include "ns3/qdl-fso-helper.h"
#include "ns3/buildings-module.h"

using namespace ns3;

int main() {
// Nodes: a 20 m high transmitter and a receiver 1 km away at 1.5 m
Ptr<Node> tx_node = CreateObject<Node>();
Ptr<Node> rx_node = CreateObject<Node>();
Ptr<ConstantPositionMobilityModel> tx_mob = CreateObject<ConstantPositionMobilityModel>();
Ptr<ConstantPositionMobilityModel> rx_mob = CreateObject<ConstantPositionMobilityModel>();
tx_mob->SetPosition(Vector(0.0, 0.0, 20.0));
rx_mob->SetPosition(Vector(1000.0, 0.0, 1.5));
tx_node->AggregateObject(tx_mob);
rx_node->AggregateObject(rx_mob);
// Add bulding info to the mobility model (this is purely a "coding" necessity, it is not something logical)
BuildingsHelper::Install(tx_node);
BuildingsHelper::Install(rx_node);

// Terminals
Ptr<QdlFsoOpticalTerminal> tx_term = CreateObject<QdlFsoOpticalTerminal>();
tx_term->SetWavelength(1550e-9);		// 1550 nm
tx_term->SetTxBeamWaist(0.10);		// 10 cm radius
tx_term->SetPointingJitter(1.0e-6);	// 1 μrad
tx_term->SetRxApertureDiameter(0.3);	// 30 cm diameter
tx_term->SetTrackingEfficiency(0.85); // 100% tracking efficiency
tx_term->SetHardwareEfficiency(1.0);	// 100% hardware efficiency
tx_term->SetTxPower(30.0); // 1 W

Ptr<QdlFsoOpticalTerminal> rx_term = CreateObject<QdlFsoOpticalTerminal>();
rx_term->SetWavelength(1550e-9);	  // 1550 nm
rx_term->SetTxBeamWaist(0.10);		  // 10 cm radius
rx_term->SetPointingJitter(1.0e-6);	  // 1 μrad
rx_term->SetRxApertureDiameter(0.3);  // 30 cm diameter
rx_term->SetTrackingEfficiency(0.85); // 100% tracking efficiency
rx_term->SetHardwareEfficiency(1.0);  // 100% hardware efficiency

// FSO channel: configure atmospheric, turbulence, pointing, and scintillation models
// (or rely on the defaults) and attach the optical terminals of the two endpoints
ns3::Ptr<QdlFsoHelper> helper = CreateObject<QdlFsoHelper>();
Ptr<QdlFsoPropagationLossModel> loss = helper->CreateDefaultPropagationLossModel();
loss->SetAttribute("IsQuantum", BooleanValue(false));
loss->SetTerminals(tx_term, tx_mob, rx_term, rx_mob);


// Received power over 1000 realizations
const uint32_t nRuns = 1000;
double sumPower = 0.0;
double sumEta = 0.0;
for (uint32_t i = 0; i < nRuns; ++i)
{
    double rxPowerDbm = loss->CalcRxPower(tx_term->GetTxPower(), tx_mob, rx_mob);
	sumPower += rxPowerDbm;
    sumEta += std::pow(10.0, (rxPowerDbm - tx_term->GetTxPower()) / 10.0); // eta_ch of this realization
}
std::cout << "Mean rx power: " << sumPower / nRuns << std::endl;
std::cout << "Mean transmissivity: " << sumEta / nRuns << std::endl;
}
