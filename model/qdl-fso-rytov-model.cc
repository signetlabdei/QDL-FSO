#include "qdl-fso-rytov-model.h"

#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/qdl-fso-cn2-model.h"
#include "ns3/qdl-fso-propagation-loss-model.h"

#include <cmath>
#include <iomanip>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("QdlFsoRytovModel");

NS_OBJECT_ENSURE_REGISTERED(QdlFsoRytovModel);

TypeId
QdlFsoRytovModel::GetTypeId()
{
	static TypeId tid = TypeId("ns3::QdlFsoRytovModel")
							.SetParent<Object>()
							.SetGroupName("QdlFso")
							.AddConstructor<QdlFsoRytovModel>()
							.AddAttribute("Cn2Model",
										  "Refractive index structure parameter model.",
										  PointerValue(),
										  MakePointerAccessor(&QdlFsoRytovModel::m_cn2Model),
										  MakePointerChecker<QdlFsoCn2Model>());

	return tid;
}

QdlFsoRytovModel::QdlFsoRytovModel()
{
	NS_LOG_FUNCTION(this);
}

QdlFsoRytovModel::~QdlFsoRytovModel()
{
	NS_LOG_FUNCTION(this);
}

void
QdlFsoRytovModel::DoDispose()
{
	NS_LOG_FUNCTION(this);
	m_cn2Model = nullptr;
	Object::DoDispose();
}

void
QdlFsoRytovModel::SetCn2Model(Ptr<QdlFsoCn2Model> model)
{
	NS_LOG_FUNCTION(this);
	m_cn2Model = model;
}

Ptr<QdlFsoCn2Model>
QdlFsoRytovModel::GetCn2Model() const
{
	NS_LOG_FUNCTION(this);
	return m_cn2Model;
}

double
QdlFsoRytovModel::GetRytovVariance(const QdlFsoPropagationContext& context)
{
	NS_LOG_FUNCTION(this);

	NS_ASSERT_MSG(m_cn2Model != nullptr, "Cn2 model not set!");

	switch (context.scenario)
	{
	case QdlFsoScenario::Horizontal:
		return GetRytovVarianceHorizontal(context);
	case QdlFsoScenario::Downlink:
	case QdlFsoScenario::Uplink:
		return GetRytovVarianceUplinkDownlink(context);
	default:
		NS_ABORT_MSG("context.scenario " << context.scenario << " is not supported!");
	}
	return {}; // unreachable
}

double
QdlFsoRytovModel::GetRytovVarianceHorizontal(const QdlFsoPropagationContext& context)
{
	NS_LOG_FUNCTION(this);

	NS_ASSERT_MSG(m_cn2Model != nullptr, "Cn2 model not set!");

	NS_LOG_INFO("Rytov Scenario: " << context.scenario);

	// Chek tha link is horizontal
	bool altitudesEqual = std::abs(context.txAltitude - context.rxAltitude) < QdlFsoPropagationLossModel::ALT_TOLERANCE;
	NS_ASSERT_MSG(altitudesEqual,
				  "If context.scenario is Horizontal, tx and rx altitude must not differ more "
				  "than "
					  << QdlFsoPropagationLossModel::ALT_TOLERANCE << " meters! tx_alt = " << context.txAltitude
					  << " != rx_alt = " << context.rxAltitude);

	const double h0 = std::min(context.txAltitude, context.rxAltitude);
	const double H = std::max(context.txAltitude, context.rxAltitude);

	const double h = (H + h0) / 2;

	const double k = 2.0 * M_PI / context.wavelength; // optical wavenumber [rad/m]

	auto cn2Fn = m_cn2Model->GetCn2Function();

	// Use plane wave approx for horizontal links
	double rytov_var = 1.23 * cn2Fn(h) * std::pow(k, 7.0 / 6.0) * std::pow(context.pathLength, 11.0 / 6.0);
	NS_LOG_DEBUG("Rytov variance = " << std::setprecision(15) << rytov_var);

	return rytov_var;
}

double
QdlFsoRytovModel::GetRytovVarianceUplinkDownlink(const QdlFsoPropagationContext& context)
{
	NS_LOG_FUNCTION(this);

	NS_ASSERT_MSG(m_cn2Model != nullptr, "Cn2 model not set!");

	NS_LOG_INFO("Rytov Scenario: " << context.scenario);

	// Check that the link is not horizontal
	bool altitudesEqual = std::abs(context.txAltitude - context.rxAltitude) < QdlFsoPropagationLossModel::ALT_TOLERANCE;
	NS_ASSERT_MSG(!altitudesEqual,
				  "If context.scenario is Downlink or Uplink, tx and rx altitude must differ "
				  "more than "
					  << QdlFsoPropagationLossModel::ALT_TOLERANCE << " meters! tx_alt = " << context.txAltitude
					  << " == rx_alt = " << context.rxAltitude);

	const double h0 = std::min(context.txAltitude, context.rxAltitude);
	const double H = std::max(context.txAltitude, context.rxAltitude);
	const double deltaAltitude = H - h0;

	const double k = 2.0 * M_PI / context.wavelength; // optical wavenumber [rad/m]
	const double secZeta = 1.0 / std::cos(context.zenithAngle);
	const double prefactor = 2.25 * std::pow(k, 7.0 / 6.0) * std::pow(secZeta, 11.0 / 6.0);

	auto cn2Fn = m_cn2Model->GetCn2Function();

	auto weight = [&](double h) -> double {
		if (context.scenario == QdlFsoScenario::Uplink)
		{
			return std::pow((h - h0) * (H - h) / deltaAltitude, 5.0 / 6.0);
		}
		return std::pow((h - h0), 5.0 / 6.0); // plane wave (default)
	};

	auto integrand = [&](double h) -> double { return cn2Fn(h) * weight(h); };

	return prefactor * qdlfso::GaussLegendreCompositeIntegrate(integrand, h0, H, 500, 250);
}

} // namespace ns3
