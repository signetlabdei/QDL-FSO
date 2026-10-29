/*
 * QDL-FSO Cn2 model validation
 *
 * Evaluates the Hufnagel-Valley Cn2(h) profile for the downlink and uplink scenarios
 * (zenith = 0, h = 2 m to 600 km) over a grid of wind speeds (21, 41, 61 m/s) and ground-level
 * Cn2(0) values (1.7e-14, 1.7e-13, 1.7e-12 m^-2/3), and writes the profiles to a CSV file.
 * The goal is to reproduce the Cn2 vs. altitude figure 12.1 (sec. 12.2.1) from L. C. Andrews and R. L. Phillips,
 * "Laser Beam Propagation through Random Media".
 *
 *  Output: contrib/qdlfso/validation/qdl_fso_cn2_validation.csv
 */

#include "ns3/core-module.h"
#include "ns3/qdl-fso-cn2-model.h"
#include "ns3/qdl-fso-optical-terminal.h"
#include "ns3/qdl-fso-propagation-loss-model.h"

#include <cmath>
#include <iostream>

using namespace ns3;

int
main(int argc, char* argv[])
{
	const std::string outFile = "contrib/qdlfso/validation/qdl_fso_cn2_validation.csv";
	std::ofstream csvFile(outFile);
	NS_ASSERT_MSG(csvFile.is_open(), "Unable to open output CSV file.");

	csvFile << "scenario,"
			<< "v_wind,"
			<< "Cn20,"
			<< "h,"
			<< "Cn2h\n";

	double wavelength = 1550e-9;
	double zenith = 0.0;
	double h0 = 2.0;
	double H = 600e3;

	QdlFsoPropagationLossModel::SetAltitudeTolerance(90.0);

	// Instantiate Tx Terminal and configure parameters
	Ptr<QdlFsoOpticalTerminal> txTerminal = CreateObject<QdlFsoOpticalTerminal>();
	txTerminal->SetWavelength(wavelength);
	txTerminal->SetTxBeamWaist(0.15); // 15 cm radius

	// Instantiate Rx Terminal and configure parameters
	Ptr<QdlFsoOpticalTerminal> rxTerminal = CreateObject<QdlFsoOpticalTerminal>();
	rxTerminal->SetWavelength(wavelength);
	rxTerminal->SetRxApertureDiameter(0.3); // 30 cm diameter

	// Instantiate and populate the QdlFsoPropagationContext struct

	QdlFsoPropagationContext context_down;
	context_down.scenario = QdlFsoScenario::Downlink;
	context_down.wavelength = wavelength;
	context_down.tx = txTerminal;
	context_down.rx = rxTerminal;
	context_down.zenithAngle = zenith;
	context_down.rxAltitude = h0;
	context_down.txAltitude = H;
	context_down.pathLength =
		std::abs(context_down.rxAltitude - context_down.txAltitude) / std::cos(context_down.zenithAngle);

	QdlFsoPropagationContext context_up;
	context_up.scenario = QdlFsoScenario::Uplink;
	context_up.wavelength = wavelength;
	context_up.tx = txTerminal;
	context_up.rx = rxTerminal;
	context_up.zenithAngle = zenith;
	context_up.txAltitude = h0;
	context_up.rxAltitude = H;
	context_up.pathLength = std::abs(context_up.rxAltitude - context_up.txAltitude) / std::cos(context_up.zenithAngle);

	Ptr<QdlFsoCn2Model> cn2_model = CreateObject<QdlFsoCn2Model>();

	std::vector<std::pair<double, double>> tests =
		{{21.0, 1.7e-14}, {10.0, 1.7e-14}, {30.0, 1.7e-14}, {21.0, 1.7e-13}, {10.0, 1.7e-13}, {30.0, 1.7e-13}};
	// {21.0, 1.7e-12},
	// {10.0, 1.7e-12},
	// {30.0, 1.7e-12}};

	for (std::pair<double, double> test : tests)
	{
		double v = test.first;
		double c = test.second;

		cn2_model->SetVWind(v);
		cn2_model->SetCn20(c);

		std::vector<std::pair<double, double>> cn2h_down = cn2_model->GetCn2ProfileUplinkDownlink(context_down);
		std::vector<std::pair<double, double>> cn2h_up = cn2_model->GetCn2ProfileUplinkDownlink(context_up);

		for (std::pair<double, double> h_ch : cn2h_down)
		{
			csvFile << context_down.scenario << "," << v << "," << c << "," << h_ch.first << "," << h_ch.second << "\n";
		}

		for (std::pair<double, double> h_ch : cn2h_up)
		{
			csvFile << context_up.scenario << "," << v << "," << c << "," << h_ch.first << "," << h_ch.second << "\n";
		}
	}

	std::cout << "qdl-fso-cn2 validation run completed successfully. Output written to: " << outFile << std::endl;

	return 0;
}
