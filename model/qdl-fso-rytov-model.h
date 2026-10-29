#ifndef QDL_FSO_RYTOV_MODEL_H
#define QDL_FSO_RYTOV_MODEL_H

#include "ns3/object.h"
#include "ns3/qdl-fso-cn2-model.h"
#include "ns3/qdl-fso-numerical-integration.h"
#include "ns3/type-id.h"

namespace ns3
{

struct QdlFsoPropagationContext;

/**
 * @ingroup qdl-fso
 *
 * @brief Computes the Rytov variance σ_R^2 to quantify atmospheric turbulence strength for a link, for the qdl-fso
 * module [Morellato2027].
 *
 * The Rytov variance σ_R^2 is a dimensionless parameter that measures the strength of optical turbulence in the
 * atmosphere, and it serves as a value input across the qdl-fso module.
 * Specifically, it is required to derive:
 *  - Long-term spot size W_LT and short-term beam footprint W_ST.
 *  - Beam wandering variance <r_c^2>.
 *  - Gamma-Gamma scintillation distribution parameters (α, β) via large-scale σ_x^2 and small-scale σ_y^2
 * variances [Andrews2005].
 *  - Quantum dephasing noise probability p_deph = erf(σ_R^2) [Chehimi2025].
 *
 * The formulation for σ_R^2 depends on the link geometry and wave propagation model [Andrews2005]:
 *  - Horizontal links: Plane-wave expression.
 *  - Downlink slant paths: Plane-wave expression accounting for zenith angle ζ and altitude-dependent C_n^2(h).
 *  - Uplink slant paths: Spherical-wave expression accounting for zenith angle ζ and altitude-dependent C_n^2(h).
 *
 * @see [Morellato2027] B. Morellato, R. Di Bella et al., "An FSO Channel Model for ns-3"
 * @see [Andrews2005] L. C. Andrews and R. L. Phillips, "Laser Beam Propagation through Random Media"
 * @see [Chehimi2025] M. Chehimi et al., "Reconfigurable Intelligent Surface (RIS)-Assisted Entanglement Distribution in
 * FSO Quantum Networks"
 */
class QdlFsoRytovModel : public Object
{
  public:
	/**
	 * @brief Get the type ID.
	 * @return the object TypeId
	 */
	static TypeId GetTypeId();

	/**
	 * @brief QdlFsoRytovModel constructor
	 */
	QdlFsoRytovModel();

	/**
	 * @brief ~QdlFsoRytovModel destructor
	 */
	~QdlFsoRytovModel() override;

	/**
	 * @brief Computes the Rytov variance σ_R^2 for the given propagation context.
	 *
	 * Automatically routes the calculation to either plane-wave (horizontal/downlink) or spherical-wave (uplink)
	 * expressions based on the trajectory and direction specified in the context.
	 *
	 * @param context Channel/link geometry and physical parameters.
	 * @return The dimensionless Rytov variance σ_R^2.
	 */
	double GetRytovVariance(const QdlFsoPropagationContext& context);

	/**
	 * @brief Computes the Rytov variance σ_R^2 specifically for horizontal paths.
	 *
	 * Uses the plane-wave formula σ_R^2 = 1.23 C_n^2 k^{7/6} L^{11/6}, assuming constant C_n^2 along the horizontal
	 * propagation path.
	 *
	 * @param context Channel/link geometry and physical parameters.
	 * @return The dimensionless Rytov variance σ_R^2 for horizontal links.
	 *
	 * @see [Andrews2005] L. C. Andrews and R. L. Phillips, "Laser Beam Propagation through Random Media"
	 */
	double GetRytovVarianceHorizontal(const QdlFsoPropagationContext& context);

	/**
	 * @brief Computes the Rytov variance σ_R^2 for slant paths (uplink and downlink).
	 *
	 * Integrates the refractive-index structure parameter C_n^2(h) along the path from ground altitude h_0 to
	 * space/airborne altitude H. Adopts a plane-wave model for downlink and a spherical-wave model for uplink
	 * [Andrews2005].
	 *
	 * @param context Channel/link geometry and physical parameters.
	 * @return The dimensionless Rytov variance σ_R^2 for slant paths.
	 *
	 * @see [Andrews2005] L. C. Andrews and R. L. Phillips, "Laser Beam Propagation through Random Media"
	 */
	double GetRytovVarianceUplinkDownlink(const QdlFsoPropagationContext& context);

	/**
	 * @brief Get the refractive-index structure parameter (C_n^2) model.
	 * @return A pointer to the underlying QdlFsoCn2Model.
	 */
	Ptr<QdlFsoCn2Model> GetCn2Model() const;

	/**
	 * @brief Set the refractive-index structure parameter (C_n^2) model.
	 *
	 * The C_n^2 profile is required to calculate the path integral for Rytov variance [Morellato2027].
	 *
	 * @param model A pointer to a QdlFsoCn2Model.
	 */
	void SetCn2Model(Ptr<QdlFsoCn2Model> model);

  protected:
	// inherited from Object
	void DoDispose() override;

  private:
	Ptr<QdlFsoCn2Model> m_cn2Model; //!< Model for the refractive-index structure parameter C_n^2(h)
};

} // namespace ns3

#endif // QDL_FSO_RYTOV_MODEL_H
