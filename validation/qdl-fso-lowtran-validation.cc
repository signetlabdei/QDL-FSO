#include "ns3/core-module.h"
#include "ns3/qdl-fso-atmospheric-model.h"
#include "ns3/qdl-fso-beam-wander-model.h"
// #include "ns3/qdl-fso-channel-core.h"
#include "ns3/qdl-fso-geometric-pointing-error-model.h"
#include "ns3/qdl-fso-optical-terminal.h"
#include "ns3/qdl-fso-propagation-loss-model.h"
#include "ns3/qdl-fso-rytov-model.h"
#include "ns3/qdl-fso-scintillation-model.h"

using namespace ns3;

int
main(int argc, char* argv[])
{
	std::string DELIM = ",";

	LogComponentEnable("QdlFsoAtmosphericModel", LOG_LEVEL_INFO);
	LogComponentEnable("QdlFsoPropagationLossModel", LOG_LEVEL_INFO);
	LogComponentEnable("QdlFsoRytovModel", LOG_LEVEL_INFO);
	LogComponentEnable("QdlFsoBeamWanderModel", LOG_LEVEL_INFO);
	LogComponentEnable("QdlFsoGeometricPointingErrorModel", LOG_LEVEL_INFO);
	LogComponentEnable("QdlFsoScintillationModel", LOG_LEVEL_INFO);

	QdlFsoPropagationLossModel::SetAltitudeTolerance(101.0);

	double wavelength_m = 1550e-9;

	// Instantiate Tx Terminal and configure parameters
	Ptr<QdlFsoOpticalTerminal> txTerminal = CreateObject<QdlFsoOpticalTerminal>();
	txTerminal->SetWavelength(wavelength_m);
	txTerminal->SetTxBeamWaist(0.10);		 // 2.5 cm radius
	txTerminal->SetTxPower(0.01);			 // 10 mW classical power
	txTerminal->SetPointingJitter(1e-6);	 // 1 µrad jitter
	txTerminal->SetHardwareEfficiency(0.85); // 85% efficiency

	// Instantiate Rx Terminal and configure parameters
	Ptr<QdlFsoOpticalTerminal> rxTerminal = CreateObject<QdlFsoOpticalTerminal>();
	rxTerminal->SetWavelength(wavelength_m);
	rxTerminal->SetRxApertureDiameter(0.3);	 // 30 cm diameter
	rxTerminal->SetTrackingEfficiency(0.95); // 95% tracking efficiency
	rxTerminal->SetHardwareEfficiency(0.85); // 85% efficiency

	QdlFsoPropagationContext context;

	context.scenario = QdlFsoScenario::Horizontal;
	context.wavelength = wavelength_m;
	context.tx = txTerminal;
	context.rx = rxTerminal;
	context.rxAltitude = context.txAltitude = 0;
	context.zenithAngle = M_PI / 2;
	context.pathLength = 50;

	std::ofstream csvFile1("contrib/qdlfso/validation/qdl_fso_lowtran_kim_kruse.csv");

	NS_ASSERT_MSG(csvFile1.is_open(), "Unable to open output CSV file.");

	Ptr<QdlFsoLowtranAtmosphericModel> lwtrn_model = CreateObject<QdlFsoLowtranAtmosphericModel>();
	lwtrn_model->SetAttribute("AtmosphericModel", EnumValue(QdlFsoLowtranAtmosphericModel::AtmosphericModel::TROPICAL));
	lwtrn_model->SetAttribute("AerosolType", EnumValue(QdlFsoLowtranAtmosphericModel::AerosolType::URBAN_5_KM));
	// lwtrn_model->SetAttribute("Visibility", DoubleValue(5));
	Ptr<QdlFsoKimAtmosphericModel> kim_model = CreateObject<QdlFsoKimAtmosphericModel>();
	kim_model->SetAttribute("VisibilityKm", DoubleValue(5));
	Ptr<QdlFsoKruseAtmosphericModel> kruse_model = CreateObject<QdlFsoKruseAtmosphericModel>();
	kruse_model->SetAttribute("VisibilityKm", DoubleValue(5));

	csvFile1 << "meters" << DELIM << "LOWTRAN (Tropical Urban 5 km)" << DELIM << "Kim (5 km)" << DELIM << "Kruse (5 km)"
			 << std::endl;

	double init_dist = 100;
	for (double distance = init_dist; distance <= 10000; distance += init_dist)
	{
		context.pathLength = distance;

		double res_lwtrn = lwtrn_model->GetAtmosphericTransmissivity(context);
		double res_kim = kim_model->GetAtmosphericTransmissivity(context);
		double res_kruse = kruse_model->GetAtmosphericTransmissivity(context);

		csvFile1 << (int)distance << DELIM << res_lwtrn << DELIM << res_kim << DELIM << res_kruse << std::endl;
	}
	csvFile1.close();

	std::ofstream csvFile2("contrib/qdlfso/validation/qdl_fso_lowtran_atmospheric_model_no_aerosol.csv");

	Config::SetDefault("ns3::QdlFsoLowtranAtmosphericModel::AerosolType",
					   EnumValue(QdlFsoLowtranAtmosphericModel::AerosolType::NO_AEROSOL));
	Ptr<QdlFsoLowtranAtmosphericModel> tropical = CreateObject<QdlFsoLowtranAtmosphericModel>();
	tropical->SetAttribute("AtmosphericModel", EnumValue(QdlFsoLowtranAtmosphericModel::AtmosphericModel::TROPICAL));
	Ptr<QdlFsoLowtranAtmosphericModel> us = CreateObject<QdlFsoLowtranAtmosphericModel>();
	us->SetAttribute("AtmosphericModel", EnumValue(QdlFsoLowtranAtmosphericModel::AtmosphericModel::US_STANDARD));
	Ptr<QdlFsoLowtranAtmosphericModel> ms = CreateObject<QdlFsoLowtranAtmosphericModel>();
	ms->SetAttribute("AtmosphericModel",
					 EnumValue(QdlFsoLowtranAtmosphericModel::AtmosphericModel::MIDLATITUDE_SUMMER));
	Ptr<QdlFsoLowtranAtmosphericModel> mw = CreateObject<QdlFsoLowtranAtmosphericModel>();
	mw->SetAttribute("AtmosphericModel",
					 EnumValue(QdlFsoLowtranAtmosphericModel::AtmosphericModel::MIDLATITUDE_WINTER));
	Ptr<QdlFsoLowtranAtmosphericModel> ss = CreateObject<QdlFsoLowtranAtmosphericModel>();
	ss->SetAttribute("AtmosphericModel", EnumValue(QdlFsoLowtranAtmosphericModel::AtmosphericModel::SUBARCTIC_SUMMER));
	Ptr<QdlFsoLowtranAtmosphericModel> sw = CreateObject<QdlFsoLowtranAtmosphericModel>();
	sw->SetAttribute("AtmosphericModel", EnumValue(QdlFsoLowtranAtmosphericModel::AtmosphericModel::SUBARCTIC_WINTER));

	csvFile2 << "meters" << DELIM << "Tropical" << DELIM << "US Standard" << DELIM << "Midlatitude Summer" << DELIM
			 << "Midlatitude Winter" << DELIM << "Subarctic Summer" << DELIM << "Subarctic Winter" << std::endl;
	init_dist = 1000;
	for (double distance = init_dist; distance <= 20000; distance += init_dist)
	{
		context.pathLength = distance;

		double res_tropical = tropical->GetAtmosphericTransmissivity(context);
		double res_us = us->GetAtmosphericTransmissivity(context);
		double res_ms = ms->GetAtmosphericTransmissivity(context);
		double res_mw = mw->GetAtmosphericTransmissivity(context);
		double res_ss = ss->GetAtmosphericTransmissivity(context);
		double res_sw = sw->GetAtmosphericTransmissivity(context);

		csvFile2 << (int)distance << DELIM << res_tropical << DELIM << res_us << DELIM << res_ms << DELIM << res_mw
				 << DELIM << res_ss << DELIM << res_sw << std::endl;
	}
	csvFile2.close();

	std::ofstream csvFile3("contrib/qdlfso/validation/qdl_fso_lowtran_desert_wind.csv");

	Config::SetDefault("ns3::QdlFsoLowtranAtmosphericModel::AtmosphericModel",
					   EnumValue(QdlFsoLowtranAtmosphericModel::AtmosphericModel::MIDLATITUDE_SUMMER));
	Config::SetDefault("ns3::QdlFsoLowtranAtmosphericModel::AerosolType",
					   EnumValue(QdlFsoLowtranAtmosphericModel::AerosolType::DESERT));
	Ptr<QdlFsoLowtranAtmosphericModel> des_0 = CreateObject<QdlFsoLowtranAtmosphericModel>();
	Ptr<QdlFsoLowtranAtmosphericModel> des_5 = CreateObject<QdlFsoLowtranAtmosphericModel>();
	des_5->SetAttribute("WindSpeed", DoubleValue(5.0));
	Ptr<QdlFsoLowtranAtmosphericModel> des_10 = CreateObject<QdlFsoLowtranAtmosphericModel>();
	des_10->SetAttribute("WindSpeed", DoubleValue(10.0));
	Ptr<QdlFsoLowtranAtmosphericModel> des_15 = CreateObject<QdlFsoLowtranAtmosphericModel>();
	des_15->SetAttribute("WindSpeed", DoubleValue(15.0));
	Ptr<QdlFsoLowtranAtmosphericModel> des_20 = CreateObject<QdlFsoLowtranAtmosphericModel>();
	des_20->SetAttribute("WindSpeed", DoubleValue(20.0));

	csvFile3 << "meters" << DELIM << "0 m/s" << DELIM << "5 m/s" << DELIM << "10 m/s" << DELIM << "15 m/s" << DELIM
			 << "20 m/s" << std::endl;
	init_dist = 1000;
	for (double distance = init_dist; distance <= 20000; distance += init_dist)
	{
		context.pathLength = distance;

		double res_0 = des_0->GetAtmosphericTransmissivity(context);
		double res_5 = des_5->GetAtmosphericTransmissivity(context);
		double res_10 = des_10->GetAtmosphericTransmissivity(context);
		double res_15 = des_15->GetAtmosphericTransmissivity(context);
		double res_20 = des_20->GetAtmosphericTransmissivity(context);

		csvFile3 << (int)distance << DELIM << res_0 << DELIM << res_5 << DELIM << res_10 << DELIM << res_15 << DELIM
				 << res_20 << std::endl;
	}
	csvFile3.close();

	std::ofstream csvFile4("contrib/qdlfso/validation/qdl_fso_lowtran_navy_windspeed_avgwindspeed.csv");

	Config::SetDefault("ns3::QdlFsoLowtranAtmosphericModel::AtmosphericModel",
					   EnumValue(QdlFsoLowtranAtmosphericModel::AtmosphericModel::MIDLATITUDE_SUMMER));
	Config::SetDefault("ns3::QdlFsoLowtranAtmosphericModel::AerosolType",
					   EnumValue(QdlFsoLowtranAtmosphericModel::AerosolType::NAVY));
	Config::SetDefault("ns3::QdlFsoLowtranAtmosphericModel::WindSpeed", DoubleValue(0));
	Config::SetDefault("ns3::QdlFsoLowtranAtmosphericModel::AvgWindSpeed", DoubleValue(0));
	Config::SetDefault("ns3::QdlFsoLowtranAtmosphericModel::AirMassCharacter", IntegerValue(5));

	Ptr<QdlFsoLowtranAtmosphericModel> navy_0_0 = CreateObject<QdlFsoLowtranAtmosphericModel>();
	Ptr<QdlFsoLowtranAtmosphericModel> navy_5_0 = CreateObject<QdlFsoLowtranAtmosphericModel>();
	navy_5_0->SetAttribute("WindSpeed", DoubleValue(5));
	Ptr<QdlFsoLowtranAtmosphericModel> navy_15_0 = CreateObject<QdlFsoLowtranAtmosphericModel>();
	navy_15_0->SetAttribute("WindSpeed", DoubleValue(15));
	Ptr<QdlFsoLowtranAtmosphericModel> navy_15_5 = CreateObject<QdlFsoLowtranAtmosphericModel>();
	navy_15_5->SetAttribute("WindSpeed", DoubleValue(15));
	navy_15_5->SetAttribute("AvgWindSpeed", DoubleValue(5));
	Ptr<QdlFsoLowtranAtmosphericModel> navy_15_15 = CreateObject<QdlFsoLowtranAtmosphericModel>();
	navy_15_15->SetAttribute("WindSpeed", DoubleValue(15));
	navy_15_15->SetAttribute("AvgWindSpeed", DoubleValue(15));
	Ptr<QdlFsoLowtranAtmosphericModel> navy_15_30 = CreateObject<QdlFsoLowtranAtmosphericModel>();
	navy_15_30->SetAttribute("WindSpeed", DoubleValue(15));
	navy_15_30->SetAttribute("AvgWindSpeed", DoubleValue(30));
	Ptr<QdlFsoLowtranAtmosphericModel> navy_5_30 = CreateObject<QdlFsoLowtranAtmosphericModel>();
	navy_5_30->SetAttribute("WindSpeed", DoubleValue(5));
	navy_5_30->SetAttribute("AvgWindSpeed", DoubleValue(30));

	csvFile4 << "meters" << DELIM << "0 m/s - 0 m/s" << DELIM << "5 m/s - 0 m/s" << DELIM << "15 m/s - 0 m/s" << DELIM
			 << "15 m/s - 5 m/s" << DELIM << "15 m/s - 15 m/s" << DELIM << "15 m/s - 30 m/s" << DELIM
			 << "5 m/s - 30 m/s" << std::endl;
	init_dist = 1000;
	for (double distance = init_dist; distance <= 20000; distance += init_dist)
	{
		context.pathLength = distance;

		double res_0_0 = navy_0_0->GetAtmosphericTransmissivity(context);
		double res_5_0 = navy_5_0->GetAtmosphericTransmissivity(context);
		double res_15_0 = navy_15_0->GetAtmosphericTransmissivity(context);
		double res_15_5 = navy_15_5->GetAtmosphericTransmissivity(context);
		double res_15_15 = navy_15_15->GetAtmosphericTransmissivity(context);
		double res_15_30 = navy_15_30->GetAtmosphericTransmissivity(context);
		double res_5_30 = navy_5_30->GetAtmosphericTransmissivity(context);

		csvFile4 << (int)distance << DELIM << res_0_0 << DELIM << res_5_0 << DELIM << res_15_0 << DELIM << res_15_5
				 << DELIM << res_15_15 << DELIM << res_15_30 << DELIM << res_5_30 << std::endl;
	}
	csvFile4.close();

	std::ofstream csvFile5("contrib/qdlfso/validation/qdl_fso_lowtran_navy_airmasscharacter.csv");

	Config::SetDefault("ns3::QdlFsoLowtranAtmosphericModel::AtmosphericModel",
					   EnumValue(QdlFsoLowtranAtmosphericModel::AtmosphericModel::MIDLATITUDE_SUMMER));
	Config::SetDefault("ns3::QdlFsoLowtranAtmosphericModel::AerosolType",
					   EnumValue(QdlFsoLowtranAtmosphericModel::AerosolType::NAVY));
	Config::SetDefault("ns3::QdlFsoLowtranAtmosphericModel::WindSpeed", DoubleValue(5));
	Config::SetDefault("ns3::QdlFsoLowtranAtmosphericModel::AvgWindSpeed", DoubleValue(5));

	Ptr<QdlFsoLowtranAtmosphericModel> navy_0 = CreateObject<QdlFsoLowtranAtmosphericModel>();
	navy_0->SetAttribute("AirMassCharacter", IntegerValue(1));
	Ptr<QdlFsoLowtranAtmosphericModel> navy_5 = CreateObject<QdlFsoLowtranAtmosphericModel>();
	navy_5->SetAttribute("AirMassCharacter", IntegerValue(5));
	Ptr<QdlFsoLowtranAtmosphericModel> navy_10 = CreateObject<QdlFsoLowtranAtmosphericModel>();
	navy_10->SetAttribute("AirMassCharacter", IntegerValue(10));

	csvFile5 << "meters" << DELIM << "1 (open ocean)" << DELIM << "5" << DELIM << "10 (strong continental influence)"
			 << std::endl;
	init_dist = 1000;
	for (double distance = init_dist; distance <= 20000; distance += init_dist)
	{
		context.pathLength = distance;

		double res_0 = navy_0->GetAtmosphericTransmissivity(context);
		double res_5 = navy_5->GetAtmosphericTransmissivity(context);
		double res_10 = navy_10->GetAtmosphericTransmissivity(context);

		csvFile5 << (int)distance << DELIM << res_0 << DELIM << res_5 << DELIM << res_10 << std::endl;
	}
	csvFile5.close();

	std::ofstream csvFile6("contrib/qdlfso/validation/qdl_fso_lowtran_vertical_volcanic.csv");
	context.scenario = QdlFsoScenario::Uplink;
	context.wavelength = wavelength_m;
	context.tx = txTerminal;
	context.rx = rxTerminal;
	context.txAltitude = 0;
	context.zenithAngle = 0;

	Config::SetDefault("ns3::QdlFsoLowtranAtmosphericModel::AtmosphericModel",
					   EnumValue(QdlFsoLowtranAtmosphericModel::AtmosphericModel::MIDLATITUDE_SUMMER));
	Config::SetDefault("ns3::QdlFsoLowtranAtmosphericModel::AerosolType",
					   EnumValue(QdlFsoLowtranAtmosphericModel::AerosolType::RURAL_23_KM));
	Config::SetDefault("ns3::QdlFsoLowtranAtmosphericModel::CloudRainModel",
					   EnumValue(QdlFsoLowtranAtmosphericModel::CloudRainModel::NO_CLOUDS));

	Ptr<QdlFsoLowtranAtmosphericModel> str_bg = CreateObject<QdlFsoLowtranAtmosphericModel>();
	str_bg->SetAttribute("VolcanicTypeProfile",
						 EnumValue(QdlFsoLowtranAtmosphericModel::VolcanicTypeProfile::STRATOSPHERIC_BACKGROUND));

	Ptr<QdlFsoLowtranAtmosphericModel> avmv = CreateObject<QdlFsoLowtranAtmosphericModel>();
	avmv->SetAttribute("VolcanicTypeProfile",
					   EnumValue(QdlFsoLowtranAtmosphericModel::VolcanicTypeProfile::AGED_VOLC_MODERATE_VOLC));

	Ptr<QdlFsoLowtranAtmosphericModel> fvhv = CreateObject<QdlFsoLowtranAtmosphericModel>();
	fvhv->SetAttribute("VolcanicTypeProfile",
					   EnumValue(QdlFsoLowtranAtmosphericModel::VolcanicTypeProfile::FRESH_VOLC_HIGH_VOLC));

	Ptr<QdlFsoLowtranAtmosphericModel> avhv = CreateObject<QdlFsoLowtranAtmosphericModel>();
	avhv->SetAttribute("VolcanicTypeProfile",
					   EnumValue(QdlFsoLowtranAtmosphericModel::VolcanicTypeProfile::AGED_VOLC_HIGH_VOLC));

	Ptr<QdlFsoLowtranAtmosphericModel> fvmv = CreateObject<QdlFsoLowtranAtmosphericModel>();
	fvmv->SetAttribute("VolcanicTypeProfile",
					   EnumValue(QdlFsoLowtranAtmosphericModel::VolcanicTypeProfile::FRESH_VOLC_MODERATE_VOLC));

	Ptr<QdlFsoLowtranAtmosphericModel> bsmv = CreateObject<QdlFsoLowtranAtmosphericModel>();
	bsmv->SetAttribute("VolcanicTypeProfile",
					   EnumValue(QdlFsoLowtranAtmosphericModel::VolcanicTypeProfile::BACKGROUND_STRATO_MODERATE_VOLC));

	Ptr<QdlFsoLowtranAtmosphericModel> bshv = CreateObject<QdlFsoLowtranAtmosphericModel>();
	bshv->SetAttribute("VolcanicTypeProfile",
					   EnumValue(QdlFsoLowtranAtmosphericModel::VolcanicTypeProfile::BACKGROUND_STRATO_HIGH_VOLC));

	Ptr<QdlFsoLowtranAtmosphericModel> fvev = CreateObject<QdlFsoLowtranAtmosphericModel>();
	fvev->SetAttribute("VolcanicTypeProfile",
					   EnumValue(QdlFsoLowtranAtmosphericModel::VolcanicTypeProfile::FRESH_VOLC_EXTREME_VOLC));

	csvFile6 << "meters" << DELIM << "Stratospheric background" << DELIM
			 << "Aged volcanic type/moderate volcanic profile" << DELIM << "Fresh volcanic type/high volcanic profile"
			 << DELIM << "Aged volcanic type/high volcanic profile" << DELIM
			 << "Fresh volcanic type/moderate volcanic profile" << DELIM
			 << "Background stratospheric type/moderate volcanic profile" << DELIM
			 << "Background stratospheric type/high volcanic profile" << DELIM
			 << "Fresh volcanic type/extreme volcanic profile" << std::endl;

	init_dist = 1000;
	for (double distance = init_dist; distance <= 40000; distance += init_dist)
	{
		context.pathLength = context.rxAltitude = distance;

		double res_str_bg = str_bg->GetAtmosphericTransmissivity(context);
		double res_avmv = avmv->GetAtmosphericTransmissivity(context);
		double res_fvhv = fvhv->GetAtmosphericTransmissivity(context);
		double res_avhv = avhv->GetAtmosphericTransmissivity(context);
		double res_fvmv = fvmv->GetAtmosphericTransmissivity(context);
		double res_bsmv = bsmv->GetAtmosphericTransmissivity(context);
		double res_bshv = bshv->GetAtmosphericTransmissivity(context);
		double res_fvev = fvev->GetAtmosphericTransmissivity(context);

		csvFile6 << (int)distance << DELIM << res_str_bg << DELIM << res_avmv << DELIM << res_fvhv << DELIM << res_avhv
				 << DELIM << res_fvmv << DELIM << res_bsmv << DELIM << res_bshv << DELIM << res_fvev << std::endl;
	}
	csvFile6.close();

	std::ofstream csvFile7("contrib/qdlfso/validation/qdl_fso_lowtran_slant_to_space.csv");
	context.scenario = QdlFsoScenario::Uplink;
	context.wavelength = wavelength_m;
	context.tx = txTerminal;
	context.rx = rxTerminal;
	context.txAltitude = 0;
	Config::SetDefault("ns3::QdlFsoLowtranAtmosphericModel::AtmosphericModel",
					   EnumValue(QdlFsoLowtranAtmosphericModel::AtmosphericModel::MIDLATITUDE_SUMMER));
	Config::SetDefault("ns3::QdlFsoLowtranAtmosphericModel::AerosolType",
					   EnumValue(QdlFsoLowtranAtmosphericModel::AerosolType::RURAL_23_KM));
	Config::SetDefault("ns3::QdlFsoLowtranAtmosphericModel::CloudRainModel",
					   EnumValue(QdlFsoLowtranAtmosphericModel::CloudRainModel::NO_CLOUDS));
	Config::SetDefault("ns3::QdlFsoLowtranAtmosphericModel::VolcanicTypeProfile",
					   EnumValue(QdlFsoLowtranAtmosphericModel::VolcanicTypeProfile::FRESH_VOLC_MODERATE_VOLC));

	Ptr<QdlFsoLowtranAtmosphericModel> model = CreateObject<QdlFsoLowtranAtmosphericModel>();

	csvFile7 << "height" << DELIM << "0°" << DELIM << "30°" << DELIM << "45°" << DELIM << "60°" << std::endl;

	init_dist = 10;
	for (double height = init_dist; height <= 200000; height *= 1.2)
	{
		context.rxAltitude = height;
		context.zenithAngle = 0;
		context.pathLength = std::abs(height / std::cos(context.zenithAngle));
		double zero = model->GetAtmosphericTransmissivity(context);

		context.zenithAngle = M_PI / 6;
		context.pathLength = std::abs(height / std::cos(context.zenithAngle));
		double one_sixth = model->GetAtmosphericTransmissivity(context);

		context.zenithAngle = M_PI / 4;
		context.pathLength = std::abs(height / std::cos(context.zenithAngle));
		double one_fourth = model->GetAtmosphericTransmissivity(context);

		context.zenithAngle = M_PI / 3;
		context.pathLength = std::abs(height / std::cos(context.zenithAngle));
		double one_third = model->GetAtmosphericTransmissivity(context);

		csvFile7 << height << DELIM << zero << DELIM << one_sixth << DELIM << one_fourth << DELIM << one_third
				 << std::endl;
	}
	csvFile7.close();

	std::cout << "Program terminated normally." << std::endl;
	return 0;
}
