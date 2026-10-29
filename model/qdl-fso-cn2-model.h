#ifndef QDL_FSO_CN2_MODEL_H
#define QDL_FSO_CN2_MODEL_H

#include "ns3/object.h"
#include "ns3/type-id.h"

namespace ns3
{

struct QdlFsoPropagationContext;

/**
 * @ingroup qdl-fso
 *
 * @brief Computes the refractive-index structure parameter C_n^2 profile for atmospheric turbulence for the qdl-fso
 * module [Morellato2027].
 *
 * The refractive-index structure parameter C_n^2 characterizes the atmospheric turbulence strength
 * as a function of altitude h. This class implements the Hufnagel-Valley model,
 * which parameterize C_n^2(h) using the high-altitude RMS wind speed v_wind and the ground-level
 * reference turbulence parameter C_n^2(0) [Karakosta2025].
 *
 * For short and horizontal links, C_n^2 is assumed to be constant, while for slant paths (uplink and downlink)
 * it varies with altitude. The computed profile is used across the qdl-fso module by the Rytov,
 * beam wander, and scintillation models to derive channel statistics [Morellato2027].
 *
 * @see [Morellato2027] B. Morellato, R. Di Bella et al., "An FSO Channel Model for ns-3"
 * @see [Karakosta2025] A. Karakosta-Amarantidou et al., "Free-space model for a balloon-based quantum network"
 */
class QdlFsoCn2Model : public Object
{
  public:
	/**
	 * @brief Get the type ID.
	 * @return the object TypeId
	 */
	static TypeId GetTypeId();

	/**
	 * @brief QdlFsoCn2Model constructor
	 */
	QdlFsoCn2Model();

	/**
	 * @brief ~QdlFsoCn2Model destructor
	 */
	~QdlFsoCn2Model() override;

	/**
	 * @brief Get a functional wrapper mapping altitude h (in meters) to C_n^2(h) (in m^{-2/3}).
	 *
	 * @return A std::function wrapping the Hufnagel-Valley profile with the configured parameters.
	 */
	std::function<double(double)> GetCn2Function();

	/**
	 * @brief Set the high-altitude RMS wind speed v_wind.
	 * @param v_wind High-altitude wind speed in m/s (e.g., 21 m/s for default conditions).
	 */
	void SetVWind(double v_wind);

	/**
	 * @brief Set the ground-level refractive-index structure parameter C_n^2(0).
	 * @param Cn2_0 Ground-level reference turbulence strength in m^{-2/3} (e.g., 1.7e-14 m^{-2/3} for default
	 * conditions).
	 */
	void SetCn20(double Cn2_0);

	/**
	 * @brief Get the high-altitude RMS wind speed v_wind.
	 * @return Wind speed in m/s.
	 */
	double GetVWind();

	/**
	 * @brief Get the ground-level refractive-index structure parameter C_n^2(0).
	 * @return Ground-level reference C_n^2 value in m^{-2/3}.
	 */
	double GetCn20();

	// ---- JUST FOR DEBUGGING ----
	/**
	 * @brief Sample the C_n^2 profile along the link propagation path for a given context.
	 *
	 * Used just for debugging.
	 *
	 * Automatically dispatches to horizontal or uplink/downlink evaluation depending on link geometry.
	 *
	 * @param context Channel/link geometry and physical parameters.
	 * @return A vector of (altitude, C_n^2) coordinate pairs along the propagation path.
	 */
	std::vector<std::pair<double, double>> GetCn2Profile(const QdlFsoPropagationContext& context);

	/**
	 * @brief Just for debugging
	 */
	std::vector<std::pair<double, double>> GetCn2ProfileHorizontal(const QdlFsoPropagationContext& context);

	/**
	 * @brief Just for debugging
	 */
	std::vector<std::pair<double, double>> GetCn2ProfileUplinkDownlink(const QdlFsoPropagationContext& context);
	// ----------------------------

  protected:
	// inherited from Object
	void DoDispose() override;

  private:
	/**
	 * @brief Computes the Hufnagel-Valley C_n^2 value at altitude h.
	 *
	 * @param h Altitude above ground level in meters.
	 * @param v_wind High-altitude RMS wind speed in m/s.
	 * @param Cn2_0 Ground-level C_n^2 in m^{-2/3}.
	 * @return C_n^2 value at altitude h in m^{-2/3}.
	 */
	static double HufnagelValley(double h, double v_wind, double Cn2_0);

	double m_v_wind; //!< High-altitude RMS wind speed v_w in m/s
	double m_Cn2_0;	 //!< Ground-level refractive-index structure parameter C_n^2(0) in m^{-2/3}
};

} // namespace ns3

#endif // QDL_FSO_CN2_MODEL_H
