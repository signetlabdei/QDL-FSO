#include "qdl-fso-cn2-model.h"

#include "ns3/double.h"
#include "ns3/log.h"
// #include "ns3/qdl-fso-channel-core.h"
#include "ns3/qdl-fso-propagation-loss-model.h"

#include <cmath>
#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("QdlFsoCn2Model");

NS_OBJECT_ENSURE_REGISTERED(QdlFsoCn2Model);

TypeId
QdlFsoCn2Model::GetTypeId()
{
	static TypeId tid =
		TypeId("ns3::QdlFsoCn2Model")
			.SetParent<Object>()
			.SetGroupName("QdlFso")
			.AddConstructor<QdlFsoCn2Model>()
			.AddAttribute("VWind",
						  "Wind speed used in the Hufnagel-Valley Cn2 profile (m/s)", // TODO wind speed at what point
																					  // in the atmosphere? Clarify
						  DoubleValue(21.0),
						  MakeDoubleAccessor(&QdlFsoCn2Model::m_v_wind),
						  MakeDoubleChecker<double>())
			.AddAttribute("Cn20",
						  "Ground-level refractive index structure parameter Cn2(0)",
						  DoubleValue(1.7e-14),
						  MakeDoubleAccessor(&QdlFsoCn2Model::m_Cn2_0),
						  MakeDoubleChecker<double>());

	return tid;
}

QdlFsoCn2Model::QdlFsoCn2Model()
{
	NS_LOG_FUNCTION(this);

	// Default values are for the HV5/7 model
	m_v_wind = 21.0;
	m_Cn2_0 = 1.7e-14;
}

QdlFsoCn2Model::~QdlFsoCn2Model()
{
	NS_LOG_FUNCTION(this);
}

void
QdlFsoCn2Model::DoDispose()
{
	NS_LOG_FUNCTION(this);
	Object::DoDispose();
}

void
QdlFsoCn2Model::SetVWind(double v_wind)
{
	NS_LOG_FUNCTION(this);
	m_v_wind = v_wind;
}

void
QdlFsoCn2Model::SetCn20(double Cn2_0)
{
	NS_LOG_FUNCTION(this);
	m_Cn2_0 = Cn2_0;
}

double
QdlFsoCn2Model::GetVWind()
{
	NS_LOG_FUNCTION(this);
	return m_v_wind;
}

double
QdlFsoCn2Model::GetCn20()
{
	NS_LOG_FUNCTION(this);
	return m_Cn2_0;
}

double
QdlFsoCn2Model::HufnagelValley(double h, double v_wind, double Cn2_0)
{
	return 0.00594 * std::pow((v_wind / 27.0), 2) * std::pow(1e-5 * h, 10) * std::exp(-(h / 1000.0)) +
		   2.7 * 1e-16 * std::exp(-(h / 1500.0)) + Cn2_0 * std::exp(-(h / 100.0));
}

std::function<double(double)>
QdlFsoCn2Model::GetCn2Function()
{
	NS_LOG_FUNCTION(this);
	double v_wind = m_v_wind;
	double Cn2_0 = m_Cn2_0;
	return [v_wind, Cn2_0](double h) { return HufnagelValley(h, v_wind, Cn2_0); };
}

// ---- JUST FOR DEBUGGING ----
std::vector<std::pair<double, double>>
QdlFsoCn2Model::GetCn2Profile(const QdlFsoPropagationContext& context)
{
	NS_LOG_FUNCTION(this);
	switch (context.scenario)
	{
	case QdlFsoScenario::Horizontal:
		return GetCn2ProfileHorizontal(context);
	case QdlFsoScenario::Downlink:
	case QdlFsoScenario::Uplink:
		return GetCn2ProfileUplinkDownlink(context);
	default:
		NS_ABORT_MSG("context.scenario " << context.scenario << " is not supported!");
	}
	return {}; // unreachable
}

std::vector<std::pair<double, double>>
QdlFsoCn2Model::GetCn2ProfileHorizontal(const QdlFsoPropagationContext& context)
{
	NS_LOG_FUNCTION(this);
	bool altitudesEqual = std::abs(context.txAltitude - context.rxAltitude) < QdlFsoPropagationLossModel::ALT_TOLERANCE;

	NS_ASSERT_MSG(altitudesEqual,
				  "If context.scenario is Horizontal, tx and rx altitude must not differ more "
				  "than "
					  << QdlFsoPropagationLossModel::ALT_TOLERANCE << " meters! tx_alt = " << context.txAltitude
					  << " != rx_alt = " << context.rxAltitude);

	std::vector<std::pair<double, double>> Cn2_profile;

	double high = std::max(context.rxAltitude, context.txAltitude);
	double low = std::min(context.rxAltitude, context.txAltitude);

	double h = (high + low) / 2;
	// Hufnagel-Valley (or similar) turbulence profile calculation
	double C_h = HufnagelValley(h, m_v_wind, m_Cn2_0);
	Cn2_profile.emplace_back(std::make_pair(h, C_h));

	return Cn2_profile;
}

std::vector<std::pair<double, double>>
QdlFsoCn2Model::GetCn2ProfileUplinkDownlink(const QdlFsoPropagationContext& context)
{
	NS_LOG_FUNCTION(this);

	bool altitudesEqual = std::abs(context.txAltitude - context.rxAltitude) < QdlFsoPropagationLossModel::ALT_TOLERANCE;

	NS_ASSERT_MSG(!altitudesEqual,
				  "If context.scenario is Downlink or Uplink, tx and rx altitude must differ "
				  "more than "
					  << QdlFsoPropagationLossModel::ALT_TOLERANCE << " meters! tx_alt = " << context.txAltitude
					  << " == rx_alt = " << context.rxAltitude);

	std::vector<std::pair<double, double>> Cn2_profile;

	double high = std::max(context.rxAltitude, context.txAltitude);
	double low = std::min(context.rxAltitude, context.txAltitude);

	double granularity = 10000.0;
	double step = (high - low) / granularity;

	// Calculate exact number of steps safely
	// Adding 1 to include both the start (low) and end (high) points
	size_t num_steps = static_cast<size_t>(std::floor((high - low) / step)) + 1;

	// Reserve memory to prevent reallocation during the loop (improves performance)
	Cn2_profile.reserve(num_steps);

	double h = low;
	for (size_t i = 0; i < num_steps; ++i)
	{
		// Ensure the last step doesn't slightly exceed 'high' due to rounding
		if (h > high)
		{
			h = high;
		}

		// Hufnagel-Valley (or similar) turbulence profile calculation
		double C_h = HufnagelValley(h, m_v_wind, m_Cn2_0);

		Cn2_profile.emplace_back(std::make_pair(h, C_h));

		h += step;
	}

	// The vector size will perfectly match num_steps now.
	NS_ASSERT(Cn2_profile.size() == num_steps);

	return Cn2_profile;
}

// ----------------------------

} // namespace ns3
