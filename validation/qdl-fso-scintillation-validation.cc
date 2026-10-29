/*
 * QDL-FSO Gamma-Gamma scintillation model validation
 *
 * Draws many irradiance samples per case (downlink: Rytov variance 0.1, 2, 25; uplink: 0.06, 2, 5)
 * and compares the Monte Carlo histograms against the theoretical PDFs, for both the raw
 * irradiance and the standardized log-irradiance. Raw-irradiance MAE is asserted below 0.03.
 *
 * We try to replicate figures in sections 9.10.1 and 9.10.2 of L. C. Andrews and R. L. Phillips, "Laser Beam
 * Propagation through Random Media".
 *
 * Output: contrib/qdlfso/validation/qdl_fso_scintillation_validation.csv
 */

#include "ns3/core-module.h"
#include "ns3/qdl-fso-optical-terminal.h"
#include "ns3/qdl-fso-propagation-loss-model.h"
#include "ns3/qdl-fso-scintillation-model.h"

#include <boost/math/special_functions/bessel.hpp>
#include <boost/math/special_functions/gamma.hpp>
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

// Number of Monte Carlo irradiance samples drawn per test case
constexpr uint32_t NUM_SAMPLES = 100000000;

// Histogram settings for the raw (normalized) irradiance PDF: bins cover [0, MAX_IRRADIANCE)
constexpr int NUM_IRRADIANCE_BINS = 100;
constexpr double MAX_IRRADIANCE = 5.0;

// Histogram settings for the standardized log-irradiance PDF: bins cover [X_MIN, X_MAX)
constexpr int NUM_LOG_IRRADIANCE_BINS = 200;
constexpr double X_MIN = -8.0;
constexpr double X_MAX = 5.0;

// Mean, variance and standard deviation of ln(I) for a Gamma-Gamma distribution
struct LogIrradianceParameters
{
	double mean;
	double variance;
	double standardDeviation;
};

// Analytical mean and variance of ln(I) for Gamma-Gamma(alpha, beta), using digamma/trigamma:
//   E[ln I]   = psi(alpha) - ln(alpha) + psi(beta) - ln(beta)
//   Var[ln I] = psi1(alpha) + psi1(beta)
LogIrradianceParameters
ComputeLogIrradianceParameters(double alpha, double beta)
{
	double mean = boost::math::digamma(alpha) - std::log(alpha) + boost::math::digamma(beta) - std::log(beta);

	double variance = boost::math::polygamma(1, alpha) + boost::math::polygamma(1, beta);

	return {mean, variance, std::sqrt(variance)};
}

// Maps an irradiance sample I to the normalized variable x = f(ln I).
// Uplink uses the spherical-wave form (ln I + s^2/2) / s; downlink uses the usual (ln I - mu) / sigma.
double
ComputeNormalizedLogIrradiance(double irradiance, QdlFsoScenario scenario, const LogIrradianceParameters& parameters)
{
	NS_ASSERT_MSG(irradiance > 0.0, "Irradiance must be positive.");

	const double logI = std::log(irradiance);

	if (scenario == QdlFsoScenario::Uplink)
	{
		// Spherical-wave normalization:
		// x = (ln(I) + 0.5 * s^2) / s
		// where s^2 = Var[ln(I)]

		return (logI + 0.5 * parameters.variance) / parameters.standardDeviation;
	}

	// Downlink / standard normalization:
	// x = (ln(I) - mu) / sigma

	return (logI - parameters.mean) / parameters.standardDeviation;
}

// Theoretical PDF of the normalized log-irradiance x. Inverts the mapping above to get I,
// evaluates the Gamma-Gamma PDF p_I(I), and applies the change-of-variables Jacobian |dI/dx|.
double
CalculateNormalizedLogIrradiancePdf(double x,
									double alpha,
									double beta,
									QdlFsoScenario scenario,
									Ptr<QdlFsoGammaGammaScintillationModel> scintillationModel,
									const LogIrradianceParameters& parameters)
{
	double logI;

	if (scenario == QdlFsoScenario::Uplink)
	{
		// Spherical-wave normalization:
		// x = (ln(I) + 0.5 * s^2) / s
		// ln(I) = s * x - 0.5 * s^2

		logI = parameters.standardDeviation * x - 0.5 * parameters.variance;
	}
	else
	{
		// Standard normalization:
		// x = (ln(I) - mu) / sigma
		// ln(I) = mu + sigma * x

		logI = parameters.mean + parameters.standardDeviation * x;
	}

	const double irradiance = std::exp(logI);

	const double irradiancePdf = scintillationModel->GetTheoreticalPdf(irradiance, alpha, beta);

	// p_X(x) = |dI/dx| p_I(I) = sigma * I * p_I(I)

	return parameters.standardDeviation * irradiance * irradiancePdf;
}

// Runs one validation case: samples the model, builds empirical histograms, compares them with
// the theoretical PDFs, and appends both to the CSV (raw irradiance and standardized log).
void
RunValidation(const QdlFsoPropagationContext& context,
			  double rytovVariance,
			  Ptr<QdlFsoGammaGammaScintillationModel> scintillationModel,
			  std::ofstream& csvFile)
{
	// Gamma-Gamma shape parameters derived from the channel and Rytov variance
	auto [alpha, beta] = scintillationModel->ComputeAlphaBeta(context, rytovVariance);

	std::cout << std::endl;
	std::cout << "========================================================" << std::endl;
	std::cout << " Gamma-Gamma Scintillation Validation\n";
	std::cout << "========================================================" << std::endl;
	std::cout << "Scenario       : " << context.scenario << std::endl;
	std::cout << "Rytov variance : " << rytovVariance << std::endl;
	std::cout << "Alpha          : " << alpha << std::endl;
	std::cout << "Beta           : " << beta << std::endl;
	std::cout << "Samples        : " << NUM_SAMPLES << std::endl;

	// ----------------------------------------------------
	// Monte Carlo sampling
	// ----------------------------------------------------

	const LogIrradianceParameters logIrradianceParameters = ComputeLogIrradianceParameters(alpha, beta);

	const double binWidth = MAX_IRRADIANCE / NUM_IRRADIANCE_BINS;
	const double xBinWidth = (X_MAX - X_MIN) / NUM_LOG_IRRADIANCE_BINS;

	std::vector<uint32_t> histogram(NUM_IRRADIANCE_BINS, 0);
	std::vector<uint32_t> xHistogram(NUM_LOG_IRRADIANCE_BINS, 0);

	for (uint32_t i = 0; i < NUM_SAMPLES; ++i)
	{
		const double irradiance = scintillationModel->GetIrradiance(context, rytovVariance);

		// Raw irradiance histogram
		if (irradiance >= 0.0 && irradiance < MAX_IRRADIANCE)
		{
			const uint32_t bin = static_cast<uint32_t>(irradiance / binWidth);
			if (bin < histogram.size())
			{
				++histogram[bin];
			}
		}

		// Standardized log-irradiance histogram
		if (irradiance > 0.0)
		{
			const double x = ComputeNormalizedLogIrradiance(irradiance, context.scenario, logIrradianceParameters);

			if (x >= X_MIN && x < X_MAX)
			{
				const uint32_t xBin = static_cast<uint32_t>((x - X_MIN) / xBinWidth);
				if (xBin < xHistogram.size())
				{
					++xHistogram[xBin];
				}
			}
		}
	}

	double totalAbsoluteError = 0.0;
	double totalSquaredError = 0.0;

	// Compare empirical and theoretical PDF at each bin center, accumulate error, write CSV rows
	for (int i = 0; i < NUM_IRRADIANCE_BINS; ++i)
	{
		double binCenter = (i + 0.5) * binWidth;

		// Empirical density = count / (total samples * bin width)
		double empiricalPdf = static_cast<double>(histogram[i]) / (NUM_SAMPLES * binWidth);

		double theoreticalPdf = scintillationModel->GetTheoreticalPdf(binCenter, alpha, beta);

		double absoluteDifference = std::abs(empiricalPdf - theoreticalPdf);

		totalAbsoluteError += absoluteDifference;
		totalSquaredError += absoluteDifference * absoluteDifference;

		csvFile << context.scenario << ","
				<< "raw_irradiance" << "," << rytovVariance << "," << alpha << "," << beta << "," << binCenter << ","
				<< empiricalPdf << "," << theoreticalPdf << "\n";
	}

	// Mean absolute error and root-mean-square error across bins
	double mae = totalAbsoluteError / NUM_IRRADIANCE_BINS;

	double rmse = std::sqrt(totalSquaredError / NUM_IRRADIANCE_BINS);

	std::cout << std::endl;
	std::cout << "Raw irradiance validation:" << std::endl;
	std::cout << "  MAE  = " << mae << std::endl;
	std::cout << "  RMSE = " << rmse << std::endl;

	// Pass/fail criterion for the raw irradiance PDF
	NS_ASSERT_MSG(mae < 0.03,
				  "Validation failed: Monte Carlo empirical PDF "
				  "deviates significantly from theory.");

	// Write empirical vs. theoretical PDF of x at each bin center (no pass/fail check here)
	for (int i = 0; i < NUM_LOG_IRRADIANCE_BINS; ++i)
	{
		double x = X_MIN + (i + 0.5) * xBinWidth;

		double empiricalPdf = static_cast<double>(xHistogram[i]) / (NUM_SAMPLES * xBinWidth);

		double theoreticalPdf = CalculateNormalizedLogIrradiancePdf(x,
																	alpha,
																	beta,
																	context.scenario,
																	scintillationModel,
																	logIrradianceParameters);

		csvFile << context.scenario << ","
				<< "standardized_log" << "," << rytovVariance << "," << alpha << "," << beta << "," << x << ","
				<< empiricalPdf << "," << theoreticalPdf << "\n";
	}

	std::cout << "Validation successful.\n";
}

} // namespace

int
main(int argc, char* argv[])
{
	const std::string outFile = "contrib/qdlfso/validation/qdl_fso_scintillation_validation.csv";

	std::ofstream csvFile(outFile);

	NS_ASSERT_MSG(csvFile.is_open(), "Unable to open output CSV file.");

	// CSV header: one row per histogram bin, for both PDF types
	csvFile << "scenario,"
			<< "type,"
			<< "rytov,"
			<< "alpha,"
			<< "beta,"
			<< "x_val,"
			<< "mc_pdf,"
			<< "theory_pdf\n";

	// ----------------------------------------------------
	// Simulation parameters
	// ----------------------------------------------------

	const double wavelength = 1550e-9;
	const double pathLength = 600000.0;
	const double rxAperture = 0.10; // 10 cm aperture

	// Test cases: {scenario, Rytov variance}, covering weak to strong turbulence
	const std::vector<std::pair<QdlFsoScenario, double>> tests = {{QdlFsoScenario::Downlink, 0.1},
																  {QdlFsoScenario::Downlink, 2.0},
																  {QdlFsoScenario::Downlink, 25.0},
																  {QdlFsoScenario::Uplink, 0.06},
																  {QdlFsoScenario::Uplink, 2.0},
																  {QdlFsoScenario::Uplink, 5.0}};

	// ----------------------------------------------------
	// Create receiver
	// ----------------------------------------------------

	Ptr<QdlFsoOpticalTerminal> rx = CreateObject<QdlFsoOpticalTerminal>();

	rx->SetRxApertureDiameter(rxAperture);

	// ----------------------------------------------------
	// Create scintillation model
	// ----------------------------------------------------

	Ptr<QdlFsoGammaGammaScintillationModel> scintillationModel = CreateObject<QdlFsoGammaGammaScintillationModel>();

	// Disable aperture averaging so samples follow the plain Gamma-Gamma distribution
	scintillationModel->SetAttribute("EnableApertureAveraging", BooleanValue(false));

	// Fixed RNG stream for reproducible results
	scintillationModel->AssignStreams(12345);

	// ----------------------------------------------------
	// Run validation cases
	// ----------------------------------------------------

	for (std::pair<QdlFsoScenario, double> test : tests)
	{
		QdlFsoPropagationContext context;

		context.scenario = test.first;
		context.wavelength = wavelength;
		context.pathLength = pathLength;
		context.rx = rx;
		RunValidation(context, test.second, scintillationModel, csvFile);
	}

	csvFile.close();

	std::cout << std::endl;
	std::cout << "========================================================" << std::endl;
	std::cout << "qdl-fso-scintillation validation run completed successfully. Output written to: " << outFile
			  << std::endl;
	std::cout << "========================================================" << std::endl;

	return 0;
}
