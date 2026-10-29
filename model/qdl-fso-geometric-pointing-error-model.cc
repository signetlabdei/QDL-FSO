#include "qdl-fso-geometric-pointing-error-model.h"

#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/qdl-fso-cn2-model.h"
#include "ns3/qdl-fso-numerical-integration.h"
#include "ns3/qdl-fso-optical-terminal.h"
#include "ns3/qdl-fso-propagation-loss-model.h"
#include "ns3/string.h"

#include <cmath>
#include <iomanip> // float print settings

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("QdlFsoGeometricPointingErrorModel");

NS_OBJECT_ENSURE_REGISTERED(QdlFsoGeometricPointingErrorModel);

TypeId
QdlFsoGeometricPointingErrorModel::GetTypeId()
{
	static TypeId tid = TypeId("ns3::QdlFsoGeometricPointingErrorModel")
							.SetParent<Object>()
							.SetGroupName("QdlFso")
							.AddConstructor<QdlFsoGeometricPointingErrorModel>()
							.AddAttribute("Cn2Model",
										  "Refractive index structure parameter model.",
										  PointerValue(),
										  MakePointerAccessor(&QdlFsoGeometricPointingErrorModel::m_cn2Model),
										  MakePointerChecker<QdlFsoCn2Model>())
							.AddAttribute("NormalRvX",
										  "Normal random variable generator for X-axis offset.",
										  StringValue("ns3::NormalRandomVariable[Mean=0.0|Variance=1.0]"),
										  MakePointerAccessor(&QdlFsoGeometricPointingErrorModel::m_normalRvX),
										  MakePointerChecker<NormalRandomVariable>())
							.AddAttribute("NormalRvY",
										  "Normal random variable generator for Y-axis offset.",
										  StringValue("ns3::NormalRandomVariable[Mean=0.0|Variance=1.0]"),
										  MakePointerAccessor(&QdlFsoGeometricPointingErrorModel::m_normalRvY),
										  MakePointerChecker<NormalRandomVariable>());

	return tid;
}

QdlFsoGeometricPointingErrorModel::QdlFsoGeometricPointingErrorModel()
{
	NS_LOG_FUNCTION(this);
	m_normalRvX = CreateObject<NormalRandomVariable>();
	m_normalRvY = CreateObject<NormalRandomVariable>();
}

QdlFsoGeometricPointingErrorModel::~QdlFsoGeometricPointingErrorModel()
{
	NS_LOG_FUNCTION(this);
}

void
QdlFsoGeometricPointingErrorModel::DoDispose()
{
	NS_LOG_FUNCTION(this);
	m_cn2Model = nullptr;
	m_normalRvX = nullptr;
	m_normalRvY = nullptr;
	Object::DoDispose();
}

void
QdlFsoGeometricPointingErrorModel::SetCn2Model(Ptr<QdlFsoCn2Model> model)
{
	NS_LOG_FUNCTION(this);
	m_cn2Model = model;
}

Ptr<QdlFsoCn2Model>
QdlFsoGeometricPointingErrorModel::GetCn2Model() const
{
	NS_LOG_FUNCTION(this);
	return m_cn2Model;
}

int64_t
QdlFsoGeometricPointingErrorModel::AssignStreams(int64_t stream)
{
	NS_LOG_FUNCTION(this);
	m_normalRvX->SetStream(stream);
	m_normalRvY->SetStream(stream + 1);
	return 2;
}

double
QdlFsoGeometricPointingErrorModel::SampleRadialDisplacement(const QdlFsoPropagationContext& context,
															double wander_variance)
{
	NS_LOG_FUNCTION(this);

	NS_ASSERT_MSG(m_normalRvX != nullptr, "m_normalRvX r.v. not set!");
	NS_ASSERT_MSG(m_normalRvY != nullptr, "m_normalRvY r.v. not set!");

	double L = context.pathLength;

	// The commented out parts are code for future extension of the model
	double var_x = (std::pow(L * context.tx->GetPointingJitter(), 2.0) + 0.5 * wander_variance) *
				   (1.0 - context.rx->GetTrackingEfficiency());
	// double var_x = (std::pow(L * context.tx->GetPointingJitterX(), 2.0) + 0.5 * wander_variance) * (1.0 -
	// context.rx->GetTrackingEfficiency());

	double var_y = (std::pow(L * context.tx->GetPointingJitter(), 2.0) + 0.5 * wander_variance) *
				   (1.0 - context.rx->GetTrackingEfficiency());
	// double var_y = (std::pow(L * context.tx->GetPointingJitterY(), 2.0) + 0.5 * wander_variance) * (1.0 -
	// context.rx->GetTrackingEfficiency());

	double x = m_normalRvX->GetValue(0, var_x);
	// double x = m_normalRvX->GetValue(context.tx->GetPointingSystematicErrorX(), var_x);

	double y = m_normalRvY->GetValue(0, var_y);
	// double y = m_normalRvX->GetValue(context.tx->GetPointingSystematicErrorY(), var_y);

	// r = sqrt(x^2 + y^2)
	return std::hypot(x, y);
}

double
QdlFsoGeometricPointingErrorModel::GetPointingEfficiency(const QdlFsoPropagationContext& context,
														 double rytov_variance,
														 double wander_variance)
{
	NS_LOG_FUNCTION(this);

	NS_ASSERT_MSG(m_cn2Model != nullptr, "Cn2 model not set!");

	double r = SampleRadialDisplacement(context, wander_variance);

	NS_LOG_DEBUG("Pointing Error Displacement r = " << r << " m");

	double W0 = context.tx->GetTxBeamWaist();

	double k = 2 * M_PI / context.wavelength;

	NS_LOG_DEBUG("k: " << k);

	double W_LT = -1;

	double zeta = context.zenithAngle; // maybe it would be good to choose consistently what convention to use for
									   // downlink zenith angles, enforce it, and ensure that it works well everywhere
	double sec_zeta = 1 / std::cos(zeta);
	double h0 = std::min(context.txAltitude, context.rxAltitude);
	double H = std::max(context.txAltitude, context.rxAltitude);

	auto cn2Fn = m_cn2Model->GetCn2Function();

	double W = W0 * std::sqrt(1 + std::pow((context.wavelength * context.pathLength) / (M_PI * W0 * W0), 2.0));
	NS_LOG_DEBUG("W: " << W);

	switch (context.scenario)
	{
	case QdlFsoScenario::Horizontal:
		W_LT =
			W * std::sqrt(1 + 1.63 * std::pow(rytov_variance, 6.0 / 5.0) * ((2.0 * context.pathLength) / (k * W * W)));
		break;
	case QdlFsoScenario::Uplink: {
		double D0 = std::sqrt(8 * W0 * W0);
		auto integrand_cn2 = [&](double h) -> double { return cn2Fn(h); };
		double int_cn2 = qdlfso::GaussLegendreCompositeIntegrate(integrand_cn2, h0, H, 500, 250);
		double r0 = std::pow(0.42 * sec_zeta * k * k * int_cn2, -3.0 / 5.0);
		W_LT = W * std::pow(1 + std::pow(D0 / r0, 5.0 / 3.0), 3.0 / 5.0);
		break;
	}
	case QdlFsoScenario::Downlink: {
		double Lambda = 2 * context.pathLength / k * W * W;
		auto integrand_mu2d = [&](double h) -> double { return cn2Fn(h) * std::pow((h - h0) / (H - h0), 5.0 / 3.0); };
		double mu2d = qdlfso::GaussLegendreCompositeIntegrate(integrand_mu2d, h0, H, 500, 250);
		W_LT = W * std::sqrt(1 + 4.35 * mu2d * std::pow(Lambda, 5.0 / 6.0) * std::pow(k, 7.0 / 6.0) *
									 std::pow(H - h0, 5.0 / 6.0) * std::pow(sec_zeta, 11.0 / 6.0));
		break;
	}
	default:;
		NS_ABORT_MSG("bad scenario" + std::to_string(static_cast<int>(context.scenario)));
	}

	NS_LOG_DEBUG("wavelength: " << context.wavelength);
	NS_LOG_DEBUG("pathLength: " << context.pathLength);
	NS_LOG_DEBUG("rytov: " << rytov_variance);

	NS_LOG_DEBUG("W_LT: " << W_LT);
	NS_LOG_DEBUG("wander: " << wander_variance);

	double diff = W_LT * W_LT - wander_variance;
	NS_LOG_DEBUG("diff: " << diff);
	NS_ASSERT_MSG(diff > 0,
				  "Wander variance (" << wander_variance << ") cannot be >= than W_LT^2 (" << W_LT * W_LT << ")");

	double W_ST = std::sqrt(diff);
	NS_LOG_DEBUG("W_ST: " << W_ST);

	// Short-term beam can't be narrower than the diffraction-limited beam.
	if (W_ST < W)
	{
		NS_LOG_WARN("W_ST must be greater than the purely geometric broadening version. Considering clamping W_ST to "
					"W if there is a small rounding error."
					<< std::fixed << std::setprecision(5) << " W_ST = " << W_ST << "; W = " << W);

		const double deficit = W - W_ST;
		const double tol = 1e-2;

		NS_ASSERT_MSG(deficit <= tol,
					  "WST is effectively smaller than the purely geometric broadening version ("
						  << deficit << " > " << (tol)
						  << "). This should not be "
							 "possible. Abort simulation.");

		NS_LOG_WARN("Since the difference between W^2 and W_ST^2 is very low (" << deficit
																				<< "), we manually set W_ST = W.");
		W_ST = W;
	}

	double D_rx = context.rx->GetRxApertureDiameter();
	NS_LOG_DEBUG("D_rx: " << D_rx);

	if (r <= 1e-10)
	{
		NS_LOG_DEBUG("Since r == 0, there is perfect pointing. Use the formula without approximations.");

		return 1 - std::exp(-(D_rx * D_rx) / (2 * W_ST * W_ST));
	}

	NS_LOG_DEBUG("2.0 * W_ST / D_rx > 1.0 ? -> " << (2.0 * W_ST) << " / " << D_rx << " > 1.0 ? -> "
												 << (2.0 * W_ST / D_rx > 1.0));

	if (2.0 * W_ST / D_rx > 1.0)
	{
		NS_LOG_DEBUG("Use Farid approximation.");
		return GetPointingEfficiencyFarid(D_rx, r, W_ST);
	}
	else
	{
		NS_LOG_DEBUG("Use Miao approximation.");
		return GetPointingEfficiencyMiao(D_rx, r, W_ST, "smooth");
	}
}

double
QdlFsoGeometricPointingErrorModel::GetPointingEfficiencyFarid(double D_rx, double r, double W_ST)
{
	NS_LOG_FUNCTION(this);

	double v = (std::sqrt(M_PI) * D_rx / 2.0) / (std::sqrt(2.0) * W_ST);

	double A0 = std::pow(std::erf(v), 2.0);

	double W_eq_2 = W_ST * W_ST * (std::sqrt(M_PI) * std::erf(v)) / (2.0 * v * std::exp(-v * v));

	double eta = A0 * std::exp(-(2.0 * r * r) / W_eq_2);

	NS_LOG_INFO("Farid Pointing Efficiency = " << eta);

	return eta;
}

double
QdlFsoGeometricPointingErrorModel::GetPointingEfficiencyMiao(double D_rx, double r, double W_ST, std::string mode)
{
	NS_LOG_FUNCTION(this);

	if (mode == "smooth")
	{
		double alpha = (std::sqrt(2.0) * D_rx) / (std::sqrt(M_PI) * W_ST);
		double eta = 1 - 1 / (1 + std::exp(-alpha * (std::pow(2.0 * r / D_rx, 2.0) - 1)));
		NS_LOG_INFO("Miao smooth Pointing Efficiency = " << eta);
		return eta;
	}
	else if (mode == "step")
	{
		double eta;
		if (r >= 0.0 && r < D_rx / 2.0)
		{
			eta = 1.0;
		}
		else if (r == D_rx / 2.0)
		{
			eta = 0.5;
		}
		else
		{
			eta = 0.0;
		}
		NS_LOG_INFO("Miao step Pointing Efficiency = " << eta);
		return eta;
	}
	else
	{
		NS_ABORT_MSG("Miao model mode not valid. Must choose between 'step' and 'smooth'. You selected " << mode);
	}
}

} // namespace ns3
