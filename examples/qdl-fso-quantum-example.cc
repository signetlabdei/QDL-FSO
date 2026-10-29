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
#include "ns3/qdl-fso-atmospheric-model.h"
#include "ns3/qdl-fso-beam-wander-model.h"
#include "ns3/qdl-fso-cn2-model.h"
#include "ns3/qdl-fso-geometric-pointing-error-model.h"
#include "ns3/qdl-fso-helper.h"
#include "ns3/qdl-fso-optical-terminal.h"
#include "ns3/qdl-fso-rytov-model.h"
#include "ns3/qdl-fso-scintillation-model.h"
#include "ns3/simulator.h"

#include <cmath>
#include <iostream>
#include <random>
#include <string>

using namespace ns3;
using namespace q2ns;

namespace
{

const double sim_end_time = 1000.01;

// Quantum transmission rate
const double qubit_rate = 100; // qubits/s
const double qubit_interval = 1.0 / qubit_rate;

// Bob circular motion parameter to complete a circular trajectory in exactly sim_end_time seconds
const double circle_radius = 250.0;									 // m
const double circle_speed = 2 * M_PI * circle_radius / sim_end_time; // m/s
const double circle_angular_velocity = circle_speed / circle_radius;

// Global variable used for logging
std::string current_atmospheric_condition = "clear_air";

std::ofstream out_file;

// Schedule a send event from node A to node B
void
ScheduleSend(Ptr<QNode> A, Ptr<QNode> B)
{
	// Send only one qubit
	// auto q = A->CreateQubit();
	// A->Apply(q2ns::gates::H(), {q});
	// auto ok = A->Send(q, B->GetId());

	// Send an entangled pair
	auto [qA, qB] = A->CreateBellPair();
	A->Send(qB, B->GetId());
	// auto ok = A->Send(qB, B->GetId());

	// Just for debugging
	// std::cout << Simulator::Now().GetMilliSeconds() << " [" << Names::FindName(A)
	//           << "][SEND][quantum-ch] qubit-" << q->GetQubitId()
	//           << " A->B: " << (ok ? "ok" : "failed") << std::endl;
	// std::cout << "###############################################################" << std::endl;

	Simulator::Schedule(Seconds(qubit_interval), [A, B]() { ScheduleSend(A, B); });
}

// Since there is no circular mobility model available in vanilla ns3, we update the position manually
// Update is every 10 ms
void
UpdateBobPosition(Ptr<MobilityModel> bobMobility)
{
	const double t = Simulator::Now().GetSeconds();

	const double theta = circle_angular_velocity * t;

	const double x = circle_radius * std::cos(theta);
	const double y = circle_radius * std::sin(theta);
	const double z = 1.5;

	bobMobility->SetPosition(Vector(x, y, z));

	Simulator::Schedule(MilliSeconds(10), [bobMobility]() { UpdateBobPosition(bobMobility); });
}

// Write data to the out_file
void
WriteRow(const std::string event,
		 uint64_t qid,
		 Ptr<const Node> node,
		 double eta,
		 double fidelity,
		 double distance = std::nan(""),
		 double tx_fidelity = std::nan(""))
{
	const Vector p = node->GetObject<ns3::MobilityModel>()->GetPosition(); // receiver position right now
	out_file << std::fixed << Simulator::Now().GetSeconds() << "," << qid << "," << node->GetId() << "," << event
			 << ",";

	if (!std::isnan(fidelity))
	{
		out_file << fidelity;
	}

	out_file << ",";

	if (!std::isnan(eta))
	{
		out_file << eta;
	}
	out_file << "," << current_atmospheric_condition << "," << p.x << "," << p.y << "," << p.z << ",";

	if (!std::isnan(distance))
	{
		out_file << distance;
	}

	out_file << ",";

	if (!std::isnan(tx_fidelity))
	{
		out_file << tx_fidelity;
	}
	out_file << "\n";
}

// Utility function to write TX events to out_file
// QdlFsoQChannel TxTrace: (qubit, transmissivity, dephasing pron, src node, dst node)
void
OnTx(std::shared_ptr<Qubit> q, double eta, double dephProb, Ptr<const Node> src, Ptr<const Node> dst)
{
	double distance = src->GetObject<ns3::MobilityModel>()->GetDistanceFrom(dst->GetObject<ns3::MobilityModel>());
	WriteRow("TX", q->GetQubitId(), src, eta, std::nan(""), distance, 1 - dephProb);
}

// Utility function to write RX events to out_file
// QNetworker RxTrace: (qubit, id of the receiving node) -- fired before the user recv callback
void
OnRx(std::shared_ptr<q2ns::QState> sRef, NetController* net, std::shared_ptr<Qubit> q, Ptr<QNode> dst)
{
	auto st = net->GetState(q);
	if (!st)
	{
		return;
	}

	double fidelity = q2ns::analysis::Fidelity(*sRef, *st);
	WriteRow("RX", q->GetQubitId(), dst, std::nan(""), fidelity);
}

// Utility function to write LOST events to out_file
// QNetworker LostTrace: (qubit, id of the node where the loss happened)
void
OnLost(std::shared_ptr<Qubit> q, Ptr<QNode> dst)
{
	WriteRow("LOST", q->GetQubitId(), dst, std::nan(""), std::nan(""));
}

// Utility struct to change the current atmospheric params during simulations
struct AtmosphericScenario
{
	double visibilityKm;
	double rainRate;
	double cn2;
	double windSpeed;
	double pointingJitter;
	double trackingEfficiency;
};

// Function to apply the parameters specified by a AtmosphericScenario object to the various models
void
ApplyAtmosphericScenario(const AtmosphericScenario& scenario,
						 Ptr<QdlFsoAtmosphericModel> atm,
						 Ptr<QdlFsoCn2Model> cn2,
						 Ptr<QdlFsoOpticalTerminal> a,
						 Ptr<QdlFsoOpticalTerminal> b)
{
	atm->SetAttribute("VisibilityKm", DoubleValue(scenario.visibilityKm));
	atm->SetAttribute("RainRateMmPerHour", DoubleValue(scenario.rainRate));

	cn2->SetCn20(scenario.cn2);
	cn2->SetVWind(scenario.windSpeed);

	a->SetPointingJitter(scenario.pointingJitter);
	b->SetTrackingEfficiency(scenario.trackingEfficiency);
}

} // namespace

int
main(int argc, char* argv[])
{
	// -------------------------------------------------------------------------------------
	// LOGGING AND SETUP FOR RESULTS COLLECTIONS
	// -------------------------------------------------------------------------------------

	// Decide what to log. This is for debug only
	// LogComponentEnable("QdlFsoPropagationLossModel", LOG_LEVEL_DEBUG);
	// LogComponentEnable("QdlFsoQChannel", LOG_LEVEL_DEBUG);
	// LogComponentEnable("QdlFsoCn2Model", LOG_LEVEL_INFO);
	// LogComponentEnable("QdlFsoAtmosphericModel", LOG_LEVEL_INFO);
	// LogComponentEnable("QdlFsoRytovModel", LOG_LEVEL_INFO);
	// LogComponentEnable("QdlFsoBeamWanderModel", LOG_LEVEL_INFO);
	// LogComponentEnable("QdlFsoGeometricPointingErrorModel", LOG_LEVEL_INFO);
	// LogComponentEnable("QdlFsoScintillationModel", LOG_LEVEL_INFO);
	// LogComponentEnable("BuildingsChannelConditionModel", LOG_LEVEL_DEBUG);
	// LogComponentEnable("QdlFsoHelper", LOG_LEVEL_DEBUG);

	// Txer and Rxer establish a horizontal link only of their difference in altitude is less than 100 m
	QdlFsoPropagationLossModel::SetAltitudeTolerance(100.0);

	// For reproducibility
	RngSeedManager::SetSeed(42);
	RngSeedManager::SetRun(1);

	// Open or create the out_file to collect the simulation results
	out_file.open("contrib/qdlfso/examples/qdl_fso_quantum_example_results.csv");
	out_file << std::fixed << std::setprecision(9);

	// Names of the columns of the csv
	// Note that "fidelity" is the actual fidelity of the received qubit, measured using q2ns::analysis, and therefore
	// is either 0 or 1. "tx_fidelity" is instead the dephasing probability computed at tx time. If the simulation is
	// long enough we get that mean("fidelity") = "tx_fidelity" for a given atmospheric condition. To simplifying
	// plotting we actually plot the "tx_fidelity".
	out_file << "time_s,qubit_id,node,event,fidelity,transmissivity,atm_condition,x,y,z,distance,tx_fidelity\n";

	// -------------------------------------------------------------------------------------
	// SIMULATION SETUP
	// -------------------------------------------------------------------------------------

	// Instantiate the q2ns NetController
	NetController net;
	// net.SetQStateBackend(QStateBackend::DM);
	net.SetQStateBackend(QStateBackend::Ket);

	// Create the two QNodes
	auto Alice = net.CreateNode("Alice"); // QBS, fixed
	auto Bob = net.CreateNode("Bob");	  // UE, moving

	// -------------------------------------------------------
	// Set up Mobility Models and Obstructions
	// -------------------------------------------------------
	// ---- Alice (Fixed QBS) ----
	MobilityHelper mobilityAlice;
	mobilityAlice.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobilityAlice.Install(Alice);
	Ptr<MobilityModel> aliceMobility = Alice->GetObject<MobilityModel>();
	aliceMobility->SetPosition(Vector(0.0, 0.0, 20));

	// ---- Bob (Moving UE) ----
	MobilityHelper mobilityBob;
	// Remember that we update manually Bob's position, so we instantiate a ConstantPositionMobilityModel
	mobilityBob.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobilityBob.Install(Bob);
	Ptr<MobilityModel> bobMobility = Bob->GetObject<MobilityModel>();
	bobMobility->SetPosition(Vector(circle_radius, 0.0, 1.5));
	// Schedule first update of Bob's position
	Simulator::Schedule(Seconds(0.0), [bobMobility]() { UpdateBobPosition(bobMobility); });

	// Construct a building to make Bob experience link outage for NLOS conditions
	std::vector<Ptr<Building>> buildingVector;
	Ptr<Building> building = CreateObject<Building>();
	building->SetBoundaries(Box(std::cos(M_PI / 5.0) * circle_radius,
								std::cos(M_PI / 5.0) * circle_radius + 20,
								-std::sin(M_PI / 5.0) * circle_radius,
								0,
								0,
								50));
	buildingVector.push_back(building);

	// Add bulding info to the mobility model (this is purely a "coding" necessity, it is not something logical)
	BuildingsHelper::Install(Alice);
	BuildingsHelper::Install(Bob);

	// -------------------------------------------------------
	// Set up Optical Terminals for the two nodes
	// -------------------------------------------------------

	// Instantiate Alice Terminal and configure parameters
	Ptr<QdlFsoOpticalTerminal> AliceTerminal = CreateObject<QdlFsoOpticalTerminal>();
	AliceTerminal->SetWavelength(1550e-9);		// 1550 nm
	AliceTerminal->SetTxBeamWaist(0.10);		// 10 cm radius
	AliceTerminal->SetPointingJitter(1.0e-6);	// 1 μrad
	AliceTerminal->SetRxApertureDiameter(0.3);	// 30 cm diameter
	AliceTerminal->SetTrackingEfficiency(0.85); // 100% tracking efficiency
	AliceTerminal->SetHardwareEfficiency(1.0);	// 100% hardware efficiency

	// Instantiate Bob Terminal and configure parameters
	Ptr<QdlFsoOpticalTerminal> BobTerminal = CreateObject<QdlFsoOpticalTerminal>();
	BobTerminal->SetWavelength(1550e-9);	  // 1550 nm
	BobTerminal->SetTxBeamWaist(0.10);		  // 10 cm radius
	BobTerminal->SetPointingJitter(1.0e-6);	  // 1 μrad
	BobTerminal->SetRxApertureDiameter(0.3);  // 30 cm diameter
	BobTerminal->SetTrackingEfficiency(0.85); // 100% tracking efficiency
	BobTerminal->SetHardwareEfficiency(1.0);  // 100% hardware efficiency

	// -------------------------------------------------------
	// Set up the FSO Propagation Loss Model and create the QChannel
	// -------------------------------------------------------

	// In this example we set up the propagation loss model manually, and later we pass it to the helper to create the
	// QdlFsoQChannel.
	// Therefore we do not use the prop model created by default in the helper. We do that for three reasons:
	// - We want to use a Kruse model instead of the default Lowtran model
	// - We need to have pointers to all sub-modules to update the channel conditions
	// - We use this opportunity to show how to set up a custom-made prop model to be used in simulations
	// In future versions of the qdl-fso module we might simplify this process a bit.

	Ptr<QdlFsoPropagationLossModel> propagation_loss = CreateObject<QdlFsoPropagationLossModel>();

	Ptr<BuildingsChannelConditionModel> condModel = CreateObject<BuildingsChannelConditionModel>();

	propagation_loss->SetChannelConditionModel(condModel);

	// Instantiate and set all sub-models
	ns3::Ptr<QdlFsoKruseAtmosphericModel> atm_model = CreateObject<QdlFsoKruseAtmosphericModel>();
	propagation_loss->SetAtmosphericModel(atm_model);

	ns3::Ptr<QdlFsoCn2Model> cn2_model = CreateObject<QdlFsoCn2Model>();
	propagation_loss->SetCn2Model(cn2_model);

	ns3::Ptr<QdlFsoRytovModel> rytov_model = CreateObject<QdlFsoRytovModel>();
	rytov_model->SetCn2Model(cn2_model);
	propagation_loss->SetRytovModel(rytov_model);

	ns3::Ptr<QdlFsoBeamWanderModel> wander_model = CreateObject<QdlFsoBeamWanderModel>();
	wander_model->SetCn2Model(cn2_model);
	propagation_loss->SetBeamWanderModel(wander_model);

	ns3::Ptr<QdlFsoGeometricPointingErrorModel> geom_point_model = CreateObject<QdlFsoGeometricPointingErrorModel>();
	geom_point_model->SetCn2Model(cn2_model);
	propagation_loss->SetGeometricPointingErrorModel(geom_point_model);

	ns3::Ptr<QdlFsoGammaGammaScintillationModel> scintillation_model =
		CreateObject<QdlFsoGammaGammaScintillationModel>();
	propagation_loss->SetScintillationModel(scintillation_model);

	// Use the helper to create the quantum channel
	ns3::Ptr<QdlFsoHelper> qdlFsoHelper = CreateObject<QdlFsoHelper>();
	// We pass the manuallyu created prop loss
	auto ch = qdlFsoHelper->InstallQdlFsoQuantumLink(Alice, Bob, AliceTerminal, BobTerminal, propagation_loss);

	// Enable the dephasing effects of the channel on the flying qubits
	ch->SetAttribute("EnableDephasing", BooleanValue(true));

	// Just for debugging
	// Alice->SetRecvCallback([](std::shared_ptr<Qubit> q) {
	// 	std::cout << Simulator::Now().GetMilliSeconds()
	// 	          << "ms [Alice][RECV][qdl-fso-quantum-ch]: qubit-" << q->GetQubitId() << ": yes
	// 	          "
	// 	          << std::endl;
	// 	std::cout << "###############################################################" <<
	// 	std::endl;
	// });

	// Just for debugging
	// Bob->SetRecvCallback([](std::shared_ptr<Qubit> q) {
	// 	std::cout << Simulator::Now().GetMilliSeconds()
	// 	          << "ms [Bob][RECV][qdl-fso-quantum-ch]: qubit-" << q->GetQubitId() << ": yes "
	// 	          << std::endl;
	// 	std::cout << "###############################################################" <<
	// 	std::endl;
	// });

	// -------------------------------------------------------
	// Set up Qubit transmissions, Fidelity estimations and TX, RX and LOST traces
	// -------------------------------------------------------

	// Periodic transmission of qubits
	Simulator::Schedule(Seconds(0.0), [Alice, Bob]() { ScheduleSend(Alice, Bob); });

	// For fidelity estimation
	auto refNode = net.CreateNode("ReferenceNode");

	// If we use one Qubit
	// auto qRef = refNode->CreateQubit("ReferenceQubit");
	// refNode->Apply(q2ns::gates::H(), {qRef});
	// auto sRef = net.GetState(qRef);

	// If we use entagled qubits
	auto [qRefA, qRefB] = refNode->CreateBellPair();
	auto sRef = net.GetState(qRefA);

	// Set up TX, RX and LOST traces
	ch->TraceConnectWithoutContext("QubitTxedTrace", MakeCallback(&OnTx));
	Bob->ConnectNetworkerTraces(MakeBoundCallback(&OnRx, sRef, &net), MakeCallback(&OnLost));

	// -------------------------------------------------------
	// Set up changes in atmospheric conditions
	// -------------------------------------------------------

	const AtmosphericScenario clearAir{50.0, 0.0, 1.7e-14, 21.0, 1.0e-6, 0.85};

	const AtmosphericScenario haze{3.0, 0.0, 1.7e-14, 21.0, 1.0e-6, 0.85};

	const AtmosphericScenario fog{0.5, 0.0, 1.7e-14, 21.0, 1.0e-6, 0.85};

	const AtmosphericScenario lightRain{20.0, 2.5, 1.7e-14, 21.0, 1.0e-6, 0.85};
	// look at "RECOMMENDATION ITU-R P.1817-1 - Propagation data required for the design of terrestrial free-space
	// optical links" for conversion between mm/h to dB/Km loss, using Carbonneau's model

	const AtmosphericScenario heavyRain{20.0, 25.0, 1.7e-14, 21.0, 1.0e-6, 0.85}; // heavy rain is 25 mm/h
	// look at "RECOMMENDATION ITU-R P.1817-1 - Propagation data required for the design of terrestrial free-space
	// optical links" for conversion between mm/h to dB/Km loss, using Carbonneau's model

	const AtmosphericScenario strongTurbulence{20.0, 0.0, 1.7e-13, 31.0, 1.0e-6, 0.85};

	const AtmosphericScenario severeTurbulence{20.0, 0.0, 8.5e-13, 61.0, 1.0e-6, 0.85};

	// Schedule all 10 changes

	// Initial conditions: clearAir
	ApplyAtmosphericScenario(clearAir, atm_model, cn2_model, AliceTerminal, BobTerminal);

	// haze
	Simulator::Schedule(Seconds((sim_end_time - 0.01) / 10.0), [=]() {
		ApplyAtmosphericScenario(haze, atm_model, cn2_model, AliceTerminal, BobTerminal);
		current_atmospheric_condition = "haze";
	});

	// fog
	Simulator::Schedule(Seconds(2.0 * (sim_end_time - 0.01) / 10.0), [=]() {
		ApplyAtmosphericScenario(fog, atm_model, cn2_model, AliceTerminal, BobTerminal);
		current_atmospheric_condition = "fog";
	});

	// clearAir
	Simulator::Schedule(Seconds(3.0 * (sim_end_time - 0.01) / 10.0), [=]() {
		ApplyAtmosphericScenario(clearAir, atm_model, cn2_model, AliceTerminal, BobTerminal);
		current_atmospheric_condition = "clear_air";
	});

	// lightRain
	Simulator::Schedule(Seconds(4.0 * (sim_end_time - 0.01) / 10.0), [=]() {
		ApplyAtmosphericScenario(lightRain, atm_model, cn2_model, AliceTerminal, BobTerminal);
		current_atmospheric_condition = "light_rain";
	});

	// heavyRain
	Simulator::Schedule(Seconds(5.0 * (sim_end_time - 0.01) / 10.0), [=]() {
		ApplyAtmosphericScenario(heavyRain, atm_model, cn2_model, AliceTerminal, BobTerminal);
		current_atmospheric_condition = "heavy_rain";
	});

	// clearAir
	Simulator::Schedule(Seconds(6.0 * (sim_end_time - 0.01) / 10.0), [=]() {
		ApplyAtmosphericScenario(clearAir, atm_model, cn2_model, AliceTerminal, BobTerminal);
		current_atmospheric_condition = "clear_air";
	});

	// strongTurbulence
	Simulator::Schedule(Seconds(7.0 * (sim_end_time - 0.01) / 10.0), [=]() {
		ApplyAtmosphericScenario(strongTurbulence, atm_model, cn2_model, AliceTerminal, BobTerminal);
		current_atmospheric_condition = "strong_turbulence";
	});

	// severeTurbulence
	Simulator::Schedule(Seconds(8.0 * (sim_end_time - 0.01) / 10.0), [=]() {
		ApplyAtmosphericScenario(severeTurbulence, atm_model, cn2_model, AliceTerminal, BobTerminal);
		current_atmospheric_condition = "severe_turbulence";
	});

	// blockage (the atmospheric conditions are clearAir, but it is in this tie period that there is a building between
	// Alice and Bob)
	Simulator::Schedule(Seconds(9.0 * (sim_end_time - 0.01) / 10.0), [=]() {
		ApplyAtmosphericScenario(clearAir, atm_model, cn2_model, AliceTerminal, BobTerminal);
		current_atmospheric_condition = "blockage";
	});

	std::cout << "---- START QUANTUM EXAMPLE ----" << std::endl;

	Simulator::Stop(Seconds(sim_end_time));
	Simulator::Run();
	Simulator::Destroy();

	out_file.close();

	std::cout << "---- END QUANTUM EXAMPLE ----" << std::endl;

	return 0;
}
