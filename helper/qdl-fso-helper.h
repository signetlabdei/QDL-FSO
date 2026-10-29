#ifndef QDL_FSO_HELPER_H
#define QDL_FSO_HELPER_H

#include "ns3/q2ns-qnetworker.h"
#include "ns3/q2ns-qnode.h"
#include "ns3/qdl-fso-optical-terminal.h"
#include "ns3/qdl-fso-propagation-loss-model.h"
#include "ns3/qdl-fso-q2ns-qchannel.h"

namespace ns3
{

/**
 * @ingroup qdl-fso
 *
 * @brief Helper class to simplify creation, configuration, and management of FSO quantum links, for the qdl-fso module
 * [Morellato2027].
 *
 * QdlFsoHelper mirrors the channel-creation workflow of q2ns::NetController by providing helper methods such as
 * InstallQdlFsoQuantumLink() to set up FSO links between quantum nodes [Pearson2026, Morellato2027].
 *
 * Users may supply a pre-configured QdlFsoPropagationLossModel, or let the helper instantiate a default model populated
 * with default sub-components [Morellato2027]:
 *  - BuildingsChannelConditionModel
 *  - QdlFsoLowtranAtmosphericModel
 *  - QdlFsoCn2Model
 *  - QdlFsoRytovModel
 *  - QdlFsoBeamWanderModel
 *  - QdlFsoGeometricPointingErrorModel
 *  - QdlFsoGammaGammaScintillationModel
 *
 * @see [Morellato2027] B. Morellato, R. Di Bella et al., "An FSO Channel Model for ns-3"
 * @see [Pearson2026] Pearson et al., "An extensible quantum network simulator built on ns-3: Q2NS design and
 * evaluation"
 */
class QdlFsoHelper : public Object
{
  public:
	/**
	 * @brief Get the type ID.
	 * @return the object TypeId
	 */
	static ns3::TypeId GetTypeId(void);

	/**
	 * @brief QdlFsoHelper constructor
	 */
	QdlFsoHelper();

	/**
	 * @brief ~QdlFsoHelper destructor
	 */
	~QdlFsoHelper() override;

	/**
	 * @brief Ensures random variable stream indices have been assigned across managed channels.
	 */
	void EnsureStreamsAssigned_();

	/**
	 * @brief Assigns a fixed random stream index to underlying random variable streams.
	 * @param stream First stream index to allocate.
	 * @return Total number of stream indices allocated.
	 */
	int64_t AssignStreams(int64_t stream);

	/**
	 * @brief Instantiates a default QdlFsoPropagationLossModel populated with default attributes and sub-models.
	 *
	 * Composes a model containing a BuildingsChannelConditionModel, QdlFsoLowtranAtmosphericModel,
	 * QdlFsoCn2Model, QdlFsoRytovModel, QdlFsoBeamWanderModel, QdlFsoGeometricPointingErrorModel,
	 * and QdlFsoGammaGammaScintillationModel.
	 *
	 * @return Pointer to a newly allocated default QdlFsoPropagationLossModel.
	 */
	ns3::Ptr<QdlFsoPropagationLossModel> CreateDefaultPropagationLossModel();

	/**
	 * @brief Retrieves an existing QdlFsoQChannel installed between two quantum nodes.
	 *
	 * @param a Pointer to quantum node A.
	 * @param b Pointer to quantum node B.
	 * @return Pointer to the corresponding QdlFsoQChannel, or nullptr if no channel exists between the pair.
	 */
	ns3::Ptr<QdlFsoQChannel> GetChannel(ns3::Ptr<q2ns::QNode> a, ns3::Ptr<q2ns::QNode> b);

	/**
	 * @brief Installs and configures an FSO quantum channel between two quantum nodes.
	 *
	 * Creates a QdlFsoQChannel connecting node a and node b, attaches their optical terminal hardware models,
	 * and associates the channel with a physical propagation loss model.
	 * If lossModel is nullptr, CreateDefaultPropagationLossModel() is called automatically.
	 *
	 * @param a Pointer to quantum node A.
	 * @param b Pointer to quantum node B.
	 * @param aT Pointer to optical terminal at node A.
	 * @param bT Pointer to optical terminal at node B.
	 * @param lossModel Optional pointer to a custom QdlFsoPropagationLossModel.
	 * @return Pointer to the installed QdlFsoQChannel.
	 */
	ns3::Ptr<QdlFsoQChannel> InstallQdlFsoQuantumLink(Ptr<q2ns::QNode> a,
													  Ptr<q2ns::QNode> b,
													  Ptr<QdlFsoOpticalTerminal> aT,
													  Ptr<QdlFsoOpticalTerminal> bT,
													  Ptr<QdlFsoPropagationLossModel> lossModel = nullptr);

  protected:
	// inherited from Object
	void DoDispose() override;

  private:
	bool streamsAssigned_ = false; //!< True once AssignStreams has been executed
	int64_t nextStream_ = 0;	   //!< Next available stream index to allocate

	std::map<std::pair<uint32_t, uint32_t>, ns3::Ptr<QdlFsoQChannel>>
		channels_; //!< Undirected link map storing managed quantum channels keyed by sorted node ID pairs
};

} // namespace ns3

#endif // QDL_FSO_HELPER_H
