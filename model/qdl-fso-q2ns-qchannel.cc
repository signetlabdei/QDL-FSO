#include "qdl-fso-q2ns-qchannel.h"

#include "qdl-fso-optical-terminal.h"
#include "qdl-fso-rytov-model.h"

#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/node.h"
#include "ns3/pointer.h"
#include "ns3/q2ns-qnet-device.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("QdlFsoQChannel");
NS_OBJECT_ENSURE_REGISTERED(QdlFsoQChannel);

ns3::TypeId
QdlFsoQChannel::GetTypeId(void)
{
	static ns3::TypeId tid = ns3::TypeId("qdl::QdlFsoQChannel")
								 .SetParent<q2ns::QChannel>()
								 .SetGroupName("qdl-fso")
								 .AddConstructor<QdlFsoQChannel>()
								 .AddAttribute("PropagationLossModel",
											   "Propagation loss model used to compute the qubit survival probability.",
											   PointerValue(),
											   MakePointerAccessor(&QdlFsoQChannel::SetPropagationLossModel,
																   &QdlFsoQChannel::GetPropagationLossModel),
											   MakePointerChecker<QdlFsoPropagationLossModel>())
								 .AddAttribute("PropagationDelayModel",
											   "Propagation delay model used to compute the qubit propagation delay.",
											   //    PointerValue(CreateObject<ConstantSpeedPropagationDelayModel>()),
											   PointerValue(),
											   MakePointerAccessor(&QdlFsoQChannel::SetPropagationDelayModel,
																   &QdlFsoQChannel::GetPropagationDelayModel),
											   MakePointerChecker<PropagationDelayModel>())
								 .AddAttribute("EnableDephasing",
											   "Enable dephasing effects on the flying qubits",
											   BooleanValue(false),
											   MakeBooleanAccessor(&QdlFsoQChannel::m_enableDephasing),
											   MakeBooleanChecker())
								 .AddTraceSource("QubitTxedTrace",
												 "A qubit is handed to the channel for transmission.",
												 MakeTraceSourceAccessor(&QdlFsoQChannel::m_qubitTxedTrace),
												 "ns3::QdlFsoQChannel::QubitTxedTracedCallback");
	return tid;
}

QdlFsoQChannel::QdlFsoQChannel()
{
	NS_LOG_FUNCTION(this);

	m_lossMapAB = CreateObject<q2ns::LossQMap>();
	m_lossMapBA = CreateObject<q2ns::LossQMap>();

	m_dephMapAB = CreateObject<q2ns::DephasingQMap>();
	m_dephMapBA = CreateObject<q2ns::DephasingQMap>();

	// By default we leave these maps not active
	m_dephMapAB->SetAttribute("Probability", DoubleValue(0.0));
	m_dephMapBA->SetAttribute("Probability", DoubleValue(0.0));

	auto c_mapAB = q2ns::QMap::Compose({m_lossMapAB, m_dephMapAB});
	auto c_mapBA = q2ns::QMap::Compose({m_lossMapBA, m_dephMapBA});

	this->SetQMapAB(c_mapAB);
	this->SetQMapBA(c_mapBA);
}

void
QdlFsoQChannel::SetPropagationLossModel(ns3::Ptr<ns3::QdlFsoPropagationLossModel> propagation_loss)
{
	NS_LOG_FUNCTION(this);
	m_propagation_loss = propagation_loss;
}

ns3::Ptr<ns3::QdlFsoPropagationLossModel>
QdlFsoQChannel::GetPropagationLossModel() const
{
	NS_LOG_FUNCTION(this);
	return m_propagation_loss;
}

void
QdlFsoQChannel::SetPropagationDelayModel(Ptr<PropagationDelayModel> delayModel)
{
	NS_LOG_FUNCTION(this);
	m_propagationDelay = delayModel;
}

Ptr<PropagationDelayModel>
QdlFsoQChannel::GetPropagationDelayModel() const
{
	NS_LOG_FUNCTION(this);
	return m_propagationDelay;
}

bool
QdlFsoQChannel::SendFrom(q2ns::QNetDevice* src, std::shared_ptr<q2ns::Qubit> q)
{
	NS_LOG_FUNCTION(this);

	ns3::Ptr<ns3::NetDevice> aDev = this->GetDevice(0);
	ns3::Ptr<ns3::NetDevice> bDev = this->GetDevice(1);

	ns3::Ptr<ns3::NetDevice> dst;
	bool isAB = false;

	// 1. Determine direction
	if (aDev && src == ns3::PeekPointer(aDev))
	{
		dst = bDev;
		isAB = true;
	}
	else if (bDev && src == ns3::PeekPointer(bDev))
	{
		dst = aDev;
		isAB = false;
	}
	else
	{
		NS_LOG_WARN("SendFrom rejected: source device is not attached to this channel.");
		return false;
	}

	ns3::Ptr<ns3::MobilityModel> mob_src = src->GetNode()->GetObject<ns3::MobilityModel>();
	ns3::Ptr<ns3::MobilityModel> mob_dst = dst->GetNode()->GetObject<ns3::MobilityModel>();

	double survivalProb = m_propagation_loss->GetSurvivalProb(mob_src, mob_dst);
	double lossProb = 1 - survivalProb;

	NS_LOG_DEBUG("[QdlFsoQ2nsQChannel] Loss Prob " << lossProb);

	NS_ASSERT_MSG(m_propagationDelay, "Set a PropagationDelayModel on QdlFsoQChannel first.");
	ns3::Time propDelay = m_propagationDelay->GetDelay(mob_src, mob_dst);

	auto rytov_var = m_propagation_loss->GetRytovModel()->GetRytovVariance(
		m_propagation_loss->ComputeCurrentContext(mob_src, mob_dst));

	// Use 0.5*erf, since erf goes to 1, but if p_deph is > 0.5 then the effects of the dephasing can be detected and
	// corrected automatically.
	double dephProb = 0.5 * std::erf(rytov_var);

	if (isAB)
	{
		this->SetDelayAB(propDelay);
		m_lossMapAB->SetAttribute("Probability", DoubleValue(lossProb));

		if (m_enableDephasing)
		{
			m_dephMapAB->SetAttribute("Probability", DoubleValue(dephProb));
		}
	}
	else
	{
		this->SetDelayBA(propDelay);
		m_lossMapBA->SetAttribute("Probability", DoubleValue(lossProb));

		if (m_enableDephasing)
		{
			m_dephMapBA->SetAttribute("RaProbabilityte", DoubleValue(dephProb));
		}
	}

	m_qubitTxedTrace(q, survivalProb, dephProb, src->GetNode(), dst->GetNode());

	return q2ns::QChannel::SendFrom(src, q);
}

} // namespace ns3
