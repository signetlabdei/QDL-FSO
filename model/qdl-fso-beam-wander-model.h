#ifndef QDL_FSO_BEAM_WANDER_MODEL_H
#define QDL_FSO_BEAM_WANDER_MODEL_H

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
 * @brief Computes the turbulence-induced beam wandering variance for the qdl-fso module [Morellato2027].
 *
 * Beam wander makes the transmitted Gaussian spot move randomly at the receiver plane, which
 * causes the beam to cover a larger circular area over time and contributes significantly
 * to pointing errors[Andrews2005]. This class computes the variance of this displacement,
 * denoted as <r_c^2>.
 *
 * The wander variance calculation relies heavily on the refractive-index structure parameter
 * profile, C_n^2(h), and the Rytov variance.
 * The model provides specific integral evaluations for horizontal paths and slanted (uplink/downlink) paths.
 *
 * @see [Morellato2027] B. Morellato, R. Di Bella et al., "An FSO Channel Model for ns-3"
 * @see [Andrews2005] L. C. Andrews and R. L. Phillips, "Laser Beam Propagation through Random Media"
 */
class QdlFsoBeamWanderModel : public Object
{
  public:
	/**
	 * @brief Get the type ID.
	 * @return the object TypeId
	 */
	static TypeId GetTypeId();

	/**
	 * @brief QdlFsoBeamWanderModel constructor
	 */
	QdlFsoBeamWanderModel();

	/**
	 * @brief ~QdlFsoBeamWanderModel destructor
	 */
	~QdlFsoBeamWanderModel() override;

	/**
	 * @brief Computes the turbulence-induced beam wandering variance <r_c^2>.
	 *
	 * Automatically routes the calculation to either the horizontal or uplink/downlink
	 * expressions based on the link geometry.
	 *
	 * @param context Channel/link geometry and physical parameters.
	 * @param rytov_variance The Rytov variance of the channel.
	 * @return The beam wander variance <r_c^2>.
	 */
	double GetWanderVariance(const QdlFsoPropagationContext& context, double rytov_variance);

	/**
	 * @brief Computes the beam wandering variance specifically for horizontal paths.
	 *
	 * Uses a simplified integral expression for horizontal links [Andrews2005].
	 *
	 * @param context Channel/link geometry and physical parameters.
	 * @param rytov_variance The Rytov variance of the channel.
	 * @return The beam wander variance <r_c^2>.
	 *
	 * @see [Andrews2005] L. C. Andrews and R. L. Phillips, "Laser Beam Propagation through Random Media"
	 */
	double GetWanderVarianceHorizontal(const QdlFsoPropagationContext& context, double rytov_variance);

	/**
	 * @brief Computes the beam wandering variance for slant paths (uplink and downlink).
	 *
	 * Evaluates the path integral accounting for the altitude-dependent C_n^2(h) profile
	 * and the direction coefficient (positive for downlink, negative for uplink) [Morellato2027].
	 *
	 * @param context Channel/link geometry and physical parameters.
	 * @param rytov_variance The Rytov variance of the channel.
	 * @return The beam wander variance <r_c^2>.
	 *
	 * @see [Morellato2027] B. Morellato, R. Di Bella et al., "An FSO Channel Model for ns-3"
	 */
	double GetWanderVarianceUplinkDownlink(const QdlFsoPropagationContext& context, double rytov_variance);

	/**
	 * @brief Get the refractive-index structure parameter (C_n^2) model.
	 * @return A pointer to the underlying QdlFsoCn2Model.
	 */
	Ptr<QdlFsoCn2Model> GetCn2Model() const;

	/**
	 * @brief Set the refractive-index structure parameter (C_n^2) model.
	 *
	 * The C_n^2 profile is required to calculate the beam wandering variance.
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

#endif // QDL_FSO_BEAM_WANDER_MODEL_H
