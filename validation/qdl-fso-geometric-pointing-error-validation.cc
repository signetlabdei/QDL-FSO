/*
 * QDL-FSO geometric pointing-error model validation
 *
 * 1. Reproduces Figs. 5 and 8 of L. Miao et al., "New Results for the Pointing Errors Model in
 *    Two Asymptotic Cases" (Farid and Miao smooth/step pointing efficiency vs. normalized
 *    displacement), and checks that the curves are consistent (MAE < 0.03) across scenarios
 *    (downlink, uplink, horizontal) and receiver apertures.
 * 2. Compares the empirical PDF of the sampled radial displacement (Monte Carlo) with the
 *    analytical Rayleigh PDF for each scenario.
 * 3. Checks that the displacement is always 0 when the tracking efficiency is 1.
 *
 * Output:
 * 	- contrib/qdlfso/validation/qdl_fso_geometric_pointing_error_validation_1.csv
 * 	- contrib/qdlfso/validation/qdl_fso_geometric_pointing_error_validation_2.csv
 */

#include "ns3/core-module.h"
#include "ns3/qdl-fso-geometric-pointing-error-model.h"
#include "ns3/qdl-fso-optical-terminal.h"
#include "ns3/qdl-fso-propagation-loss-model.h"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

using namespace ns3;

namespace
{

constexpr uint32_t NUM_SAMPLES = 10000;

constexpr uint32_t NUM_SAMPLES_RAYLEIGH = 1000000;

constexpr double FARID_X_MIN = -10;
constexpr double FARID_X_MAX = 10;
const std::vector<double> FARID_WST_RRX = {2.0, 4.0, 6.0};

constexpr double MIAO_X_MIN = 0;
constexpr double MIAO_X_MAX = 2;
const std::vector<double> MIAO_WST_RRX = {0.05, 0.1, 0.2, 0.3};

struct PointingTests
{
	QdlFsoScenario scenario{QdlFsoScenario::Downlink};
	double Drx{0.0};
};

void
RunValidation(const QdlFsoPropagationContext& context,
			  std::pair<QdlFsoScenario, double> test,
			  Ptr<QdlFsoGeometricPointingErrorModel> pointingErrorModel,
			  std::ofstream& csvFile,
			  std::vector<std::vector<double>>& prevFarid,
			  std::vector<std::vector<double>>& prevMiaoSmooth,
			  std::vector<std::vector<double>>& prevMiaoStep,
			  bool hasPreviousTest)
{
	std::cout << std::endl;
	std::cout << "========================================================" << std::endl;
	std::cout << " Pointing Error Validation\n";
	std::cout << "========================================================" << std::endl;
	std::cout << "Scenario       : " << context.scenario << std::endl;
	std::cout << "Drx            : " << test.second << std::endl;
	std::cout << "Samples        : " << NUM_SAMPLES << std::endl;

	// Farid
	for (uint32_t w = 0; w < FARID_WST_RRX.size(); ++w)
	{
		double WST_RRX = FARID_WST_RRX[w];

		double W_ST = WST_RRX * test.second / 2.0;

		double step = (FARID_X_MAX - FARID_X_MIN) / (NUM_SAMPLES - 1);

		double totalAbsoluteError = 0.0;
		double totalSquaredError = 0.0;

		for (uint32_t i = 0; i < NUM_SAMPLES; ++i)
		{
			double x = FARID_X_MIN + i * step;

			double r = x * test.second / 2.0;

			double h_farid = pointingErrorModel->GetPointingEfficiencyFarid(test.second, r, W_ST);

			if (hasPreviousTest)
			{
				double absoluteDifference = std::abs(prevFarid[w][i] - h_farid);

				totalAbsoluteError += absoluteDifference;
				totalSquaredError += absoluteDifference * absoluteDifference;
			}

			// Store current result for comparison with next test
			prevFarid[w][i] = h_farid;

			csvFile << "Farid" << "," << context.scenario << "," << x << "," << test.second << "," << r << ","
					<< WST_RRX << "," << W_ST << "," << h_farid << "\n";
		}

		if (hasPreviousTest)
		{
			double mae = totalAbsoluteError / NUM_SAMPLES;

			double rmse = std::sqrt(totalSquaredError / NUM_SAMPLES);

			std::cout << std::endl;
			std::cout << "Farid difference wrt previous test:" << std::endl;
			std::cout << "  WST_RRX = " << WST_RRX << std::endl;
			std::cout << "  MAE      = " << mae << std::endl;
			std::cout << "  RMSE     = " << rmse << std::endl;

			NS_ASSERT_MSG(mae < 0.03, "Validation failed: Farid values should be equal across tests!");
		}
	}

	// Miao
	for (uint32_t w = 0; w < MIAO_WST_RRX.size(); ++w)
	{
		double WST_RRX = MIAO_WST_RRX[w];

		double W_ST = WST_RRX * test.second / 2.0;

		double step = (MIAO_X_MAX - MIAO_X_MIN) / (NUM_SAMPLES - 1);

		double totalAbsoluteErrorSmooth = 0.0;
		double totalSquaredErrorSmooth = 0.0;
		double totalAbsoluteErrorStep = 0.0;
		double totalSquaredErrorStep = 0.0;

		for (uint32_t i = 0; i < NUM_SAMPLES; ++i)
		{
			double x = MIAO_X_MIN + i * step;

			double r = x * test.second / 2.0;

			double h_miao_smooth = pointingErrorModel->GetPointingEfficiencyMiao(test.second, r, W_ST, "smooth");

			if (hasPreviousTest)
			{
				double absoluteDifference = std::abs(prevMiaoSmooth[w][i] - h_miao_smooth);

				totalAbsoluteErrorSmooth += absoluteDifference;
				totalSquaredErrorSmooth += absoluteDifference * absoluteDifference;
			}

			// Store current result for comparison with next test
			prevMiaoSmooth[w][i] = h_miao_smooth;

			csvFile << "Miao - smooth" << "," << context.scenario << "," << x << "," << test.second << "," << r << ","
					<< WST_RRX << "," << W_ST << "," << h_miao_smooth << "\n";

			double h_miao_step = pointingErrorModel->GetPointingEfficiencyMiao(test.second, r, W_ST, "step");

			if (hasPreviousTest)
			{
				double absoluteDifference = std::abs(prevMiaoStep[w][i] - h_miao_step);

				totalAbsoluteErrorStep += absoluteDifference;
				totalSquaredErrorStep += absoluteDifference * absoluteDifference;
			}

			// Store current result for comparison with next test
			prevMiaoStep[w][i] = h_miao_step;

			csvFile << "Miao - step" << "," << context.scenario << "," << x << "," << test.second << "," << r << ","
					<< WST_RRX << "," << W_ST << "," << h_miao_step << "\n";
		}

		if (hasPreviousTest)
		{
			double mae = totalAbsoluteErrorSmooth / NUM_SAMPLES;

			double rmse = std::sqrt(totalSquaredErrorSmooth / NUM_SAMPLES);

			std::cout << std::endl;
			std::cout << "Miao Smooth difference wrt previous test:" << std::endl;
			std::cout << "  WST_RRX = " << WST_RRX << std::endl;
			std::cout << "  MAE      = " << mae << std::endl;
			std::cout << "  RMSE     = " << rmse << std::endl;

			NS_ASSERT_MSG(mae < 0.03, "Validation failed: Miao Smooth values should be equal across tests!");

			mae = totalAbsoluteErrorStep / NUM_SAMPLES;

			rmse = std::sqrt(totalSquaredErrorStep / NUM_SAMPLES);

			std::cout << std::endl;
			std::cout << "Miao Step difference wrt previous test:" << std::endl;
			std::cout << "  WST_RRX = " << WST_RRX << std::endl;
			std::cout << "  MAE      = " << mae << std::endl;
			std::cout << "  RMSE     = " << rmse << std::endl;

			NS_ASSERT_MSG(mae < 0.03, "Validation failed: Miao Step values should be equal across tests!");
		}
	}
}

void
RunValidationRayleigh(const QdlFsoPropagationContext& context,
					  Ptr<QdlFsoGeometricPointingErrorModel> pointingErrorModel,
					  double wander_variance,
					  std::ofstream& csvFile)
{
	// ----------------------------------------------------
	// Monte Carlo sampling
	// ----------------------------------------------------

	std::vector<double> samples(NUM_SAMPLES_RAYLEIGH);

	for (uint32_t i = 0; i < NUM_SAMPLES_RAYLEIGH; ++i)
	{
		samples[i] = pointingErrorModel->SampleRadialDisplacement(context, wander_variance);
	}

	// ----------------------------------------------------
	// Rayleigh distribution parameters
	// ----------------------------------------------------

	const double L = context.pathLength;

	const double sigmaSquared = (std::pow(L * context.tx->GetPointingJitter(), 2.0) + 0.5 * wander_variance) *
								(1.0 - context.rx->GetTrackingEfficiency());

	const double sigma = std::sqrt(sigmaSquared);

	// ----------------------------------------------------
	// Rayleigh PDF validation
	// ----------------------------------------------------

	const double MAX_R = 5.0 * sigma;
	const uint32_t NUM_RAYLEIGH_BINS = 200;

	const double binWidth = MAX_R / NUM_RAYLEIGH_BINS;

	std::vector<uint32_t> histogram(NUM_RAYLEIGH_BINS, 0);

	// ----------------------------------------------------
	// Build histogram
	// ----------------------------------------------------

	for (double r : samples)
	{
		if (r < 0.0 || r >= MAX_R)
		{
			continue;
		}

		uint32_t bin = static_cast<uint32_t>(r / binWidth);

		if (bin < histogram.size())
		{
			++histogram[bin];
		}
	}

	// ----------------------------------------------------
	// Compare empirical PDF against Rayleigh PDF
	// ----------------------------------------------------

	double totalAbsoluteError = 0.0;
	double totalSquaredError = 0.0;

	for (uint32_t i = 0; i < NUM_RAYLEIGH_BINS; ++i)
	{
		const double binCenter = (i + 0.5) * binWidth;

		const double empiricalPdf = static_cast<double>(histogram[i]) / (NUM_SAMPLES_RAYLEIGH * binWidth);

		const double theoreticalPdf =
			(binCenter / sigmaSquared) * std::exp(-binCenter * binCenter / (2.0 * sigmaSquared));

		const double absoluteDifference = std::abs(empiricalPdf - theoreticalPdf);

		totalAbsoluteError += absoluteDifference;

		totalSquaredError += absoluteDifference * absoluteDifference;

		csvFile << context.scenario << ","
				<< "rayleigh" << "," << wander_variance << "," << sigma << "," << binCenter << "," << empiricalPdf
				<< "," << theoreticalPdf << "\n";
	}

	// ----------------------------------------------------
	// Validation metrics
	// ----------------------------------------------------

	const double mae = totalAbsoluteError / NUM_RAYLEIGH_BINS;

	const double rmse = std::sqrt(totalSquaredError / NUM_RAYLEIGH_BINS);

	std::cout << std::endl;
	std::cout << "Rayleigh radial displacement validation:" << std::endl;
	std::cout << "  sigma = " << sigma << std::endl;
	std::cout << "  MAE   = " << mae << std::endl;
	std::cout << "  RMSE  = " << rmse << std::endl;

	NS_ASSERT_MSG(mae < 0.05,
				  "Validation failed: Monte Carlo radial displacement "
				  "deviates significantly from Rayleigh theory.");
}

} // namespace

int
main(int argc, char* argv[])
{
	const std::string outFile1 = "contrib/qdlfso/validation/qdl_fso_geometric_pointing_error_validation_1.csv";

	std::ofstream csvFile1(outFile1);

	NS_ASSERT_MSG(csvFile1.is_open(), "Unable to open output CSV file.");

	csvFile1 << "type,"
			 << "scenario,"
			 << "x_val,"
			 << "D_rx,"
			 << "r,"
			 << "WST_RRX,"
			 << "W_ST,"
			 << "h\n";

	const std::string outFile2 = "contrib/qdlfso/validation/qdl_fso_geometric_pointing_error_validation_2.csv";

	std::ofstream csvFile2(outFile2);

	NS_ASSERT_MSG(csvFile2.is_open(), "Unable to open output CSV file.");

	csvFile2 << "scenario,"
			 << "type,"
			 << "wander_variance,"
			 << "sigma,"
			 << "r,"
			 << "mc_pdf,"
			 << "theory_pdf\n";

	// ----------------------------------------------------
	// Simulation parameters
	// ----------------------------------------------------

	const double wavelength = 1550e-9;
	const double pathLength = 600000.0;

	const std::vector<std::pair<QdlFsoScenario, double>> tests_efficiency = {//{QdlFsoScenario::Downlink, 0.2},
																			 {QdlFsoScenario::Downlink, 0.8},
																			 //  {QdlFsoScenario::Uplink, 0.2},
																			 {QdlFsoScenario::Uplink, 0.8},
																			 //  {QdlFsoScenario::Horizontal, 0.2},
																			 {QdlFsoScenario::Horizontal, 0.8}};

	const std::vector<std::pair<QdlFsoScenario, double>> tests_displacement = {{QdlFsoScenario::Downlink, 0.01},
																			   {QdlFsoScenario::Uplink, 200.0},
																			   {QdlFsoScenario::Horizontal, 0.1}};

	// ----------------------------------------------------
	// Create receiver
	// ----------------------------------------------------

	Ptr<QdlFsoOpticalTerminal> rx = CreateObject<QdlFsoOpticalTerminal>();
	// Here you can put any value in [0,1]
	rx->SetTrackingEfficiency(0.0);

	Ptr<QdlFsoOpticalTerminal> tx = CreateObject<QdlFsoOpticalTerminal>();
	// Here you can put any (small) value
	tx->SetPointingJitter(0.0);

	// ----------------------------------------------------
	// Create pointing error model
	// ----------------------------------------------------

	Ptr<QdlFsoGeometricPointingErrorModel> pointingErrorModel = CreateObject<QdlFsoGeometricPointingErrorModel>();

	pointingErrorModel->AssignStreams(12345);

	// ----------------------------------------------------
	// Run validation cases
	// ----------------------------------------------------

	std::vector<std::vector<double>> prevFarid(FARID_WST_RRX.size(), std::vector<double>(NUM_SAMPLES, 0.0));
	std::vector<std::vector<double>> prevMiaoSmooth(MIAO_WST_RRX.size(), std::vector<double>(NUM_SAMPLES, 0.0));
	std::vector<std::vector<double>> prevMiaoStep(MIAO_WST_RRX.size(), std::vector<double>(NUM_SAMPLES, 0.0));

	bool hasPreviousTest = false;

	for (std::pair<QdlFsoScenario, double> test : tests_efficiency)
	{
		rx->SetRxApertureDiameter(test.second);

		QdlFsoPropagationContext context;

		context.scenario = test.first;
		context.wavelength = wavelength;
		context.pathLength = pathLength;
		context.rx = rx;
		RunValidation(context,
					  test,
					  pointingErrorModel,
					  csvFile1,
					  prevFarid,
					  prevMiaoSmooth,
					  prevMiaoStep,
					  hasPreviousTest);

		hasPreviousTest = true;
	}

	for (std::pair<QdlFsoScenario, double> test : tests_displacement)
	{
		QdlFsoPropagationContext context;

		context.scenario = test.first;
		context.wavelength = wavelength;
		context.pathLength = pathLength;
		context.rx = rx;
		context.tx = tx;

		RunValidationRayleigh(context, pointingErrorModel, test.second, csvFile2);
	}

	csvFile1.close();
	csvFile2.close();

	// Check that if the tracking efficiency is set to 1, then the displacement should be equal to 0.
	rx->SetTrackingEfficiency(1.0);
	QdlFsoPropagationContext context;
	context.scenario = QdlFsoScenario::Downlink;
	context.wavelength = wavelength;
	context.pathLength = pathLength;
	context.rx = rx;
	context.tx = tx;

	double r1 = pointingErrorModel->SampleRadialDisplacement(context, 60.0);

	NS_ASSERT_MSG(
		r1 == 0.0,
		"If the tracking efficiency is set to 1, then the radial displacement should be always equal to 0. r = " << r1);

	std::cout << std::endl;
	std::cout << "========================================================" << std::endl;
	std::cout << "qdl-fso-geometric-and-pointing-error validation run completed successfully." << std::endl;
	std::cout << "Output written to: " << outFile1 << " and " << outFile2 << std::endl;
	std::cout << "========================================================" << std::endl;

	return 0;
}
