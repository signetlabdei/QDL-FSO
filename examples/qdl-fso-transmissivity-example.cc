#include "ns3/core-module.h"
#include "ns3/qdl-fso-atmospheric-model.h"
#include "ns3/qdl-fso-beam-wander-model.h"
// #include "ns3/qdl-fso-channel-core.h"
#include "ns3/qdl-fso-geometric-pointing-error-model.h"
#include "ns3/qdl-fso-optical-terminal.h"
#include "ns3/qdl-fso-propagation-loss-model.h"
#include "ns3/qdl-fso-rytov-model.h"
#include "ns3/qdl-fso-scintillation-model.h"

#include <fstream>
#include <iostream>

using namespace ns3;

int
main(int argc, char* argv[])
{
	std::string DELIM = ",";

	LogComponentEnable("QdlFsoAtmosphericModel", LOG_LEVEL_ERROR);
	LogComponentEnable("QdlFsoPropagationLossModel", LOG_LEVEL_ERROR);
	LogComponentEnable("QdlFsoRytovModel", LOG_LEVEL_ERROR);
	LogComponentEnable("QdlFsoBeamWanderModel", LOG_LEVEL_ERROR);
	LogComponentEnable("QdlFsoGeometricPointingErrorModel", LOG_LEVEL_ERROR);
	LogComponentEnable("QdlFsoScintillationModel", LOG_LEVEL_ERROR);

	QdlFsoPropagationLossModel::SetAltitudeTolerance(10.0);

	double wavelength_m = 1550e-9;

	// Independent RNG streams; AssignStreams returns how many it used
	int64_t streams_counter = 0;

	// =====================================================================
	// Scenario 1: tropical / rural
	// =====================================================================

	// Terminals
	Ptr<QdlFsoOpticalTerminal> txTerminal_tr = CreateObject<QdlFsoOpticalTerminal>();
	txTerminal_tr->SetWavelength(wavelength_m);
	txTerminal_tr->SetTxBeamWaist(0.10);
	txTerminal_tr->SetTxPower(0.01);		// 10 mW classical power
	txTerminal_tr->SetPointingJitter(1e-6); // 1 µrad jitter
	txTerminal_tr->SetHardwareEfficiency(1.0);

	Ptr<QdlFsoOpticalTerminal> rxTerminal_tr = CreateObject<QdlFsoOpticalTerminal>();
	rxTerminal_tr->SetWavelength(wavelength_m);
	rxTerminal_tr->SetRxApertureDiameter(0.3);	// 30 cm diameter
	rxTerminal_tr->SetTrackingEfficiency(0.85); //
	rxTerminal_tr->SetHardwareEfficiency(1.0);

	// Context
	QdlFsoPropagationContext context_tr;
	context_tr.scenario = QdlFsoScenario::Horizontal;
	context_tr.wavelength = wavelength_m;
	context_tr.tx = txTerminal_tr;
	context_tr.rx = rxTerminal_tr;
	context_tr.zenithAngle = M_PI / 2;
	context_tr.pathLength = 50;

	// Propagation loss and its models
	Ptr<QdlFsoPropagationLossModel> propagation_loss_tr = CreateObject<QdlFsoPropagationLossModel>();
	Ptr<AlwaysLosChannelConditionModel> condModel_tr = CreateObject<AlwaysLosChannelConditionModel>();
	propagation_loss_tr->SetChannelConditionModel(condModel_tr);

	Ptr<QdlFsoLowtranAtmosphericModel> rural_tropical_model = CreateObject<QdlFsoLowtranAtmosphericModel>();
	rural_tropical_model->SetAttribute("AtmosphericModel",
									   EnumValue(QdlFsoLowtranAtmosphericModel::AtmosphericModel::TROPICAL));
	rural_tropical_model->SetAttribute("AerosolType",
									   EnumValue(QdlFsoLowtranAtmosphericModel::AerosolType::RURAL_5_KM));
	propagation_loss_tr->SetAtmosphericModel(rural_tropical_model);

	Ptr<QdlFsoCn2Model> cn2_model_tr = CreateObject<QdlFsoCn2Model>();
	propagation_loss_tr->SetCn2Model(cn2_model_tr);
	Ptr<QdlFsoRytovModel> rytov_model_tr = CreateObject<QdlFsoRytovModel>();
	rytov_model_tr->SetCn2Model(cn2_model_tr);
	propagation_loss_tr->SetRytovModel(rytov_model_tr);
	Ptr<QdlFsoBeamWanderModel> wander_model_tr = CreateObject<QdlFsoBeamWanderModel>();
	wander_model_tr->SetCn2Model(cn2_model_tr);
	propagation_loss_tr->SetBeamWanderModel(wander_model_tr);
	Ptr<QdlFsoGeometricPointingErrorModel> geom_point_model_tr = CreateObject<QdlFsoGeometricPointingErrorModel>();
	geom_point_model_tr->SetCn2Model(cn2_model_tr);
	propagation_loss_tr->SetGeometricPointingErrorModel(geom_point_model_tr);
	Ptr<QdlFsoGammaGammaScintillationModel> scintillation_model_tr = CreateObject<QdlFsoGammaGammaScintillationModel>();
	propagation_loss_tr->SetScintillationModel(scintillation_model_tr);

	streams_counter += propagation_loss_tr->AssignStreams(streams_counter);

	// =====================================================================
	// Scenario 3: Windy
	// =====================================================================

	// Terminals
	Ptr<QdlFsoOpticalTerminal> txTerminal_wi = CreateObject<QdlFsoOpticalTerminal>();
	txTerminal_wi->SetWavelength(wavelength_m);
	txTerminal_wi->SetTxBeamWaist(0.10);
	txTerminal_wi->SetTxPower(0.01);		// 10 mW classical power
	txTerminal_wi->SetPointingJitter(1e-6); // 1 µrad jitter
	txTerminal_wi->SetHardwareEfficiency(1.0);

	Ptr<QdlFsoOpticalTerminal> rxTerminal_wi = CreateObject<QdlFsoOpticalTerminal>();
	rxTerminal_wi->SetWavelength(wavelength_m);
	rxTerminal_wi->SetRxApertureDiameter(0.3);	// 30 cm diameter
	rxTerminal_wi->SetTrackingEfficiency(0.85); //
	rxTerminal_wi->SetHardwareEfficiency(1.0);

	// Context
	QdlFsoPropagationContext context_wi;
	context_wi.scenario = QdlFsoScenario::Horizontal;
	context_wi.wavelength = wavelength_m;
	context_wi.tx = txTerminal_wi;
	context_wi.rx = rxTerminal_wi;
	context_wi.zenithAngle = M_PI / 2;
	context_wi.pathLength = 50;

	// Propagation loss and its models
	Ptr<QdlFsoPropagationLossModel> propagation_loss_wi = CreateObject<QdlFsoPropagationLossModel>();
	Ptr<AlwaysLosChannelConditionModel> condModel_wi = CreateObject<AlwaysLosChannelConditionModel>();
	propagation_loss_wi->SetChannelConditionModel(condModel_wi);

	Ptr<QdlFsoLowtranAtmosphericModel> navy_model = CreateObject<QdlFsoLowtranAtmosphericModel>();
	navy_model->SetAttribute("AtmosphericModel",
							 EnumValue(QdlFsoLowtranAtmosphericModel::AtmosphericModel::MIDLATITUDE_WINTER));
	navy_model->SetAttribute("AerosolType", EnumValue(QdlFsoLowtranAtmosphericModel::AerosolType::NAVY));
	navy_model->SetAttribute("AirMassCharacter", IntegerValue(3));
	navy_model->SetAttribute("WindSpeed", DoubleValue(5));
	navy_model->SetAttribute("AvgWindSpeed", DoubleValue(5));
	propagation_loss_wi->SetAtmosphericModel(navy_model);

	Ptr<QdlFsoCn2Model> cn2_model_wi = CreateObject<QdlFsoCn2Model>();
	cn2_model_wi->SetCn20(5e-13);
	cn2_model_wi->SetVWind(50);
	propagation_loss_wi->SetCn2Model(cn2_model_wi);
	Ptr<QdlFsoRytovModel> rytov_model_wi = CreateObject<QdlFsoRytovModel>();
	rytov_model_wi->SetCn2Model(cn2_model_wi);
	propagation_loss_wi->SetRytovModel(rytov_model_wi);
	Ptr<QdlFsoBeamWanderModel> wander_model_wi = CreateObject<QdlFsoBeamWanderModel>();
	wander_model_wi->SetCn2Model(cn2_model_wi);
	propagation_loss_wi->SetBeamWanderModel(wander_model_wi);
	Ptr<QdlFsoGeometricPointingErrorModel> geom_point_model_wi = CreateObject<QdlFsoGeometricPointingErrorModel>();
	geom_point_model_wi->SetCn2Model(cn2_model_wi);
	propagation_loss_wi->SetGeometricPointingErrorModel(geom_point_model_wi);
	Ptr<QdlFsoGammaGammaScintillationModel> scintillation_model_wi = CreateObject<QdlFsoGammaGammaScintillationModel>();
	propagation_loss_wi->SetScintillationModel(scintillation_model_wi);

	streams_counter += propagation_loss_wi->AssignStreams(streams_counter);

	// =====================================================================
	// Scenario 4: Arctic
	// =====================================================================

	// Terminals
	Ptr<QdlFsoOpticalTerminal> txTerminal_ar = CreateObject<QdlFsoOpticalTerminal>();
	txTerminal_ar->SetWavelength(wavelength_m);
	txTerminal_ar->SetTxBeamWaist(0.10);
	txTerminal_ar->SetTxPower(0.01);		// 10 mW classical power
	txTerminal_ar->SetPointingJitter(1e-6); // 1 µrad jitter
	txTerminal_ar->SetHardwareEfficiency(1.0);

	Ptr<QdlFsoOpticalTerminal> rxTerminal_ar = CreateObject<QdlFsoOpticalTerminal>();
	rxTerminal_ar->SetWavelength(wavelength_m);
	rxTerminal_ar->SetRxApertureDiameter(0.3);	// 30 cm diameter
	rxTerminal_ar->SetTrackingEfficiency(0.85); //
	rxTerminal_ar->SetHardwareEfficiency(1.0);

	// Context
	QdlFsoPropagationContext context_ar;
	context_ar.scenario = QdlFsoScenario::Horizontal;
	context_ar.wavelength = wavelength_m;
	context_ar.tx = txTerminal_ar;
	context_ar.rx = rxTerminal_ar;
	context_ar.zenithAngle = M_PI / 2;
	context_ar.pathLength = 50;

	// Propagation loss and its models
	Ptr<QdlFsoPropagationLossModel> propagation_loss_ar = CreateObject<QdlFsoPropagationLossModel>();
	Ptr<AlwaysLosChannelConditionModel> condModel_ar = CreateObject<AlwaysLosChannelConditionModel>();
	propagation_loss_ar->SetChannelConditionModel(condModel_ar);

	Ptr<QdlFsoLowtranAtmosphericModel> arctic_model = CreateObject<QdlFsoLowtranAtmosphericModel>();
	arctic_model->SetAttribute("AtmosphericModel",
							   EnumValue(QdlFsoLowtranAtmosphericModel::AtmosphericModel::SUBARCTIC_WINTER));
	arctic_model->SetAttribute("AerosolType", EnumValue(QdlFsoLowtranAtmosphericModel::AerosolType::RURAL_23_KM));
	propagation_loss_ar->SetAtmosphericModel(arctic_model);

	Ptr<QdlFsoCn2Model> cn2_model_ar = CreateObject<QdlFsoCn2Model>();
	propagation_loss_ar->SetCn2Model(cn2_model_ar);
	Ptr<QdlFsoRytovModel> rytov_model_ar = CreateObject<QdlFsoRytovModel>();
	rytov_model_ar->SetCn2Model(cn2_model_ar);
	propagation_loss_ar->SetRytovModel(rytov_model_ar);
	Ptr<QdlFsoBeamWanderModel> wander_model_ar = CreateObject<QdlFsoBeamWanderModel>();
	wander_model_ar->SetCn2Model(cn2_model_ar);
	propagation_loss_ar->SetBeamWanderModel(wander_model_ar);
	Ptr<QdlFsoGeometricPointingErrorModel> geom_point_model_ar = CreateObject<QdlFsoGeometricPointingErrorModel>();
	geom_point_model_ar->SetCn2Model(cn2_model_ar);
	propagation_loss_ar->SetGeometricPointingErrorModel(geom_point_model_ar);
	Ptr<QdlFsoGammaGammaScintillationModel> scintillation_model_ar = CreateObject<QdlFsoGammaGammaScintillationModel>();
	propagation_loss_ar->SetScintillationModel(scintillation_model_ar);

	streams_counter += propagation_loss_ar->AssignStreams(streams_counter);

	constexpr int NUM_SAMPLES = 1000;

	// =====================================================================
	// Horizontal
	// =====================================================================
	std::ofstream csvFile1("contrib/qdlfso/examples/qdl_fso_transmissivity_example_horizontal.csv");
	NS_ASSERT_MSG(csvFile1.is_open(), "Unable to open output CSV file.");

	csvFile1 << "Horizontal distance [m]" << DELIM << "Tropical" /*<< DELIM << "Washington"*/ << DELIM << "Ocean Wind"
			 << DELIM << "Subarctic" << std::endl;

	context_tr.rxAltitude = context_tr.txAltitude = 0;
	// context_wa.rxAltitude = context_wa.txAltitude = 10;
	context_wi.rxAltitude = context_wi.txAltitude = 0;
	context_ar.rxAltitude = context_ar.txAltitude = 0;

	double init_dist = 100;
	for (double distance = init_dist; distance <= 5000; distance += init_dist)
	{
		std::cout << distance << std::endl;
		context_tr.pathLength = distance;
		// context_wa.pathLength = distance;
		context_wi.pathLength = distance;
		context_ar.pathLength = distance;

		for (int i = 0; i < NUM_SAMPLES; i++)
		{
			csvFile1 << (int)distance;

			double res_rural_tropical = propagation_loss_tr->GetTransmissivity(context_tr);
			csvFile1 << DELIM << res_rural_tropical;

			// double res_washington = propagation_loss_wa->GetTransmissivity(context_wa);
			// csvFile1 << DELIM << res_washington;

			double res_windy = propagation_loss_wi->GetTransmissivity(context_wi);
			csvFile1 << DELIM << res_windy;

			double res_artic = propagation_loss_ar->GetTransmissivity(context_ar);
			csvFile1 << DELIM << res_artic;

			csvFile1 << std::endl;
		}
	}
	csvFile1.close();

	// =====================================================================
	// Downlink
	// =====================================================================
	std::ofstream csvFile2("contrib/qdlfso/examples/qdl_fso_transmissivity_example_downlink.csv");
	NS_ASSERT_MSG(csvFile2.is_open(), "Unable to open output CSV file.");

	context_tr.scenario = QdlFsoScenario::Downlink;
	context_tr.zenithAngle = 0;
	context_tr.rxAltitude = 0;

	// context_wa.scenario = QdlFsoScenario::Downlink;
	// context_wa.zenithAngle = 0;
	// context_wa.rxAltitude = 0;

	context_wi.scenario = QdlFsoScenario::Downlink;
	context_wi.zenithAngle = 0;
	context_wi.rxAltitude = 0;

	context_ar.scenario = QdlFsoScenario::Downlink;
	context_ar.zenithAngle = 0;
	context_ar.rxAltitude = 0;

	// washington_model->SetAttribute("GroundAltitude", DoubleValue(0.0));
	// washington_model->SetAttribute("CloudRainModel",
	// EnumValue(QdlFsoLowtranAtmosphericModel::CloudRainModel::NO_CLOUDS));

	csvFile2 << "Transmitter altitude [m]" << DELIM << "Tropical" /*<< DELIM << "Washington"*/ << DELIM << "Ocean Wind"
			 << DELIM << "Subarctic" << std::endl;
	for (double distance = 500; distance <= 500000;
		 distance += ((distance < 5000) ? 500 : ((distance < 50000) ? 5000 : 50000)))
	{
		std::cout << distance << std::endl;
		context_tr.txAltitude = context_tr.pathLength = distance;
		// context_wa.txAltitude = context_wa.pathLength = distance;
		context_wi.txAltitude = context_wi.pathLength = distance;
		context_ar.txAltitude = context_ar.pathLength = distance;

		for (int i = 0; i < NUM_SAMPLES; i++)
		{
			csvFile2 << (int)distance;

			double res_rural_tropical = propagation_loss_tr->GetTransmissivity(context_tr);
			csvFile2 << DELIM << res_rural_tropical;

			// double res_washington = propagation_loss_wa->GetTransmissivity(context_wa);
			// csvFile2 << DELIM << res_washington;

			double res_windy = propagation_loss_wi->GetTransmissivity(context_wi);
			csvFile2 << DELIM << res_windy;

			double res_artic = propagation_loss_ar->GetTransmissivity(context_ar);
			csvFile2 << DELIM << res_artic;

			csvFile2 << std::endl;
		}
	}
	csvFile2.close();

	// =====================================================================
	// Uplink
	// =====================================================================
	std::ofstream csvFile3("contrib/qdlfso/examples/qdl_fso_transmissivity_example_uplink.csv");
	NS_ASSERT_MSG(csvFile3.is_open(), "Unable to open output CSV file.");

	context_tr.scenario = QdlFsoScenario::Uplink;
	context_tr.zenithAngle = 0;
	context_tr.txAltitude = 0;

	// context_wa.scenario = QdlFsoScenario::Uplink;
	// context_wa.zenithAngle = 0;
	// context_wa.txAltitude = 0;

	context_wi.scenario = QdlFsoScenario::Uplink;
	context_wi.zenithAngle = 0;
	context_wi.txAltitude = 0;

	context_ar.scenario = QdlFsoScenario::Uplink;
	context_ar.zenithAngle = 0;
	context_ar.txAltitude = 0;

	csvFile3 << "Receiver altitude [m]" << DELIM << "Tropical" /*<< DELIM << "Washington"*/ << DELIM << "Ocean Wind"
			 << DELIM << "Subarctic" << std::endl;
	for (double distance = 500; distance <= 500000;
		 distance += ((distance < 5000) ? 500 : ((distance < 50000) ? 5000 : 50000)))
	{
		std::cout << distance << std::endl;
		context_tr.rxAltitude = context_tr.pathLength = distance;
		// context_wa.rxAltitude = context_wa.pathLength = distance;
		context_wi.rxAltitude = context_wi.pathLength = distance;
		context_ar.rxAltitude = context_ar.pathLength = distance;

		for (int i = 0; i < NUM_SAMPLES; i++)
		{
			csvFile3 << (int)distance;

			double res_rural_tropical = propagation_loss_tr->GetTransmissivity(context_tr);
			csvFile3 << DELIM << res_rural_tropical;

			// double res_washington = propagation_loss_wa->GetTransmissivity(context_wa);
			// csvFile3 << DELIM << res_washington;

			double res_windy = propagation_loss_wi->GetTransmissivity(context_wi);
			csvFile3 << DELIM << res_windy;

			double res_artic = propagation_loss_ar->GetTransmissivity(context_ar);
			csvFile3 << DELIM << res_artic;

			csvFile3 << std::endl;
		}
	}
	csvFile3.close();

	std::cout << "Program terminated normally." << std::endl;
	return 0;
}
