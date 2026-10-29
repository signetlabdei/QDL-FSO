#ifndef QDL_FSO_Q2NS_QCHANNEL_H
#define QDL_FSO_Q2NS_QCHANNEL_H

#include "ns3/mobility-model.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/ptr.h"
#include "ns3/q2ns-qchannel.h"
#include "ns3/q2ns-qmap.h"
#include "ns3/q2ns-qubit.h"
#include "ns3/qdl-fso-propagation-loss-model.h"
#include "ns3/traced-callback.h"
#include "ns3/vector.h"

namespace ns3
{

/**
 * @ingroup qdl-fso
 *
 * @brief An FSO-enabled Quantum Channel integrating the qdl-fso module with Q2NS [Pearson2026,
 * Morellato2027].
 *
 * QdlFsoQChannel extends the q2ns::QChannel base class to support quantum communication over atmospheric optical links.
 * Whenever a qubit transmission is initiated via SendFrom(), the channel evaluates the current physical link state
 * using QdlFsoPropagationLossModel and updates Q2NS quantum maps prior to delivery [Morellato2027]:
 *  - Photon loss: Modeled as an erasure channel configured via q2ns::LossQMap with erasure probability \f$ 1 -
 * \eta_{ch}\f$, where \f$ \eta_{ch} \f$ is the total channel transmissivity/survival probability [Chehimi2025].
 *  - Turbulence-induced phase noise: Modeled as a dephasing channel configured via q2ns::DephasingQMap
 *    with dephasing probability \f$ p_{deph} = 0.5 \cdot erf(\sigma_R^2)\f$, where \f$ \sigma_R^2 \f$ is the Rytov
 * variance [Chehimi2025].
 *  - Propagation delay: Obtained dynamically from a configurable PropagationDelayModel (e.g.,
 * ConstantSpeedPropagationDelayModel).
 *
 * Channel conditions are recomputed at every transmission turn, ensuring each qubit experiences
 * the exact instantaneous physical channel state at its send time.
 * At the moment there is no concept of channel coeherence time or time-correlated effects.
 *
 * @see [Morellato2027] B. Morellato, R. Di Bella et al., "An FSO Channel Model for ns-3"
 * @see [Pearson2026] Pearson et al., "An extensible quantum network simulator built on ns-3: Q2NS design and
 * evaluation"
 * @see [Chehimi2025] M. Chehimi et al., "Reconfigurable Intelligent Surface (RIS)-Assisted Entanglement Distribution in
 * FSO Quantum Networks"
 */
class QdlFsoQChannel : public q2ns::QChannel
{
  public:
	/**
	 * @brief Get the type ID.
	 * @return the object TypeId
	 */
	static ns3::TypeId GetTypeId(void);

	/**
	 * @brief QdlFsoQChannel constructor
	 */
	QdlFsoQChannel();

	/**
	 * @brief ~QdlFsoQChannel destructor
	 */
	~QdlFsoQChannel() override = default;

	/**
	 * @brief Set the underlying physical FSO propagation loss model.
	 * @param QdlFsoPropagationLossModel Pointer to QdlFsoPropagationLossModel instance.
	 */
	void SetPropagationLossModel(ns3::Ptr<ns3::QdlFsoPropagationLossModel> QdlFsoPropagationLossModel);

	/**
	 * @brief Get the underlying physical FSO propagation loss model.
	 * @return Pointer to current QdlFsoPropagationLossModel.
	 */
	ns3::Ptr<ns3::QdlFsoPropagationLossModel> GetPropagationLossModel() const;

	/**
	 * @brief Set the propagation delay model used to compute qubit time of flight.
	 * @param delayModel Pointer to a PropagationDelayModel.
	 */
	void SetPropagationDelayModel(ns3::Ptr<PropagationDelayModel> delayModel);

	/**
	 * @brief Get the active propagation delay model.
	 * @return Pointer to current PropagationDelayModel.
	 */
	ns3::Ptr<PropagationDelayModel> GetPropagationDelayModel() const;

	/**
	 * @brief TracedCallback signature for qubit transmission events over the FSO channel.
	 *
	 * @param qubit Shared pointer to transmitted qubit.
	 * @param transmissivity Instantaneous channel transmissivity (qubit survival probability \f$ \eta_{ch}\f$) at send
	 * time.
	 * @param src Pointer to source mobility model.
	 */
	typedef void (*QubitTxedTracedCallback)(std::shared_ptr<q2ns::Qubit> qubit,
											double transmissivity,
											ns3::Ptr<const ns3::MobilityModel> src);

  protected:
	/**
	 * @brief Overridden transmission hook invoked when sending a qubit through the quantum channel.
	 *
	 * Queries the physical propagation loss model for current transmissivity \f$ \eta_{ch} \f$ and Rytov variance \f$
	 * \sigma_R^2\f$, configures the Q2NS LossQMap and DephasingQMap accordingly, computes propagation delay, and
	 * forwards the qubit via q2ns::QChannel::SendFrom().
	 *
	 * @param src Pointer to sending quantum network device.
	 * @param q Shared pointer to transmitted qubit.
	 * @return True if transmission was successfully initiated.
	 */
	bool SendFrom(q2ns::QNetDevice* src, std::shared_ptr<q2ns::Qubit> q) override;

  private:
	ns3::Ptr<QdlFsoPropagationLossModel> m_propagation_loss; //!< Pointer to FSO physical channel loss model
	ns3::Ptr<PropagationDelayModel> m_propagationDelay;		 //!< Pointer to propagation delay model

	ns3::Ptr<q2ns::LossQMap> m_lossMapAB; //!< Q2NS loss channel map for directional link A -> B
	ns3::Ptr<q2ns::LossQMap> m_lossMapBA; //!< Q2NS loss channel map for directional link B -> A

	bool m_enableDephasing;					   //!< Flag controlling whether turbulence dephasing noise is active
	ns3::Ptr<q2ns::DephasingQMap> m_dephMapAB; //!< Q2NS dephasing channel map for directional link A -> B
	ns3::Ptr<q2ns::DephasingQMap> m_dephMapBA; //!< Q2NS dephasing channel map for directional link B -> A

	ns3::TracedCallback<std::shared_ptr<q2ns::Qubit>,
						double,
						double,
						ns3::Ptr<const ns3::Node>,
						ns3::Ptr<const ns3::Node>>
		m_qubitTxedTrace; //!< Traced callback fired upon qubit transmission
};

} // namespace ns3

#endif // QDL_FSO_Q2NS_QCHANNEL_H
