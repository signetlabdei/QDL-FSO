#include "qdl-fso-beam-wander-model.h"

#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/qdl-fso-cn2-model.h"
#include "ns3/qdl-fso-optical-terminal.h"
#include "ns3/qdl-fso-propagation-loss-model.h"

#include <iomanip>
#include <string>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("QdlFsoBeamWanderModel");

NS_OBJECT_ENSURE_REGISTERED(QdlFsoBeamWanderModel);

TypeId
QdlFsoBeamWanderModel::GetTypeId()
{
	static TypeId tid = TypeId("ns3::QdlFsoBeamWanderModel")
							.SetParent<Object>()
							.SetGroupName("QdlFso")
							.AddConstructor<QdlFsoBeamWanderModel>()
							.AddAttribute("Cn2Model",
										  "Refractive index structure parameter model.",
										  PointerValue(),
										  MakePointerAccessor(&QdlFsoBeamWanderModel::m_cn2Model),
										  MakePointerChecker<QdlFsoCn2Model>());

	return tid;
}

QdlFsoBeamWanderModel::QdlFsoBeamWanderModel()
{
	NS_LOG_FUNCTION(this);
}

QdlFsoBeamWanderModel::~QdlFsoBeamWanderModel()
{
	NS_LOG_FUNCTION(this);
}

void
QdlFsoBeamWanderModel::DoDispose()
{
	NS_LOG_FUNCTION(this);
	m_cn2Model = nullptr;
	Object::DoDispose();
}

void
QdlFsoBeamWanderModel::SetCn2Model(Ptr<QdlFsoCn2Model> model)
{
	NS_LOG_FUNCTION(this);
	m_cn2Model = model;
}

Ptr<QdlFsoCn2Model>
QdlFsoBeamWanderModel::GetCn2Model() const
{
	NS_LOG_FUNCTION(this);
	return m_cn2Model;
}

double
QdlFsoBeamWanderModel::GetWanderVariance(const QdlFsoPropagationContext& context, double rytov_variance)
{
	NS_LOG_FUNCTION(this);

	NS_ASSERT_MSG(m_cn2Model != nullptr, "Cn2 model not set!");

	switch (context.scenario)
	{
	case QdlFsoScenario::Horizontal:
		return GetWanderVarianceHorizontal(context, rytov_variance);
	case QdlFsoScenario::Downlink:
	case QdlFsoScenario::Uplink:
		return GetWanderVarianceUplinkDownlink(context, rytov_variance);
	default:
		NS_ABORT_MSG("context.scenario " << context.scenario << " is not supported!");
	}
	return {}; // unreachable
}

double
QdlFsoBeamWanderModel::GetWanderVarianceHorizontal(const QdlFsoPropagationContext& context, double rytov_variance)
{
	NS_LOG_FUNCTION(this);

	NS_ASSERT_MSG(m_cn2Model != nullptr, "Cn2 model not set!");

	NS_LOG_INFO("Wander Scenario: " << context.scenario);

	// Check if the link is horizontal
	bool altitudesEqual = std::abs(context.txAltitude - context.rxAltitude) < QdlFsoPropagationLossModel::ALT_TOLERANCE;
	NS_ASSERT_MSG(altitudesEqual,
				  "If context.scenario is Horizontal, tx and rx altitude must not differ more "
				  "than "
					  << QdlFsoPropagationLossModel::ALT_TOLERANCE << " meters! tx_alt = " << context.txAltitude
					  << " != rx_alt = " << context.rxAltitude);

	const double h0 = std::min(context.txAltitude, context.rxAltitude);
	const double H = std::max(context.txAltitude, context.rxAltitude);

	// Get the altitude of the horizontal link
	const double h = (H + h0) / 2.0;

	auto cn2Fn = m_cn2Model->GetCn2Function();

	double W0 = context.tx->GetTxBeamWaist();

	const double prefactor = 7.25 * cn2Fn(h) * std::pow(context.pathLength, 3.0) * std::pow(W0, -1.0 / 3.0);

	// Theta0, ThetaBar0 and Lambda0 computations
	const double k = 2.0 * M_PI / context.wavelength; // optical wavenumber [rad/m]
	const double rayleigh_length = 0.5 * k * std::pow(W0, 2.0);
	double Theta0 = 1.0 - context.pathLength / rayleigh_length;
	double ThetaBar0 = 1.0 - Theta0;
	const double Lambda0 = (2 * context.pathLength) / (k * std::pow(W0, 2.0));

	NS_LOG_DEBUG("k: " << k);
	NS_LOG_DEBUG("rayleigh_length: " << rayleigh_length);
	NS_LOG_DEBUG("Theta0: " << Theta0);
	NS_LOG_DEBUG("ThetaBar0: " << ThetaBar0);
	NS_LOG_DEBUG("Lambda0: " << Lambda0);

	// Formula presented in: L. C. Andrews and R. L. Phillips, "Laser Beam Propagation through Random Media"
	auto integrand = [&](double xi) -> double {
		return std::pow(xi, 2.0) /
			   std::pow((std::pow(Theta0 + ThetaBar0 * xi, 2.0) +
						 1.63 * std::pow(rytov_variance, 6.0 / 5.0) * Lambda0 * std::pow(1 - xi, 16.0 / 5.0)),
						1.0 / 6.0);
	};

	return prefactor * qdlfso::GaussLegendreCompositeIntegrate(integrand, 0.0, 1.0, 100, 50);
}

double
QdlFsoBeamWanderModel::GetWanderVarianceUplinkDownlink(const QdlFsoPropagationContext& context, double rytov_variance)
{
	NS_LOG_FUNCTION(this);

	NS_ASSERT_MSG(m_cn2Model != nullptr, "Cn2 model not set!");

	NS_LOG_INFO("Wander Scenario: " << context.scenario);

	// Check if the link is not horizontal
	bool altitudesEqual = std::abs(context.txAltitude - context.rxAltitude) < QdlFsoPropagationLossModel::ALT_TOLERANCE;
	NS_ASSERT_MSG(!altitudesEqual,
				  "If context.scenario is Downlink or Uplink, tx and rx altitude must differ "
				  "more than "
					  << QdlFsoPropagationLossModel::ALT_TOLERANCE << " meters! tx_alt = " << context.txAltitude
					  << " == rx_alt = " << context.rxAltitude);

	const double h0 = std::min(context.txAltitude, context.rxAltitude);
	const double H = std::max(context.txAltitude, context.rxAltitude);

	double W0 = context.tx->GetTxBeamWaist();

	const double prefactor = 7.25 * std::pow(W0, -1.0 / 3.0);

	auto cn2Fn = m_cn2Model->GetCn2Function();

	// Theta0, ThetaBar0 and Lambda0 computations
	const double k = 2.0 * M_PI / context.wavelength; // optical wavenumber [rad/m]
	const double rayleigh_length = 0.5 * k * std::pow(W0, 2.0);
	double Theta0 = 1.0 - context.pathLength / rayleigh_length;
	double ThetaBar0 = 1.0 - Theta0;
	const double Lambda0 = (2 * context.pathLength) / (k * std::pow(W0, 2.0));

	NS_LOG_DEBUG("k: " << k);
	NS_LOG_DEBUG("rayleigh_length: " << rayleigh_length);
	NS_LOG_DEBUG("Theta0: " << Theta0);
	NS_LOG_DEBUG("ThetaBar0: " << ThetaBar0);
	NS_LOG_DEBUG("Lambda0: " << Lambda0);

	// Generalized formula, without normalizing the distance
	// see [Morellato2027] B. Morellato, R. Di Bella et al., "An FSO Channel Model for ns-3"
	auto integrand = [&](double s) -> double {
		double xi = s / context.pathLength;

		// Transformation to go from downlink to uplink
		if (context.scenario == QdlFsoScenario::Uplink)
		{
			return (cn2Fn(H - s * std::cos(context.zenithAngle)) * std::pow(s, 2.0)) /
				   std::pow((std::pow(Theta0 + ThetaBar0 * xi, 2.0) +
							 1.63 * std::pow(rytov_variance, 6.0 / 5.0) * Lambda0 * std::pow((1.0 - xi), 16.0 / 5.0)),
							1.0 / 6.0);
		}

		return (cn2Fn(h0 + s * std::cos(context.zenithAngle)) * std::pow(s, 2.0)) /
			   std::pow((std::pow(Theta0 + ThetaBar0 * xi, 2.0) +
						 1.63 * std::pow(rytov_variance, 6.0 / 5.0) * Lambda0 * std::pow((1.0 - xi), 16.0 / 5.0)),
						1.0 / 6.0);
	};

	return prefactor * qdlfso::GaussLegendreCompositeIntegrate(integrand, 0, context.pathLength, 1000, 500);
}

} // namespace ns3
