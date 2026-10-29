#ifndef QDL_FSO_PROPAGATION_LOSS_H
#define QDL_FSO_PROPAGATION_LOSS_H

#include "ns3/mobility-module.h"
#include "ns3/object.h"
#include "ns3/propagation-module.h"
#include "ns3/type-id.h"

#include <iostream>

namespace ns3
{

class QdlFsoOpticalTerminal;
class QdlFsoAtmosphericModel;
class QdlFsoCn2Model;
class QdlFsoRytovModel;
class QdlFsoGeometricPointingErrorModel;
class QdlFsoBeamWanderModel;
class QdlFsoScintillationModel;

/**
 * @ingroup qdl-fso
 * @enum QdlFsoScenario
 * @brief Link trajectory scenarios supported by the qdl-fso module [Morellato2027].
 *
 * @see [Morellato2027] B. Morellato, R. Di Bella et al., "An FSO Channel Model for ns-3"
 */
enum class QdlFsoScenario
{
	Horizontal, //!< Horizontal link scenario where both terminals share similar altitudes
	Downlink,	//!< Downlink slant path from airborne/spaceborne transmitter to ground/airborne/spaceborne receiver
	Uplink		//!< Uplink slant path from ground/airborne/spaceborne transmitter to airborne/spaceborne receiver
};

/**
 * @ingroup qdl-fso
 * @brief Context structure holding link geometry and physical parameters for an FSO transmission [Morellato2027].
 *
 * @see [Morellato2027] B. Morellato, R. Di Bella et al., "An FSO Channel Model for ns-3"
 */
struct QdlFsoPropagationContext
{
	double wavelength;			   //!< Operating optical wavelength \f$ \lambda \f$ in meters
	QdlFsoScenario scenario;	   //!< Link trajectory type (Horizontal, Downlink, or Uplink)
	Ptr<QdlFsoOpticalTerminal> tx; //!< Transmitter optical terminal hardware model
	Ptr<QdlFsoOpticalTerminal> rx; //!< Receiver optical terminal hardware model
	Ptr<MobilityModel> mob_tx;	   //!< Transmitter mobility model
	Ptr<MobilityModel> mob_rx;	   //!< Receiver mobility model
	double txAltitude;			   //!< Altitude of transmitter above ground level in meters
	double rxAltitude;			   //!< Altitude of receiver above ground level in meters
	double pathLength;			   //!< Line-of-sight propagation distance L in meters
	double zenithAngle;			   //!< Zenith angle \f$ \zeta \f$ in radians measured from ground normal
};

/**
 * @brief Stream insertion operator for QdlFsoScenario.
 */
std::ostream& operator<<(std::ostream& os, QdlFsoScenario scenario);

/**
 * @ingroup qdl-fso
 *
 * @brief Core physical propagation loss model for FSO channels, for the qdl-fso module [Morellato2027].
 *
 * QdlFsoPropagationLossModel computes the stochastic optical link transmissivity \f$ \eta_{ch} \f$ by
 * factorizing independent physical loss and noise mechanisms [Phuchortham2026, Morellato2027]:
 * \f$ \eta_{ch} = \eta_{tx} \cdot \eta_{rx} \cdot \eta_{atm} \cdot \eta_{gpe} \cdot \eta_{scint} \f$
 *
 * Sub-models combined by this class include:
 *  - Hardware efficiencies \f$ (\eta_{tx}, \eta_{rx}) \f$ via QdlFsoOpticalTerminal.
 *  - Atmospheric attenuation \f$ (\eta_{atm}) \f$ via QdlFsoAtmosphericModel (Kim, Kruse, or LOWTRAN).
 *  - Refractive-index structure parameter profile \f$ C_n^2(h) \f$ via QdlFsoCn2Model.
 *  - Rytov turbulence variance \f$ \sigma_R^2 \f$ via QdlFsoRytovModel.
 *  - Turbulence beam wander via QdlFsoBeamWanderModel.
 *  - Geometric divergence, beam spreading, and pointing jitter \f$ (\eta_{gpe}) \f$ via
 * QdlFsoGeometricPointingErrorModel.
 *  - Gamma-Gamma scintillation fluctuations \f$ (\eta_{scint}) \f$ via QdlFsoScintillationModel.
 *
 * The model serves both classical and quantum network simulation stacks:
 *  - Classical links: Overrides DoCalcRxPower() to scale transmit power \f$ P_{rx} = P_{tx} \cdot \eta_{ch}\f$.
 *  - Quantum links: Exposes GetSurvivalProb() to provide photon/qubit survival probability clamped to [0, 1].
 *
 * @see [Morellato2027] B. Morellato, R. Di Bella et al., "An FSO Channel Model for ns-3"
 * @see [Phuchortham2026] P. Phuchortham et al., "A Review of the Structure of Free-Space Optical Channel Models:
 * Physical Meaning, Assumptions, and Atmospheric Conditions"
 */
class QdlFsoPropagationLossModel : public PropagationLossModel
{
  public:
	/**
	 * @brief Get the type ID.
	 * @return the object TypeId
	 */
	static TypeId GetTypeId();

	/**
	 * @brief QdlFsoPropagationLossModel constructor
	 */
	QdlFsoPropagationLossModel();

	/**
	 * @brief ~QdlFsoPropagationLossModel destructor
	 */
	~QdlFsoPropagationLossModel() override;

	// Delete copy constructor and assignment operator to avoid misuse
	QdlFsoPropagationLossModel(const QdlFsoPropagationLossModel&) = delete;
	QdlFsoPropagationLossModel& operator=(const QdlFsoPropagationLossModel&) = delete;

	/**
	 * @brief Set the optical operating wavelength \lambda.
	 * @param w Wavelength in meters.
	 */
	void SetWavelength(double w);

	/**
	 * @brief Get the optical operating wavelength \lambda.
	 * @return Wavelength in meters.
	 */
	double GetWavelength() const;

	/**
	 * @brief Set the optical terminals and mobility models for link endpoints A and B.
	 * @param terminalA Pointer to optical terminal at endpoint A.
	 * @param mobA Pointer to mobility model at endpoint A.
	 * @param terminalB Pointer to optical terminal at endpoint B.
	 * @param mobB Pointer to mobility model at endpoint B.
	 */
	void SetTerminals(Ptr<QdlFsoOpticalTerminal> terminalA,
					  Ptr<MobilityModel> mobA,
					  Ptr<QdlFsoOpticalTerminal> terminalB,
					  Ptr<MobilityModel> mobB);

	/**
	 * @brief Set the atmospheric attenuation model.
	 * @param model Pointer to a QdlFsoAtmosphericModel.
	 */
	void SetAtmosphericModel(Ptr<QdlFsoAtmosphericModel> model);

	/**
	 * @brief Set the refractive-index structure parameter \f$ (C_n^2) \f$ model.
	 * @param model Pointer to a QdlFsoCn2Model.
	 */
	void SetCn2Model(Ptr<QdlFsoCn2Model> model);

	/**
	 * @brief Set the Rytov variance model.
	 * @param model Pointer to a QdlFsoRytovModel.
	 */
	void SetRytovModel(Ptr<QdlFsoRytovModel> model);

	/**
	 * @brief Set the geometric loss and pointing error model.
	 * @param model Pointer to a QdlFsoGeometricPointingErrorModel.
	 */
	void SetGeometricPointingErrorModel(Ptr<QdlFsoGeometricPointingErrorModel> model);

	/**
	 * @brief Set the beam wander model.
	 * @param model Pointer to a QdlFsoBeamWanderModel.
	 */
	void SetBeamWanderModel(Ptr<QdlFsoBeamWanderModel> model);

	/**
	 * @brief Set the scintillation model.
	 * @param model Pointer to a QdlFsoScintillationModel.
	 */
	void SetScintillationModel(Ptr<QdlFsoScintillationModel> model);

	/**
	 * @brief Get the atmospheric attenuation model.
	 * @return Pointer to current QdlFsoAtmosphericModel.
	 */
	Ptr<QdlFsoAtmosphericModel> GetAtmosphericModel() const;

	/**
	 * @brief Get the refractive-index structure parameter \f$ (C_n^2) \f$ model.
	 * @return Pointer to current QdlFsoCn2Model.
	 */
	Ptr<QdlFsoCn2Model> GetCn2Model() const;

	/**
	 * @brief Get the Rytov variance model.
	 * @return Pointer to current QdlFsoRytovModel.
	 */
	Ptr<QdlFsoRytovModel> GetRytovModel() const;

	/**
	 * @brief Get the geometric loss and pointing error model.
	 * @return Pointer to current QdlFsoGeometricPointingErrorModel.
	 */
	Ptr<QdlFsoGeometricPointingErrorModel> GetGeometricPointingErrorModel() const;

	/**
	 * @brief Get the beam wander model.
	 * @return Pointer to current QdlFsoBeamWanderModel.
	 */
	Ptr<QdlFsoBeamWanderModel> GetBeamWanderModel() const;

	/**
	 * @brief Get the scintillation model.
	 * @return Pointer to current QdlFsoScintillationModel.
	 */
	Ptr<QdlFsoScintillationModel> GetScintillationModel() const;

	/**
	 * @brief Set the channel condition model used to determine channel state (e.g., LOS/NLOS condition).
	 * @param model Pointer to a ChannelConditionModel.
	 */
	void SetChannelConditionModel(Ptr<ChannelConditionModel> model);

	/**
	 * @brief Get the associated channel condition model.
	 * @return Pointer to the channel condition model.
	 */
	Ptr<ChannelConditionModel> GetChannelConditionModel() const;

	/**
	 * @brief Calculates overall linear link transmissivity \f$ \eta_{ch} \f$ for a given context.
	 *
	 * Combines deterministic components \f$(\eta_{tx}, \eta_{rx}, \eta_{atm}) \f$ and stochastic components \f$
	 * (\eta_{gpe}, \eta_{scint}) \f$.
	 *
	 * @param ctx Context structure containing geometry and terminal parameters.
	 * @return Linear dimensionless transmissivity \f$ \eta_{ch} \f$.
	 */
	double GetTransmissivity(QdlFsoPropagationContext ctx) const;

	/**
	 * @brief Computes qubit survival probability for quantum channel transmission.
	 *
	 * Evaluates total channel transmissivity \f$ \eta_{ch} \f$, accounts for blockage (returns 0 under NLOS),
	 * and clamps the resulting value to [0, 1].
	 *
	 * @param tx_mob Mobility model of transmitter node.
	 * @param rx_mob Mobility model of receiver node.
	 * @return Survival probability in [0, 1].
	 */
	double GetSurvivalProb(Ptr<MobilityModel> tx_mob, Ptr<MobilityModel> rx_mob) const;

	static double ALT_TOLERANCE; //!< Altitude difference threshold in meters for classifying links as horizontal

	/**
	 * @brief Get altitude difference tolerance for horizontal link classification.
	 * @return Tolerance in meters.
	 */
	static double GetAltitudeTolerance();

	/**
	 * @brief Set altitude difference tolerance for horizontal link classification.
	 * @param tolerance Altitude tolerance threshold in meters.
	 */
	static void SetAltitudeTolerance(double tolerance);

	/**
	 * @brief Computes current link propagation context from terminal positions and mobility models.
	 *
	 * Determines path length L, terminal altitudes, zenith angle \f$ \zeta \f$, and link scenario (Horizontal,
	 * Downlink, or Uplink).
	 *
	 * @param tx_mob Transmitter mobility model.
	 * @param rx_mob Receiver mobility model.
	 * @return Computed QdlFsoPropagationContext structure.
	 */
	QdlFsoPropagationContext ComputeCurrentContext(Ptr<MobilityModel> tx_mob, Ptr<MobilityModel> rx_mob) const;

  protected:
	// inherited from Object
	void DoDispose() override;

	Ptr<ChannelConditionModel> m_channelConditionModel; //!< Pointer to channel condition model (e.g., in qdl-fso the
														//!< default is BuildingsChannelConditionModel)

	/**
	 * @brief Assigns a fixed random stream index to underlying random variable streams.
	 * @param stream First stream index to allocate.
	 * @return Number of streams allocated across all sub-models.
	 */
	int64_t DoAssignStreams(int64_t stream) override;

  private:
	// QdlFsoPropagationContext ComputeCurrentContext(Ptr<MobilityModel> tx_mob, Ptr<MobilityModel> rx_mob) const;

	/**
	 * @brief Calculates classical received power \f$ P_{rx} = P_{tx} \cdot \eta_{ch} \f$.
	 *
	 * Overrides ns-3 PropagationLossModel interface for classical channel calculation.
	 *
	 * @param txPowerDbm Optical transmit power in dBm.
	 * @param tx_mob Transmitter mobility model.
	 * @param rx_mob Receiver mobility model.
	 * @return Received power in dBm.
	 */
	double DoCalcRxPower(double txPowerDbm, Ptr<MobilityModel> tx_mob, Ptr<MobilityModel> rx_mob) const override;

	/**
	 * @brief Computes pathloss in dB given the channel condition.
	 * @param cond Channel condition state (LOS/NLOS).
	 * @param tx_mob Transmitter mobility model.
	 * @param rx_mob Receiver mobility model.
	 * @return Pathloss value in dB.
	 */
	double GetLoss(Ptr<ChannelCondition> cond, Ptr<MobilityModel> tx_mob, Ptr<MobilityModel> rx_mob) const;

	/**
	 * @brief Computes pathloss in dB assuming unobstructed Line-of-Sight (LOS).
	 * @param tx_mob Transmitter mobility model.
	 * @param rx_mob Receiver mobility model.
	 * @return Pathloss value in dB.
	 */
	double GetLossLos(Ptr<MobilityModel> tx_mob, Ptr<MobilityModel> rx_mob) const;

	/**
	 * @brief Computes pathloss in dB assuming obstructed Non-Line-of-Sight (NLOS).
	 * NOT YET IMPLEMENTED!
	 * @param tx_mob Transmitter mobility model.
	 * @param rx_mob Receiver mobility model.
	 * @return Pathloss value in dB.
	 */
	double GetLossNlos(Ptr<MobilityModel> tx_mob, Ptr<MobilityModel> rx_mob) const;

	/**
	 * @brief Computes qubit survival probability under Line-of-Sight (LOS) conditions.
	 * @param tx_mob Transmitter mobility model.
	 * @param rx_mob Receiver mobility model.
	 * @return Survival probability in [0, 1].
	 */
	double GetSurvivalProbLos(Ptr<MobilityModel> tx_mob, Ptr<MobilityModel> rx_mob) const;

	/**
	 * @brief Computes qubit survival probability under Non-Line-of-Sight (NLOS) blockage.
	 * @param tx_mob Transmitter mobility model.
	 * @param rx_mob Receiver mobility model.
	 * @return Survival probability (0.0).
	 */
	double GetSurvivalProbNLos(Ptr<MobilityModel> tx_mob, Ptr<MobilityModel> rx_mob) const;

	double m_wavelength; //!< Default operating wavelength in meters

	Ptr<QdlFsoAtmosphericModel> m_atmosphericModel; //!< Sub-model for atmospheric attenuation \f$ \eta_{atm} \f$
	Ptr<QdlFsoCn2Model> m_cn2Model;					//!< Sub-model for \f$ C_n^2 \f$ turbulence profile
	Ptr<QdlFsoRytovModel> m_rytovModel;				//!< Sub-model for Rytov variance \f$ \sigma_R^2 \f$
	Ptr<QdlFsoGeometricPointingErrorModel>
		m_geometricPointingErrorModel; //!< Sub-model for pointing loss and geometric divergence \f$ \eta_{gpe} \f$
	Ptr<QdlFsoBeamWanderModel> m_beamWanderModel; //!< Sub-model for turbulence beam wander <r_c^2> [Morellato2027]
	Ptr<QdlFsoScintillationModel>
		m_scintillationModel; //!< Sub-model for Gamma-Gamma scintillation \f$ \eta_{scint} \f$

	Ptr<QdlFsoOpticalTerminal> m_terminalA; //!< Optical terminal hardware configuration for node A
	Ptr<MobilityModel> m_mobA;				//!< Mobility model for node A
	Ptr<QdlFsoOpticalTerminal> m_terminalB; //!< Optical terminal hardware configuration for node B
	Ptr<MobilityModel> m_mobB;				//!< Mobility model for node B

	bool m_isQuantum; //!< Flag indicating whether channel operates in quantum mode
};

} // namespace ns3

#endif // QDL_FSO_PROPAGATION_LOSS_H
