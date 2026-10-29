#ifndef QDL_FSO_OPTICAL_TERMINAL_H
#define QDL_FSO_OPTICAL_TERMINAL_H

#include "ns3/object.h"
#include "ns3/type-id.h"

namespace ns3
{

/**
 * @ingroup qdl-fso
 *
 * @brief Models the optical hardware properties at an FSO link endpoint for the qdl-fso module [Morellato2027].
 *
 * The QdlFsoOpticalTerminal encapsulates physical and hardware-specific parameters of an optical transmitter or
 * receiver, independently of the propagation environment. This design allows the same terminal class to be shared
 * across classical and quantum communication links.
 *
 * Key parameters modeled by this class include:
 *  - Operational optical wavelength (λ).
 *  - Transmitter beam waist (W_0) and receiver telescope aperture diameter (D_rx).
 *  - Hardware efficiency (η_tx, η_rx), capturing internal optics, fiber coupling, and quantum memory efficiency.
 *  - Pointing jitter (θ_pe) and active tracking-loop correction efficiency (η_tr).
 *  - Optical transmit power (P_tx) used for classical channel power calculations.
 *
 * @see [Morellato2027] B. Morellato, R. Di Bella et al., "An FSO Channel Model for ns-3"
 */
class QdlFsoOpticalTerminal : public Object
{
  public:
	/**
	 * @brief Get the type ID.
	 * @return the object TypeId
	 */
	static TypeId GetTypeId();

	/**
	 * @brief QdlFsoOpticalTerminal constructor
	 */
	QdlFsoOpticalTerminal();

	/**
	 * @brief ~QdlFsoOpticalTerminal destructor
	 */
	~QdlFsoOpticalTerminal() override;

	/**
	 * @brief Get the operating optical wavelength λ.
	 * @return Wavelength in meters.
	 */
	double GetWavelength() const;

	/**
	 * @brief Get the receiver aperture diameter D_rx.
	 * @return Receiver aperture diameter in meters.
	 */
	double GetRxApertureDiameter() const;

	/**
	 * @brief Get the transmitter beam waist radius W_0.
	 * @return Transmitter beam waist radius in meters.
	 */
	double GetTxBeamWaist() const;

	/**
	 * @brief Get the hardware efficiency coefficient (η_tx or η_rx).
	 * @return Linear hardware efficiency in [0, 1].
	 */
	double GetHardwareEfficiency() const;

	/**
	 * @brief Get the tracking-loop correction efficiency η_tr.
	 * @return Tracking efficiency in [0, 1].
	 */
	double GetTrackingEfficiency() const;

	/**
	 * @brief Get the non-turbulence equivalent angular pointing jitter θ_pe.
	 * @return Pointing jitter in radians.
	 */
	double GetPointingJitter() const;

	/**
	 * @brief Get the classical optical transmit power P_tx.
	 * @return Transmit power in Watts.
	 */
	double GetTxPower() const;

	/**
	 * @brief Set the operating optical wavelength λ.
	 * @param wavelength Wavelength in meters (e.g., 1550e-9 m).
	 */
	void SetWavelength(double wavelength);

	/**
	 * @brief Set the receiver aperture diameter D_rx.
	 * @param rxApertureDiameter Receiver telescope diameter in meters.
	 */
	void SetRxApertureDiameter(double rxApertureDiameter);

	/**
	 * @brief Set the transmitter beam waist radius W_0.
	 * @param txBeamWaist Transmitter beam waist radius in meters (typically W_0 = D_tx/2).
	 */
	void SetTxBeamWaist(double txBeamWaist);

	/**
	 * @brief Set the hardware efficiency coefficient (η_tx or η_rx).
	 *
	 * Models deterministic hardware losses including optical mirrors, beam splitters,
	 * fiber coupling, and quantum-memory efficiencies.
	 *
	 * @param hardwareEfficiency Linear efficiency coefficient in [0, 1].
	 */
	void SetHardwareEfficiency(double hardwareEfficiency);

	/**
	 * @brief Set the tracking-loop correction efficiency η_tr.
	 * @param trackingEfficiency Tracking loop efficiency in [0, 1].
	 */
	void SetTrackingEfficiency(double trackingEfficiency);

	/**
	 * @brief Set the non-turbulence equivalent angular pointing jitter θ_pe.
	 *
	 * Accounts for mechanical vibration, structural jitter, and misalignment errors.
	 *
	 * @param pointingJitter Angular pointing jitter in radians.
	 */
	void SetPointingJitter(double pointingJitter);

	/**
	 * @brief Set the classical transmit power P_tx.
	 * @param txPower Optical transmit power in Watts.
	 */
	void SetTxPower(double txPower);

	// ---- For future extensions ----
	//   double GetTxPulseRate() const;
	//   double GetTxMeanPhotonNumber() const;
	//   void SetTxPulseRate(double txPulseRate);
	//   void SetTxMeanPhotonNumber(double txMeanPhotonNumber);
	// -------------------------------

  protected:
	// inherited from Object
	void DoDispose() override;

  private:
	double m_wavelength;		 //!< Operating wavelength λ in meters
	double m_rxApertureDiameter; //!< Receiver aperture diameter D_{rx} in meters
	double m_txBeamWaist;		 //!< Transmitter beam waist radius W_0 in meters
	double m_hardwareEfficiency; //!< Hardware efficiency (η_{tx} or η_{rx}) in [0, 1]
	double m_trackingEfficiency; //!< Tracking-loop correction efficiency η_{tr} in [0, 1]
	double m_pointingJitter;	 //!< Mechanical/angular pointing jitter θ_pe in radians

	// Just for classical
	double m_txPower; //!< Classical optical transmit power P_tx in Watts

	// ---- For future extensions ----
	// Just for quantum
	// double m_txPulseRate;
	// double m_txMeanPhotonNumber;
	// -------------------------------
};

} // namespace ns3

#endif // QDL_FSO_OPTICAL_TERMINAL_H
