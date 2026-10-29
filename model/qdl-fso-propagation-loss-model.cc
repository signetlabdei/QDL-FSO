#include "qdl-fso-propagation-loss-model.h"

#include "qdl-fso-atmospheric-model.h"
#include "qdl-fso-beam-wander-model.h"
#include "qdl-fso-cn2-model.h"
#include "qdl-fso-geometric-pointing-error-model.h"
#include "qdl-fso-optical-terminal.h"
#include "qdl-fso-rytov-model.h"
#include "qdl-fso-scintillation-model.h"

#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/pointer.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("QdlFsoPropagationLossModel");

NS_OBJECT_ENSURE_REGISTERED(QdlFsoPropagationLossModel);

double QdlFsoPropagationLossModel::ALT_TOLERANCE = 100.0;

TypeId
QdlFsoPropagationLossModel::GetTypeId()
{
	static TypeId tid =
		TypeId("ns3::QdlFsoPropagationLossModel")
			.SetParent<PropagationLossModel>()
			.SetGroupName("QdlFso")
			.AddConstructor<QdlFsoPropagationLossModel>()
			.AddAttribute("Wavelength",
						  "The wavelength in m.",
						  DoubleValue(1550e-9),
						  MakeDoubleAccessor(&QdlFsoPropagationLossModel::SetWavelength,
											 &QdlFsoPropagationLossModel::GetWavelength),
						  MakeDoubleChecker<double>())
			.AddAttribute("IsQuantum",
						  "Boolean flag that specifies if this propagation loss model is to be used for classical or "
						  "quantum communications.",
						  BooleanValue(true),
						  MakeBooleanAccessor(&QdlFsoPropagationLossModel::m_isQuantum),
						  MakeBooleanChecker())
			.AddAttribute("ChannelConditionModel",
						  "Pointer to the channel condition model.",
						  PointerValue(),
						  MakePointerAccessor(&QdlFsoPropagationLossModel::SetChannelConditionModel,
											  &QdlFsoPropagationLossModel::GetChannelConditionModel),
						  MakePointerChecker<ChannelConditionModel>())
			.AddAttribute("AtmosphericModel",
						  "Atmospheric attenuation model.",
						  PointerValue(),
						  MakePointerAccessor(&QdlFsoPropagationLossModel::SetAtmosphericModel,
											  &QdlFsoPropagationLossModel::GetAtmosphericModel),
						  MakePointerChecker<QdlFsoAtmosphericModel>())
			.AddAttribute(
				"Cn2Model",
				"Refractive index structure parameter model.",
				PointerValue(),
				MakePointerAccessor(&QdlFsoPropagationLossModel::SetCn2Model, &QdlFsoPropagationLossModel::GetCn2Model),
				MakePointerChecker<QdlFsoCn2Model>())
			.AddAttribute("RytovModel",
						  "Rytov variance calculation model.",
						  PointerValue(),
						  MakePointerAccessor(&QdlFsoPropagationLossModel::SetRytovModel,
											  &QdlFsoPropagationLossModel::GetRytovModel),
						  MakePointerChecker<QdlFsoRytovModel>())
			.AddAttribute("GeometricPointingErrorModel",
						  "Pointing error and jitter loss model.",
						  PointerValue(),
						  MakePointerAccessor(&QdlFsoPropagationLossModel::SetGeometricPointingErrorModel,
											  &QdlFsoPropagationLossModel::GetGeometricPointingErrorModel),
						  MakePointerChecker<QdlFsoGeometricPointingErrorModel>())
			.AddAttribute("BeamWanderModel",
						  "Beam wander model.",
						  PointerValue(),
						  MakePointerAccessor(&QdlFsoPropagationLossModel::SetBeamWanderModel,
											  &QdlFsoPropagationLossModel::GetBeamWanderModel),
						  MakePointerChecker<QdlFsoBeamWanderModel>())
			.AddAttribute("ScintillationModel",
						  "Scintillation and irradiance model.",
						  PointerValue(),
						  MakePointerAccessor(&QdlFsoPropagationLossModel::SetScintillationModel,
											  &QdlFsoPropagationLossModel::GetScintillationModel),
						  MakePointerChecker<QdlFsoScintillationModel>());

	return tid;
}

QdlFsoPropagationLossModel::QdlFsoPropagationLossModel()
{
	NS_LOG_FUNCTION(this);
}

QdlFsoPropagationLossModel::~QdlFsoPropagationLossModel()
{
	NS_LOG_FUNCTION(this);
}

void
QdlFsoPropagationLossModel::DoDispose()
{
	NS_LOG_FUNCTION(this);
	m_channelConditionModel = nullptr;
	m_atmosphericModel = nullptr;
	m_cn2Model = nullptr;
	m_rytovModel = nullptr;
	m_geometricPointingErrorModel = nullptr;
	m_beamWanderModel = nullptr;
	m_scintillationModel = nullptr;
	Object::DoDispose();
}

void
QdlFsoPropagationLossModel::SetChannelConditionModel(Ptr<ChannelConditionModel> model)
{
	NS_LOG_FUNCTION(this);
	m_channelConditionModel = model;
}

Ptr<ChannelConditionModel>
QdlFsoPropagationLossModel::GetChannelConditionModel() const
{
	NS_LOG_FUNCTION(this);
	return m_channelConditionModel;
}

void
QdlFsoPropagationLossModel::SetAtmosphericModel(Ptr<QdlFsoAtmosphericModel> model)
{
	NS_LOG_FUNCTION(this);
	m_atmosphericModel = model;
}

void
QdlFsoPropagationLossModel::SetWavelength(double w)
{
	NS_LOG_FUNCTION(this);
	if (m_terminalA)
	{
		NS_ASSERT_MSG(m_terminalA->GetWavelength() == w,
					  "Changing the channel wavelength after SetTerminals() would break the "
					  "wavelength match with the installed optical terminals.");
	}
	m_wavelength = w;
}

double
QdlFsoPropagationLossModel::GetWavelength() const
{
	NS_LOG_FUNCTION(this);
	return m_wavelength;
}

void
QdlFsoPropagationLossModel::SetTerminals(Ptr<QdlFsoOpticalTerminal> terminalA,
										 Ptr<MobilityModel> mobA,
										 Ptr<QdlFsoOpticalTerminal> terminalB,
										 Ptr<MobilityModel> mobB)
{
	NS_LOG_FUNCTION(this);

	NS_ASSERT_MSG(terminalA && terminalB && mobA && mobB, "All endpoints must be non-null.");

	NS_ASSERT_MSG(terminalA->GetWavelength() == terminalB->GetWavelength(),
				  "Both optical terminals bound to this channel must share the same wavelength.");

	NS_ASSERT_MSG(terminalA->GetWavelength() == m_wavelength,
				  "The wavelength of both optical terminals and the channel must be equal! terminals: "
					  << terminalA->GetWavelength() << " != channel: " << m_wavelength);

	m_terminalA = terminalA;
	m_mobA = mobA;
	m_terminalB = terminalB;
	m_mobB = mobB;
}

void
QdlFsoPropagationLossModel::SetCn2Model(Ptr<QdlFsoCn2Model> model)
{
	NS_LOG_FUNCTION(this);
	m_cn2Model = model;
}

void
QdlFsoPropagationLossModel::SetRytovModel(Ptr<QdlFsoRytovModel> model)
{
	NS_LOG_FUNCTION(this);

	NS_ASSERT_MSG(m_cn2Model != nullptr,
				  "QdlFsoPropagationLossModel: Cannot set RytovModel because m_cn2Model is null on ChannelCore."
				  "Set the Cn2Model on ChannelCore first.");

	NS_ASSERT_MSG(
		model->GetCn2Model() != nullptr,
		"QdlFsoPropagationLossModel: The internal Cn2Model of the provided RytovModel is null. Set the Cn2Model on "
		"RytovModel first.");

	NS_ASSERT_MSG(m_cn2Model == model->GetCn2Model(),
				  "QdlFsoPropagationLossModel: Mismatch detected. The RytovModel provided must use the exact same "
				  "Cn2Model instance as the ChannelCore.");

	m_rytovModel = model;
}

void
QdlFsoPropagationLossModel::SetGeometricPointingErrorModel(Ptr<QdlFsoGeometricPointingErrorModel> model)
{
	NS_LOG_FUNCTION(this);
	m_geometricPointingErrorModel = model;
}

void
QdlFsoPropagationLossModel::SetBeamWanderModel(Ptr<QdlFsoBeamWanderModel> model)
{
	NS_LOG_FUNCTION(this);

	NS_ASSERT_MSG(m_cn2Model != nullptr,
				  "QdlFsoPropagationLossModel: Cannot set BeamWanderModel because m_cn2Model is null on ChannelCore."
				  "Set the Cn2Model on ChannelCore first.");

	NS_ASSERT_MSG(model->GetCn2Model() != nullptr,
				  "QdlFsoPropagationLossModel: The internal Cn2Model of the provided BeamWanderModel is null. Set the "
				  "Cn2Model on BeamWanderModel first.");

	NS_ASSERT_MSG(
		m_cn2Model == model->GetCn2Model(),
		"QdlFsoPropagationLossModel: Mismatch detected. The BeamWanderModel provided must use the exact same Cn2Model "
		"instance as the ChannelCore.");

	m_beamWanderModel = model;
}

void
QdlFsoPropagationLossModel::SetScintillationModel(Ptr<QdlFsoScintillationModel> model)
{
	NS_LOG_FUNCTION(this);
	m_scintillationModel = model;
}

Ptr<QdlFsoAtmosphericModel>
QdlFsoPropagationLossModel::GetAtmosphericModel() const
{
	NS_LOG_FUNCTION(this);
	return m_atmosphericModel;
}

Ptr<QdlFsoCn2Model>
QdlFsoPropagationLossModel::GetCn2Model() const
{
	NS_LOG_FUNCTION(this);
	return m_cn2Model;
}

Ptr<QdlFsoRytovModel>
QdlFsoPropagationLossModel::GetRytovModel() const
{
	NS_LOG_FUNCTION(this);
	return m_rytovModel;
}

Ptr<QdlFsoGeometricPointingErrorModel>
QdlFsoPropagationLossModel::GetGeometricPointingErrorModel() const
{
	NS_LOG_FUNCTION(this);
	return m_geometricPointingErrorModel;
}

Ptr<QdlFsoBeamWanderModel>
QdlFsoPropagationLossModel::GetBeamWanderModel() const
{
	NS_LOG_FUNCTION(this);
	return m_beamWanderModel;
}

Ptr<QdlFsoScintillationModel>
QdlFsoPropagationLossModel::GetScintillationModel() const
{
	NS_LOG_FUNCTION(this);
	return m_scintillationModel;
}

double
QdlFsoPropagationLossModel::GetAltitudeTolerance()
{
	return ALT_TOLERANCE;
}

void
QdlFsoPropagationLossModel::SetAltitudeTolerance(double tolerance)
{
	ALT_TOLERANCE = tolerance;
}

int64_t
QdlFsoPropagationLossModel::DoAssignStreams(int64_t stream)
{
	NS_LOG_FUNCTION(this << stream);
	int64_t currentStream = stream;

	if (m_geometricPointingErrorModel != nullptr)
	{
		currentStream += m_geometricPointingErrorModel->AssignStreams(currentStream);
	}

	if (m_scintillationModel != nullptr)
	{
		currentStream += m_scintillationModel->AssignStreams(currentStream);
	}

	return (currentStream - stream);
}

QdlFsoPropagationContext
QdlFsoPropagationLossModel::ComputeCurrentContext(Ptr<MobilityModel> tx_mob, Ptr<MobilityModel> rx_mob) const
{
	NS_LOG_FUNCTION(this);

	NS_ASSERT_MSG(m_terminalA && m_terminalB, "Call SetTerminals() before using this model.");

	QdlFsoPropagationContext curr_ctx;

	if (tx_mob == m_mobA && rx_mob == m_mobB)
	{
		curr_ctx.tx = m_terminalA;
		curr_ctx.rx = m_terminalB;
	}
	else if (tx_mob == m_mobB && rx_mob == m_mobA)
	{
		curr_ctx.tx = m_terminalB;
		curr_ctx.rx = m_terminalA;
	}
	else
	{
		NS_FATAL_ERROR("ComputeCurrentContext called with mobility models that don't match "
					   "this model's bound endpoints. Did you reuse a QdlFsoPropagationLossModel "
					   "across multiple links?");
	}

	curr_ctx.wavelength = curr_ctx.tx->GetWavelength();
	curr_ctx.mob_tx = tx_mob;
	curr_ctx.mob_rx = rx_mob;
	// TODO: As of now we are using a simple xyz coordinate system. Is there need to make it robust with more complex
	// systems (e.g., for satellites)?
	curr_ctx.txAltitude = tx_mob->GetPosition().z;
	curr_ctx.rxAltitude = rx_mob->GetPosition().z;
	curr_ctx.pathLength = tx_mob->GetDistanceFrom(rx_mob);

	if (std::abs(curr_ctx.txAltitude - curr_ctx.rxAltitude) <= QdlFsoPropagationLossModel::ALT_TOLERANCE)
	{
		curr_ctx.scenario = QdlFsoScenario::Horizontal;
		double deltaZ = std::abs(curr_ctx.txAltitude - curr_ctx.rxAltitude);
		curr_ctx.zenithAngle = std::acos(std::min(1.0, deltaZ / curr_ctx.pathLength));
	}
	else
	{
		double deltaZ = std::abs(curr_ctx.txAltitude - curr_ctx.rxAltitude);
		curr_ctx.zenithAngle = std::acos(std::min(1.0, deltaZ / curr_ctx.pathLength));
		curr_ctx.scenario =
			(curr_ctx.txAltitude > curr_ctx.rxAltitude) ? QdlFsoScenario::Downlink : QdlFsoScenario::Uplink;
	}

	return curr_ctx;
}

double
QdlFsoPropagationLossModel::DoCalcRxPower(double txPowerDbm, Ptr<MobilityModel> tx_mob, Ptr<MobilityModel> rx_mob) const
{
	NS_LOG_FUNCTION(this);

	NS_ASSERT_MSG(!m_isQuantum,
				  "DoCalcRxPower cannot be called when using a quantum channel. Use GetSurvivalProbability instead.");

	// Check if the model is initialized
	NS_ASSERT_MSG(m_wavelength != 0.0, "First set the wavelength!");
	NS_ASSERT_MSG(m_mobA != nullptr, "First set the Mobility model A!");
	NS_ASSERT_MSG(m_mobB != nullptr, "First set the Mobility model B!");
	NS_ASSERT_MSG(m_terminalA != nullptr, "First set the Optical Terminal A!");
	NS_ASSERT_MSG(m_terminalB != nullptr, "First set the Optical Terminal B!");
	NS_ASSERT_MSG(m_atmosphericModel != nullptr, "Atmospheric model not set!");
	NS_ASSERT_MSG(m_cn2Model != nullptr, "Cn2 model not set!");
	NS_ASSERT_MSG(m_rytovModel != nullptr, "Rytov model not set!");
	NS_ASSERT_MSG(m_beamWanderModel != nullptr, "Beam wander model not set!");
	NS_ASSERT_MSG(m_geometricPointingErrorModel != nullptr, "Geometric and Pointing error model not set!");
	NS_ASSERT_MSG(m_scintillationModel != nullptr, "Scintillation model not set!");

	// Retrieve the channel condition
	NS_ASSERT_MSG(m_channelConditionModel, "First set the channel condition model");
	Ptr<ChannelCondition> cond = m_channelConditionModel->GetChannelCondition(tx_mob, rx_mob);

	double rxPow = txPowerDbm;
	rxPow -= GetLoss(cond, tx_mob, rx_mob);

	return rxPow;
}

double
QdlFsoPropagationLossModel::GetLoss(Ptr<ChannelCondition> cond,
									Ptr<MobilityModel> tx_mob,
									Ptr<MobilityModel> rx_mob) const
{
	NS_LOG_FUNCTION(this);

	NS_ASSERT_MSG(!m_isQuantum, "GetLoss cannot be called when using a quantum channel. Use GetSurvivalProb instead.");

	double loss = 0;
	switch (cond->GetLosCondition())
	{
	case ChannelCondition::LosConditionValue::LOS:
		loss = GetLossLos(tx_mob, rx_mob);
		break;
	case ChannelCondition::LosConditionValue::NLOS:
		loss = GetLossNlos(tx_mob, rx_mob);
		break;
	default:
		NS_FATAL_ERROR("Unknown channel condition");
	}

	return loss;
}

double
QdlFsoPropagationLossModel::GetLossLos(Ptr<MobilityModel> tx_mob, Ptr<MobilityModel> rx_mob) const
{
	NS_LOG_FUNCTION(this);

	NS_ASSERT_MSG(!m_isQuantum,
				  "GetLossLos cannot be called when using a quantum channel. Use GetSurvivalProbLos instead.");

	// DOUBLE CHECK IF THIS IS OK
	return -10.0 * std::log10(GetTransmissivity(ComputeCurrentContext(tx_mob, rx_mob)));
}

double
QdlFsoPropagationLossModel::GetLossNlos(Ptr<MobilityModel> tx_mob, Ptr<MobilityModel> rx_mob) const
{
	NS_LOG_FUNCTION(this);

	NS_ASSERT_MSG(!m_isQuantum,
				  "GetLossNlos cannot be called when using a quantum channel. Use GetSurvivalProbNLos instead.");

	// YET TO BE IMPLEMENTED
	return -10.0 * std::log10(GetTransmissivity(ComputeCurrentContext(tx_mob, rx_mob)));
}

double
QdlFsoPropagationLossModel::GetSurvivalProb(Ptr<MobilityModel> tx_mob, Ptr<MobilityModel> rx_mob) const
{
	NS_LOG_FUNCTION(this);

	NS_ASSERT_MSG(m_isQuantum,
				  "GetSurvivalProb cannot be called when using a classical channel. Use DoCalcRxPower instead.");

	// Check if the model is initialized
	NS_ASSERT_MSG(m_wavelength != 0.0, "First set the wavelength!");
	NS_ASSERT_MSG(m_mobA != nullptr, "First set the Mobility model A!");
	NS_ASSERT_MSG(m_mobB != nullptr, "First set the Mobility model B!");
	NS_ASSERT_MSG(m_terminalA != nullptr, "First set the Optical Terminal A!");
	NS_ASSERT_MSG(m_terminalB != nullptr, "First set the Optical Terminal B!");
	NS_ASSERT_MSG(m_atmosphericModel != nullptr, "Atmospheric model not set!");
	NS_ASSERT_MSG(m_cn2Model != nullptr, "Cn2 model not set!");
	NS_ASSERT_MSG(m_rytovModel != nullptr, "Rytov model not set!");
	NS_ASSERT_MSG(m_beamWanderModel != nullptr, "Beam wander model not set!");
	NS_ASSERT_MSG(m_geometricPointingErrorModel != nullptr, "Geometric and Pointing error model not set!");
	NS_ASSERT_MSG(m_scintillationModel != nullptr, "Scintillation model not set!");

	// retrieve the channel condition
	NS_ASSERT_MSG(m_channelConditionModel, "First set the channel condition model");
	Ptr<ChannelCondition> cond = m_channelConditionModel->GetChannelCondition(tx_mob, rx_mob);

	double prob = 0;
	switch (cond->GetLosCondition())
	{
	case ChannelCondition::LosConditionValue::LOS:
		prob = GetSurvivalProbLos(tx_mob, rx_mob);
		break;
	case ChannelCondition::LosConditionValue::NLOS:
		prob = GetSurvivalProbNLos(tx_mob, rx_mob);
		break;
	default:
		NS_FATAL_ERROR("Unknown channel condition");
	}

	return prob;
}

double
QdlFsoPropagationLossModel::GetSurvivalProbLos(Ptr<MobilityModel> tx_mob, Ptr<MobilityModel> rx_mob) const
{
	NS_LOG_FUNCTION(this);

	NS_ASSERT_MSG(m_isQuantum,
				  "GetSurvivalProbLos cannot be called when using a classical channel. Use GetLossLos instead.");

	double eta = GetTransmissivity(ComputeCurrentContext(tx_mob, rx_mob));
	// DOUBLE CHECK IF THIS IS OK
	return std::clamp(eta, 0.0, 1.0);
}

double
QdlFsoPropagationLossModel::GetSurvivalProbNLos(Ptr<MobilityModel> tx_mob, Ptr<MobilityModel> rx_mob) const
{
	NS_LOG_FUNCTION(this);

	NS_ASSERT_MSG(m_isQuantum,
				  "GetSurvivalProbNLos cannot be called when using a classical channel. Use GetLossNlos instead.");

	// In quantum links, if NLOS the there is no communication
	return 0.0;
}

double
QdlFsoPropagationLossModel::GetTransmissivity(QdlFsoPropagationContext ctx) const
{
	NS_LOG_FUNCTION(this);

	NS_ASSERT_MSG(m_atmosphericModel != nullptr, "Atmospheric model not set!");
	NS_ASSERT_MSG(m_cn2Model != nullptr, "Cn2 model not set!");
	NS_ASSERT_MSG(m_rytovModel != nullptr, "Rytov model not set!");
	NS_ASSERT_MSG(m_beamWanderModel != nullptr, "Beam wander model not set!");
	NS_ASSERT_MSG(m_geometricPointingErrorModel != nullptr, "Geometric and Pointing error model not set!");
	NS_ASSERT_MSG(m_scintillationModel != nullptr, "Scintillation model not set!");

	NS_LOG_DEBUG("[QdlFsoPropagationLossModel] WAVELENGTH " << ctx.wavelength);
	NS_LOG_DEBUG("[QdlFsoPropagationLossModel] SCENARIO " << ctx.scenario);
	NS_LOG_DEBUG("[QdlFsoPropagationLossModel] TX ALT " << ctx.mob_tx->GetPosition().z);
	NS_LOG_DEBUG("[QdlFsoPropagationLossModel] RX ALT " << ctx.mob_rx->GetPosition().z);
	NS_LOG_DEBUG("[QdlFsoPropagationLossModel] PATHLENGTH " << ctx.pathLength);
	NS_LOG_DEBUG("[QdlFsoPropagationLossModel] ZENITH " << ctx.zenithAngle);

	double atmospheric_transmissivity = m_atmosphericModel->GetAtmosphericTransmissivity(ctx);
	NS_LOG_DEBUG("[QdlFsoPropagationLossModel] ATM TRASMISS " << atmospheric_transmissivity);

	double rytov_variance = m_rytovModel->GetRytovVariance(ctx);
	NS_LOG_DEBUG("[QdlFsoPropagationLossModel] RYTOV VAR " << rytov_variance);

	double wander_variance = m_beamWanderModel->GetWanderVariance(ctx, rytov_variance);
	NS_LOG_DEBUG("[QdlFsoPropagationLossModel] WANDER VAR " << wander_variance);

	double geom_and_pointing_eff =
		m_geometricPointingErrorModel->GetPointingEfficiency(ctx, rytov_variance, wander_variance);
	NS_LOG_DEBUG("[QdlFsoPropagationLossModel] GEOM-POINT EFF " << geom_and_pointing_eff);

	double irradiance = m_scintillationModel->GetIrradiance(ctx, rytov_variance);
	NS_LOG_DEBUG("[QdlFsoPropagationLossModel] IRRADIANCE " << irradiance);

	double eta = ctx.tx->GetHardwareEfficiency() * ctx.rx->GetHardwareEfficiency() * atmospheric_transmissivity *
				 geom_and_pointing_eff * irradiance;

	NS_LOG_DEBUG("[QdlFsoPropagationLossModel] FINAL TRANSMISSIVITY " << eta);

	return eta;
}

std::ostream&
operator<<(std::ostream& os, QdlFsoScenario scenario)
{
	switch (scenario)
	{
	case QdlFsoScenario::Horizontal:
		return os << "Horizontal";
	case QdlFsoScenario::Downlink:
		return os << "Downlink";
	case QdlFsoScenario::Uplink:
		return os << "Uplink";
	}

	return os << "Unknown";
}

} // namespace ns3
