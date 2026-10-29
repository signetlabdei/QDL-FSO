#include "qdl-fso-helper.h"

#include "ns3/buildings-module.h"
#include "ns3/mobility-module.h"
#include "ns3/names.h"
#include "ns3/q2ns-qnet-device.h"
#include "ns3/qdl-fso-atmospheric-model.h"
#include "ns3/qdl-fso-beam-wander-model.h"
#include "ns3/qdl-fso-cn2-model.h"
#include "ns3/qdl-fso-geometric-pointing-error-model.h"
#include "ns3/qdl-fso-optical-terminal.h"
#include "ns3/qdl-fso-rytov-model.h"
#include "ns3/qdl-fso-scintillation-model.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("QdlFsoHelper");
NS_OBJECT_ENSURE_REGISTERED(QdlFsoHelper);

TypeId
QdlFsoHelper::GetTypeId()
{
	static TypeId tid =
		TypeId("ns3::QdlFsoHelper").SetParent<Object>().SetGroupName("QdlFso").AddConstructor<QdlFsoHelper>();

	return tid;
}

QdlFsoHelper::QdlFsoHelper()
{
	NS_LOG_FUNCTION(this);
	// Schedule a one-time initializer at time 0 so RNG stream binding uses the
	// final ns-3 seed/run settings regardless of whether the user calls a
	// controller helper or ns3::Simulator::Run() directly.
	ns3::Simulator::ScheduleNow(&QdlFsoHelper::EnsureStreamsAssigned_, this);
}

QdlFsoHelper::~QdlFsoHelper()
{
	NS_LOG_FUNCTION(this);
}

void
QdlFsoHelper::DoDispose()
{
	NS_LOG_FUNCTION(this);
	Object::DoDispose();
}

void
QdlFsoHelper::EnsureStreamsAssigned_()
{
	if (streamsAssigned_)
	{
		return;
	}

	AssignStreams(0);
}

int64_t
QdlFsoHelper::AssignStreams(int64_t stream)
{
	if (streamsAssigned_)
	{
		NS_LOG_WARN("AssignStreams called more than once; streams will be reassigned.");
	}

	streamsAssigned_ = true;
	nextStream_ = stream;

	int64_t used = 0;

	// Channels are stored in std::map, so iteration order is deterministic.
	for (auto& kv : channels_)
	{
		auto& ch = kv.second;
		const int64_t n = ch->AssignStreams(nextStream_);
		nextStream_ += n;
		used += n;
	}

	return used;
}

ns3::Ptr<QdlFsoPropagationLossModel>
QdlFsoHelper::CreateDefaultPropagationLossModel()
{
	NS_LOG_FUNCTION(this);

	Ptr<QdlFsoPropagationLossModel> propagation_loss = CreateObject<QdlFsoPropagationLossModel>();

	// Default channel condition model is BuildingsChannelConditionModel
	Ptr<BuildingsChannelConditionModel> condModel = CreateObject<BuildingsChannelConditionModel>();

	propagation_loss->SetChannelConditionModel(condModel);

	// Instantiate and set all sub-models

	// By default we use Lowtran for the atmospheric model
	ns3::Ptr<QdlFsoLowtranAtmosphericModel> atm_model = CreateObject<QdlFsoLowtranAtmosphericModel>();
	propagation_loss->SetAtmosphericModel(atm_model);

	ns3::Ptr<QdlFsoCn2Model> cn2_model = CreateObject<QdlFsoCn2Model>();
	propagation_loss->SetCn2Model(cn2_model);

	ns3::Ptr<QdlFsoRytovModel> rytov_model = CreateObject<QdlFsoRytovModel>();
	rytov_model->SetCn2Model(cn2_model);
	propagation_loss->SetRytovModel(rytov_model);

	ns3::Ptr<QdlFsoBeamWanderModel> wander_model = CreateObject<QdlFsoBeamWanderModel>();
	wander_model->SetCn2Model(cn2_model);
	propagation_loss->SetBeamWanderModel(wander_model);

	ns3::Ptr<QdlFsoGeometricPointingErrorModel> geom_point_model = CreateObject<QdlFsoGeometricPointingErrorModel>();
	geom_point_model->SetCn2Model(cn2_model);
	propagation_loss->SetGeometricPointingErrorModel(geom_point_model);

	ns3::Ptr<QdlFsoGammaGammaScintillationModel> scintillation_model =
		CreateObject<QdlFsoGammaGammaScintillationModel>();
	// scintillation_model->SetAttribute("EnableApertureAveraging", BooleanValue(false));
	propagation_loss->SetScintillationModel(scintillation_model);

	// Assign stream index for reproducible independent Monte Carlo runs
	// JUST FOR DEBUGGING
	propagation_loss->AssignStreams(100);

	return propagation_loss;
}

ns3::Ptr<QdlFsoQChannel>
QdlFsoHelper::GetChannel(ns3::Ptr<q2ns::QNode> a, ns3::Ptr<q2ns::QNode> b)
{
	NS_LOG_FUNCTION(this);

	if (!a || !b)
	{
		NS_LOG_WARN("GetChannel rejected: null node.");
		return nullptr;
	}

	const uint32_t ida = a->GetId();
	const uint32_t idb = b->GetId();
	const auto key = std::make_pair(std::min(ida, idb), std::max(ida, idb));

	auto it = channels_.find(key);
	if (it == channels_.end())
	{
		NS_LOG_WARN("GetChannel failed: no channel connects the provided nodes.");
		return nullptr;
	}

	return it->second;
}

// Adapted from q2ns::NetController::InstallQuantumLink()
ns3::Ptr<QdlFsoQChannel>
QdlFsoHelper::InstallQdlFsoQuantumLink(ns3::Ptr<q2ns::QNode> a,
									   ns3::Ptr<q2ns::QNode> b,
									   ns3::Ptr<QdlFsoOpticalTerminal> aT,
									   ns3::Ptr<QdlFsoOpticalTerminal> bT,
									   ns3::Ptr<QdlFsoPropagationLossModel> lossModel)
{
	if (!a || !b)
	{
		NS_LOG_WARN("InstallQdlFsoQuantumLink rejected: null node.");
		return nullptr;
	}

	const uint32_t ida = a->GetId();
	const uint32_t idb = b->GetId();
	const auto key = std::make_pair(std::min(ida, idb), std::max(ida, idb));

	auto it = channels_.find(key);
	if (it != channels_.end())
	{
		NS_LOG_WARN("InstallQdlFsoQuantumLink reused existing channel for nodes " << ida << " and " << idb << ".");
		return it->second;
	}

	auto devA = ns3::CreateObject<q2ns::QNetDevice>();
	auto devB = ns3::CreateObject<q2ns::QNetDevice>();

	devA->AggregateObject(aT);
	devB->AggregateObject(bT);

	auto chan = ns3::CreateObject<QdlFsoQChannel>();

	if (streamsAssigned_)
	{
		nextStream_ += chan->AssignStreams(nextStream_);
	}

	auto mobA = a->GetObject<MobilityModel>();
	auto mobB = b->GetObject<MobilityModel>();

	if (!lossModel)
	{
		lossModel = CreateDefaultPropagationLossModel();
	}

	lossModel->SetTerminals(aT, mobA, bT, mobB);
	chan->SetPropagationLossModel(lossModel);

	// The default delay model is ConstantSpeedPropagationDelayModel
	Ptr<ConstantSpeedPropagationDelayModel> delayModel = CreateObject<ConstantSpeedPropagationDelayModel>();
	chan->SetPropagationDelayModel(delayModel);

	chan->Connect(devA, devB);

	a->AddDevice(devA);
	b->AddDevice(devB);

	const uint32_t ifA = a->AddInterface(devA);
	const uint32_t ifB = b->AddInterface(devB);

	a->AddRoute(idb, ifA);
	b->AddRoute(ida, ifB);

	channels_[key] = chan;

	NS_LOG_DEBUG("[QdlFsoQLink " << chan->GetId() << "] " << Names::FindName(a) << " <-> " << Names::FindName(b)
								 << ", n" << ida << " <-> n" << idb << ", iA" << ifA << " <-> iB" << ifB);
	return chan;
}

} // namespace ns3
