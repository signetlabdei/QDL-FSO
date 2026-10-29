#include "qdl-fso-scintillation-model.h"

#include "ns3/boolean.h"
#include "ns3/log.h"
#include "ns3/qdl-fso-optical-terminal.h"
#include "ns3/qdl-fso-propagation-loss-model.h"
#include "ns3/string.h"

#include <boost/math/special_functions/bessel.hpp>
#include <boost/math/special_functions/gamma.hpp>
#include <cmath>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("QdlFsoScintillationModel");

NS_OBJECT_ENSURE_REGISTERED(QdlFsoScintillationModel);

TypeId
QdlFsoScintillationModel::GetTypeId()
{
	static TypeId tid = TypeId("ns3::QdlFsoScintillationModel").SetParent<Object>().SetGroupName("QdlFso");

	return tid;
}

QdlFsoScintillationModel::QdlFsoScintillationModel()
{
	NS_LOG_FUNCTION(this);
}

QdlFsoScintillationModel::~QdlFsoScintillationModel()
{
	NS_LOG_FUNCTION(this);
}

void
QdlFsoScintillationModel::DoDispose()
{
	NS_LOG_FUNCTION(this);
	Object::DoDispose();
}

double
QdlFsoScintillationModel::GetIrradiance(const QdlFsoPropagationContext& context, double rytov_variance)
{
	NS_LOG_FUNCTION(this);
	return DoGetIrradiance(context, rytov_variance);
}

TypeId
QdlFsoGammaGammaScintillationModel::GetTypeId()
{
	static TypeId tid =
		TypeId("ns3::QdlFsoGammaGammaScintillationModel")
			.SetParent<Object>()
			.SetGroupName("QdlFso")
			.AddConstructor<QdlFsoGammaGammaScintillationModel>()
			.AddAttribute("GammaRvLargeA",
						  "Gamma RV for large-scale turbulence eddies.",
						  StringValue("ns3::GammaRandomVariable[Alpha=1.0|Beta=1.0]"),
						  MakePointerAccessor(&QdlFsoGammaGammaScintillationModel::m_gammaRvLargeA),
						  MakePointerChecker<GammaRandomVariable>())
			.AddAttribute("GammaRvSmallB",
						  "Gamma RV for small-scale turbulence eddies.",
						  StringValue("ns3::GammaRandomVariable[Alpha=1.0|Beta=1.0]"),
						  MakePointerAccessor(&QdlFsoGammaGammaScintillationModel::m_gammaRvSmallB),
						  MakePointerChecker<GammaRandomVariable>())
			.AddAttribute("EnableApertureAveraging",
						  "Enable or Disable aperture averaging.",
						  BooleanValue(true),
						  MakeBooleanAccessor(&QdlFsoGammaGammaScintillationModel::m_enableApertureAveraging),
						  MakeBooleanChecker());

	return tid;
}

QdlFsoGammaGammaScintillationModel::QdlFsoGammaGammaScintillationModel()
{
	NS_LOG_FUNCTION(this);
	m_gammaRvLargeA = CreateObject<GammaRandomVariable>();
	m_gammaRvSmallB = CreateObject<GammaRandomVariable>();
	// By default the aperture averaging is always on
	m_enableApertureAveraging = true;
}

QdlFsoGammaGammaScintillationModel::~QdlFsoGammaGammaScintillationModel()
{
	NS_LOG_FUNCTION(this);
}

void
QdlFsoGammaGammaScintillationModel::DoDispose()
{
	NS_LOG_FUNCTION(this);
	m_gammaRvLargeA = nullptr;
	m_gammaRvSmallB = nullptr;
	Object::DoDispose();
}

int64_t
QdlFsoGammaGammaScintillationModel::AssignStreams(int64_t stream)
{
	NS_LOG_FUNCTION(this << stream);
	m_gammaRvLargeA->SetStream(stream);
	m_gammaRvSmallB->SetStream(stream + 1);
	return 2;
}

std::pair<double, double>
QdlFsoGammaGammaScintillationModel::ComputeAlphaBeta(const QdlFsoPropagationContext& context, double rytov_variance)
{
	NS_LOG_FUNCTION(this);

	// Compute aperture averaging factor d = sqrt(k * D_rx^2 / (4 * L))
	double d2 = 0.0;

	NS_LOG_DEBUG("m_enableApertureAveraging is " << m_enableApertureAveraging);
	if (m_enableApertureAveraging)
	{
		double k = 2.0 * M_PI / context.wavelength;
		double Drx = context.rx->GetRxApertureDiameter();
		double L = context.pathLength;
		d2 = (k * Drx * Drx) / (4.0 * L);
	}

	// Calculate large-scale (sigma2_x) and small-scale (sigma2_y) variances
	double sigma2_x = 0.0;
	double sigma2_y = 0.0;

	double sigma2_R_6_5 = std::pow(rytov_variance, 6.0 / 5.0);

	if (context.scenario == QdlFsoScenario::Uplink)
	{
		// Spherical wave formulation (Uplink)
		double num_x = 0.49 * rytov_variance;
		double den_x = std::pow(1.0 + 0.18 * d2 + 0.56 * sigma2_R_6_5, 7.0 / 6.0);
		sigma2_x = std::exp(num_x / den_x) - 1.0;
	}
	else if (context.scenario == QdlFsoScenario::Horizontal || context.scenario == QdlFsoScenario::Downlink)
	{
		// Plane wave formulation (Horizontal & Downlink)
		double num_x = 0.49 * rytov_variance;
		double den_x = std::pow(1.0 + 0.65 * d2 + 1.11 * sigma2_R_6_5, 7.0 / 6.0);
		sigma2_x = std::exp(num_x / den_x) - 1.0;
	}
	else
	{
		NS_ABORT_MSG("context.scenario " << context.scenario << " is not supported!");
	}

	// Small-scale variance has identical functional form across all scenarios
	double num_y = 0.51 * rytov_variance * std::pow(1.0 + 0.69 * sigma2_R_6_5, -5.0 / 6.0);
	double den_y = 1.0 + 0.90 * d2 + 0.62 * d2 * sigma2_R_6_5;
	sigma2_y = std::exp(num_y / den_y) - 1.0;

	// Calculate Gamma-Gamma parameters alpha and beta
	// Bound variances to avoid division by zero
	sigma2_x = std::max(1e-9, sigma2_x);
	sigma2_y = std::max(1e-9, sigma2_y);

	double alpha = 1.0 / sigma2_x;
	double beta = 1.0 / sigma2_y;

	std::pair<double, double> ab = std::make_pair(alpha, beta);

	return ab;
}

double
QdlFsoGammaGammaScintillationModel::DoGetIrradiance(const QdlFsoPropagationContext& context, double rytov_variance)
{
	NS_LOG_FUNCTION(this << rytov_variance);

	// Turbulence-free limit safeguard (Cn2 -> 0 => Rytov variance -> 0)
	if (rytov_variance < 1e-12)
	{
		return 1.0;
	}

	std::pair<double, double> ab = ComputeAlphaBeta(context, rytov_variance);

	double alpha = ab.first;
	double beta = ab.second;

	// Sample X ~ Gamma(alpha, 1/alpha) and Y ~ Gamma(beta, 1/beta)
	double X = m_gammaRvLargeA->GetValue(alpha, 1.0 / alpha);
	double Y = m_gammaRvSmallB->GetValue(beta, 1.0 / beta);

	double I = X * Y;

	NS_LOG_DEBUG("eta_scint = " << I);

	return I;
}

// This function is only for validation and testing. It is not meant to be used in simulations!
double
QdlFsoGammaGammaScintillationModel::GetIrradianceGivenAlphaBeta(double alpha, double beta)
{
	NS_LOG_FUNCTION(this);

	// Sample X ~ Gamma(alpha, 1/alpha) and Y ~ Gamma(beta, 1/beta)
	double X = m_gammaRvLargeA->GetValue(alpha, 1.0 / alpha);
	double Y = m_gammaRvSmallB->GetValue(beta, 1.0 / beta);

	double I = X * Y;

	NS_LOG_DEBUG("eta_scint = " << I);

	return I;
}

// This function is only for validation and testing. It is not meant to be used in simulations!
double
QdlFsoGammaGammaScintillationModel::GetTheoreticalPdf(double I, double alpha, double beta)
{
	NS_LOG_FUNCTION(this);

	if (I <= 1e-12)
	{
		return 0.0;
	}

	double v = alpha - beta;
	double arg = 2.0 * std::sqrt(alpha * beta * I);

	double kv_val = boost::math::cyl_bessel_k(v, arg);
	double gamma_a = boost::math::tgamma(alpha);
	double gamma_b = boost::math::tgamma(beta);

	double coeff = (2.0 * std::pow(alpha * beta, (alpha + beta) / 2.0)) / (gamma_a * gamma_b);
	double power_term = std::pow(I, ((alpha + beta) / 2.0) - 1.0);

	return coeff * power_term * kv_val;
}

} // namespace ns3
