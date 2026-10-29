/*
 * QDL-FSO quantum channel validation (ground station <-> moving platform)
 *
 * Alice (ground station, z = 1.5 m) and Bob (moving at 4 km/s along z, starting at z = 80 m)
 * are connected by an FSO quantum link (1550 nm, 10 cm beam waist, 30 cm Rx aperture,
 * 1 µrad jitter).
 * Bob sends one qubit to Alice every 2 s for ~163 s, while the link distance grows, exercising the propagation-loss,
 * turbulence (Cn2/Rytov), beam-wander and pointing-error models.
 * A building spanning 400-700 km in altitude is included to test the NLOS case.
 * Each send/receive is logged to verify end-to-end channel behavior.
 * Fixed RNG seed (42) for reproducibility.
 */

#include "ns3/buildings-module.h"
#include "ns3/channel-condition-model.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/q2ns-analysis.h"
#include "ns3/q2ns-netcontroller.h"
#include "ns3/q2ns-qmap.h"
#include "ns3/q2ns-qnode.h"
#include "ns3/q2ns-qstate.h"
#include "ns3/q2ns-qubit.h"
#include "ns3/qdl-fso-helper.h"
#include "ns3/qdl-fso-optical-terminal.h"
#include "ns3/qdl-fso-q2ns-qchannel.h"
#include "ns3/simulator.h"

#include <iostream>
#include <random>
#include <string>

using namespace ns3;
using namespace q2ns;

namespace
{

const double sim_end_time = 162 + 1;

// Send a qubit and schedule the next transmission
void
ScheduleSend(Ptr<QNode> A, Ptr<QNode> B)
{
	auto q = A->CreateQubit();
	auto ok = A->Send(q, B->GetId());
	std::cout << Simulator::Now().GetMilliSeconds() << " [" << Names::FindName(A) << "][SEND][quantum-ch] qubit-"
			  << q->GetQubitId() << " " << Names::FindName(A) << "->" << Names::FindName(B) << ": "
			  << (ok ? "ok" : "failed") << std::endl;

	std::cout << "###############################################################" << std::endl;

	Simulator::Schedule(Seconds(2.0), [A, B]() { ScheduleSend(A, B); });
}
} // namespace

int
main(int argc, char* argv[])
{
	LogComponentEnable("QdlFsoPropagationLossModel", LOG_LEVEL_DEBUG);
	// LogComponentEnable("QdlFsoQChannel", LOG_LEVEL_DEBUG);
	// LogComponentEnable("QdlFsoCn2Model", LOG_LEVEL_INFO);
	// LogComponentEnable("QdlFsoAtmosphericModel", LOG_LEVEL_INFO);
	// LogComponentEnable("QdlFsoRytovModel", LOG_LEVEL_INFO);
	// LogComponentEnable("QdlFsoBeamWanderModel", LOG_LEVEL_INFO);
	// LogComponentEnable("QdlFsoBeamWanderModel", LOG_LEVEL_DEBUG);
	// LogComponentEnable("QdlFsoGeometricPointingErrorModel", LOG_LEVEL_INFO);
	// LogComponentEnable("QdlFsoGeometricPointingErrorModel", LOG_LEVEL_DEBUG);
	// LogComponentEnable("QdlFsoScintillationModel", LOG_LEVEL_INFO);
	// LogComponentEnable("BuildingsChannelConditionModel", LOG_LEVEL_DEBUG);
	// LogComponentEnable("QdlFsoHelper", LOG_LEVEL_DEBUG);

	QdlFsoPropagationLossModel::SetAltitudeTolerance(100.0);

	// To get reproducible results
	RngSeedManager::SetSeed(42);
	RngSeedManager::SetRun(1);

	NetController net;
	net.SetQStateBackend(QStateBackend::Ket);

	auto Alice = net.CreateNode("Alice"); // ground station, fixed
	auto Bob = net.CreateNode("Bob");	  // satellite / UAV, moving

	// ---------------------------------------------------------
	// Set up Mobility Models
	// ---------------------------------------------------------
	// ---- Alice (Fixed Ground Station) ----
	MobilityHelper mobilityAlice;
	mobilityAlice.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobilityAlice.Install(Alice);
	Ptr<MobilityModel> aliceMobility = Alice->GetObject<MobilityModel>();
	aliceMobility->SetPosition(Vector(0.0, 0.0, 1.5));

	// ---- Bob (Moving UAV / Satellite) ----
	MobilityHelper mobilityBob;
	mobilityBob.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
	mobilityBob.Install(Bob);
	Ptr<ConstantVelocityMobilityModel> bobMobility = Bob->GetObject<ConstantVelocityMobilityModel>();
	// Example: Start Bob at 80m altitude (z = 80m) directly above Alice
	bobMobility->SetPosition(Vector(0.0, 0.0, 80.0));
	// Example: Bob moves at 4000 m/s along the Z-axis
	bobMobility->SetVelocity(Vector(0.0, 0.0, 4000.0));

	// Construct a building to check NLOS case
	std::vector<Ptr<Building>> buildingVector;
	Ptr<Building> building = CreateObject<Building>();
	building->SetBoundaries(Box(-10, 10, -10, 10, 400000, 700000));
	buildingVector.push_back(building);

	// Add bulding info to the mobility model
	BuildingsHelper::Install(Alice);
	BuildingsHelper::Install(Bob);

	// Instantiate Alice Terminal and configure parameters
	Ptr<QdlFsoOpticalTerminal> AliceTerminal = CreateObject<QdlFsoOpticalTerminal>();
	AliceTerminal->SetWavelength(1550e-9);		// 1550 nm
	AliceTerminal->SetTxBeamWaist(0.10);		// 10 cm radius
	AliceTerminal->SetPointingJitter(1e-6);		// 1 µrad jitter
	AliceTerminal->SetRxApertureDiameter(0.3);	// 30 cm diameter
	AliceTerminal->SetTrackingEfficiency(0.95); // 95% tracking efficiency
	AliceTerminal->SetHardwareEfficiency(0.9);	// 90% efficiency

	// Instantiate Bob Terminal and configure parameters
	Ptr<QdlFsoOpticalTerminal> BobTerminal = CreateObject<QdlFsoOpticalTerminal>();
	BobTerminal->SetWavelength(1550e-9);	  // 1550 nm
	BobTerminal->SetTxBeamWaist(0.10);		  // 10 cm radius
	BobTerminal->SetPointingJitter(1e-6);	  // 1 µrad jitter
	BobTerminal->SetRxApertureDiameter(0.3);  // 30 cm diameter
	BobTerminal->SetTrackingEfficiency(0.95); // 95% tracking efficiency
	BobTerminal->SetHardwareEfficiency(0.9);  // 90% efficiency

	// Install the FSO quantum channel between Alice and Bob
	ns3::Ptr<QdlFsoHelper> qdlFsoHelper = CreateObject<QdlFsoHelper>();
	auto ch = qdlFsoHelper->InstallQdlFsoQuantumLink(Alice, Bob, AliceTerminal, BobTerminal);

	// Add qubit received cakllback to log successful reception
	// Done for both Alice and Bob in case you want to change txer and rxer
	Alice->SetRecvCallback([](std::shared_ptr<Qubit> q) {
		std::cout << Simulator::Now().GetMilliSeconds() << "ms [Alice][RECV][qdl-fso-quantum-ch]: qubit-"
				  << q->GetQubitId() << ": yes " << std::endl;

		std::cout << "###############################################################" << std::endl;
	});

	Bob->SetRecvCallback([](std::shared_ptr<Qubit> q) {
		std::cout << Simulator::Now().GetMilliSeconds() << "ms [Bob][RECV][qdl-fso-quantum-ch]: qubit-"
				  << q->GetQubitId() << ": yes " << std::endl;

		std::cout << "###############################################################" << std::endl;
	});

	// Periodic Send
	Simulator::Schedule(Seconds(0.0), [Alice, Bob]() { ScheduleSend(Bob, Alice); });

	Simulator::Stop(Seconds(sim_end_time));
	Simulator::Run();
	Simulator::Destroy();

	std::cout << "qdl-fso-qchannel validation run completed successfully." << std::endl;

	return 0;
}
