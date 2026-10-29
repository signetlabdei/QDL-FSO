/*
 * QDL-FSO beam-wander model validation
 *
 * Computes the Rytov variance and the beam-wander variance for the three scenarios:
 *   - downlink and uplink (zenith = 0, ground terminal at 2 m, other terminal from 100 m to
 *     ~600 km in 5 km steps);
 *   - horizontal (path length from 100 m to 3 km in 100 m steps).
 * Each case is swept over wind speeds (21, 41, 61 m/s) and ground-level Cn2(0) values
 * (1.7e-14, 1.7e-13, 1.7e-12 m^-2/3), and the results are written to a CSV file.
 *
 * Output: contrib/qdlfso/validation/qdl_fso_beam_wander_validation.csv
 */

#include "ns3/core-module.h"
#include "ns3/qdl-fso-beam-wander-model.h"
#include "ns3/qdl-fso-cn2-model.h"
#include "ns3/qdl-fso-optical-terminal.h"
#include "ns3/qdl-fso-propagation-loss-model.h"
#include "ns3/qdl-fso-rytov-model.h"

#include <cmath>
#include <iostream>

using namespace ns3;

const double H_MIN = 100.0;
const double H_MAX = 600e3 + 100.0;
const double DELTA_H = 5000.0;

const double L_MIN = 100.0;
const double L_MAX = 3e3;
const double DELTA_L = 100.0;

int
main(int argc, char* argv[])
{
	const std::string outFile = "contrib/qdlfso/validation/qdl_fso_beam_wander_validation.csv";
	std::ofstream csvFile(outFile);
	NS_ASSERT_MSG(csvFile.is_open(), "Unable to open output CSV file at path: " << outFile);

	// CSV Header
	csvFile << "scenario,"
			<< "v_wind,"
			<< "Cn20,"
			<< "tx_alt,"
			<< "rx_alt,"
			<< "pathlength,"
			<< "rytov_var,"
			<< "wander_var,"
			<< "root_wander_var\n";

	const double wavelength = 1550e-9;
	const double zenith = 0.0;
	const double h0 = 2.0;

	QdlFsoPropagationLossModel::SetAltitudeTolerance(90.0);

	// Instantiate Optical Terminals
	Ptr<QdlFsoOpticalTerminal> txTerminal = CreateObject<QdlFsoOpticalTerminal>();
	txTerminal->SetWavelength(wavelength);
	txTerminal->SetTxBeamWaist(0.01); // 10 cm waist radius

	Ptr<QdlFsoOpticalTerminal> rxTerminal = CreateObject<QdlFsoOpticalTerminal>();
	rxTerminal->SetWavelength(wavelength);
	rxTerminal->SetRxApertureDiameter(0.3); // 30 cm aperture diameter

	// Propagation Contexts
	QdlFsoPropagationContext context_hor;
	context_hor.scenario = QdlFsoScenario::Horizontal;
	context_hor.wavelength = wavelength;
	context_hor.tx = txTerminal;
	context_hor.rx = rxTerminal;
	context_hor.pathLength = L_MIN;
	context_hor.txAltitude = 20.0;
	context_hor.rxAltitude = h0;

	QdlFsoPropagationContext context_down;
	context_down.scenario = QdlFsoScenario::Downlink;
	context_down.wavelength = wavelength;
	context_down.tx = txTerminal;
	context_down.rx = rxTerminal;
	context_down.zenithAngle = zenith;
	context_down.rxAltitude = h0;

	QdlFsoPropagationContext context_up;
	context_up.scenario = QdlFsoScenario::Uplink;
	context_up.wavelength = wavelength;
	context_up.tx = txTerminal;
	context_up.rx = rxTerminal;
	context_up.zenithAngle = zenith;
	context_up.txAltitude = h0;

	// Turbulence & Propagation Models
	Ptr<QdlFsoCn2Model> cn2_model = CreateObject<QdlFsoCn2Model>();

	Ptr<QdlFsoRytovModel> rytov_model = CreateObject<QdlFsoRytovModel>();
	rytov_model->SetCn2Model(cn2_model);

	Ptr<QdlFsoBeamWanderModel> beam_wander_model = CreateObject<QdlFsoBeamWanderModel>();
	beam_wander_model->SetCn2Model(cn2_model);

	// Vector of test conditions: {v_wind (m/s), Cn2_0 (m^(-2/3))}
	const std::vector<std::pair<double, double>> tests = {{21.0, 1.7e-14},
														  {41.0, 1.7e-14},
														  {61.0, 1.7e-14},
														  {21.0, 1.7e-13},
														  {41.0, 1.7e-13},
														  {61.0, 1.7e-13},
														  {21.0, 1.7e-12},
														  {41.0, 1.7e-12},
														  {61.0, 1.7e-12}};

	for (std::pair<double, double> test : tests)
	{
		double v = test.first;
		double c = test.second;

		cn2_model->SetVWind(v);
		cn2_model->SetCn20(c);

		// Uplink and Downlink validation
		for (double H = H_MIN; H <= H_MAX + 1e-6; H += DELTA_H)
		{
			context_down.txAltitude = H;
			context_down.pathLength =
				std::abs(context_down.rxAltitude - context_down.txAltitude) / std::cos(context_down.zenithAngle);

			context_up.rxAltitude = H;
			context_up.pathLength =
				std::abs(context_up.rxAltitude - context_up.txAltitude) / std::cos(context_up.zenithAngle);

			double rytov_down = rytov_model->GetRytovVarianceUplinkDownlink(context_down);
			double rytov_up = rytov_model->GetRytovVarianceUplinkDownlink(context_up);

			double b_w_var_down = beam_wander_model->GetWanderVarianceUplinkDownlink(context_down, rytov_down);
			double b_w_var_up = beam_wander_model->GetWanderVarianceUplinkDownlink(context_up, rytov_up);

			csvFile << context_down.scenario << "," << v << "," << c << "," << context_down.txAltitude << ","
					<< context_down.rxAltitude << "," << context_down.pathLength << "," << rytov_down << ","
					<< b_w_var_down << "," << sqrt(b_w_var_down) << "\n";

			csvFile << context_up.scenario << "," << v << "," << c << "," << context_up.txAltitude << ","
					<< context_up.rxAltitude << "," << context_up.pathLength << "," << rytov_up << "," << b_w_var_up
					<< "," << sqrt(b_w_var_up) << "\n";
		}

		for (double L = L_MIN; L <= L_MAX + 1e-6; L += DELTA_L)
		{
			context_hor.pathLength = L;

			double rytov_hor = rytov_model->GetRytovVarianceHorizontal(context_hor);
			double b_w_var_hor = beam_wander_model->GetWanderVarianceHorizontal(context_hor, rytov_hor);

			csvFile << context_hor.scenario << "," << v << "," << c << "," << context_hor.txAltitude << ","
					<< context_hor.rxAltitude << "," << context_hor.pathLength << "," << rytov_hor << "," << b_w_var_hor
					<< "," << sqrt(b_w_var_hor) << "\n";
		}
	}

	std::cout << "qdl-fso-beam-wander validation run completed successfully. Output written to: " << outFile
			  << std::endl;

	return 0;
}
