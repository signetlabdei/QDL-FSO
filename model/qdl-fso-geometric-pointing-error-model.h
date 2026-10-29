#ifndef QDL_FSO_POINTING_ERROR_MODEL_H
#define QDL_FSO_POINTING_ERROR_MODEL_H

#include "ns3/object.h"
#include "ns3/pointer.h"
#include "ns3/qdl-fso-cn2-model.h"
#include "ns3/random-variable-stream.h"
#include "ns3/type-id.h"

namespace ns3
{

struct QdlFsoPropagationContext;

/**
 * @ingroup qdl-fso
 *
 * @brief Computes pointing efficiency and geometric losses due to beam divergence, wander, and jitter for the qdl-fso
 * module [Morellato2027].
 *
 * This class models the combined attenuation coefficient η_gpe, which accounts for:
 *  - Geometric beam divergence and short-term turbulence beam spreading (short-term spot size W_ST).
 *  - Pointing errors resulting from mechanical jitter/imperfections and turbulence-induced beam wander.
 *  - Tracking loop correction efficiency η_tr.
 *
 * Per-axis beam displacements are sampled as independent Gaussian random variables x ~ N(μ_x, σ_pe,x) and
 * y ~ N(μ_y, σ_pe,y), with per-axis variance: σ_pe,x = σ_pe,y = [︃(L*θ_pe)^2 + <r_c^2>/2]︃*(1 − η_tr) [Karakosta2025]
 *
 * Depending on the relative size of the short-term beam footprint W_ST and receiver aperture radius D_rx/2,
 * two calculation regimes are used [Morellato2027]:
 *  1) Farid-Hranilovic closed-form approximation when W_ST > D_rx/2 [Farid2007].
 *  2) Miao et al. smoothed point-aperture approximation when W_ST <= D_rx/2 [Miao2023].
 *
 * @see [Morellato2027] B. Morellato, R. Di Bella et al., "An FSO Channel Model for ns-3"
 * @see [Karakosta2025] A. Karakosta-Amarantidou et al., "Free-space model for a balloon-based quantum network"
 * @see [Farid2007] A. A. Farid and S. Hranilovic, "Outage Capacity Optimization for Free-Space Optical Links With
 * Pointing Errors"
 * @see [Miao2023] L. Miao et al., "New Results for the Pointing Errors Model in Two Asymptotic Cases"
 */
class QdlFsoGeometricPointingErrorModel : public Object
{
  public:
	/**
	 * @brief Get the type ID.
	 * @return the object TypeId
	 */
	static TypeId GetTypeId();

	/**
	 * @brief QdlFsoGeometricPointingErrorModel constructor
	 */
	QdlFsoGeometricPointingErrorModel();

	/**
	 * @brief ~QdlFsoGeometricPointingErrorModel destructor
	 */
	~QdlFsoGeometricPointingErrorModel() override;

	/**
	 * @brief Computes the overall geometric and pointing efficiency η_gpe.
	 *
	 * Samples a radial displacement r based on pointing jitter, beam wander variance, and context parameters,
	 * computes the short-term spot size W_ST, and evaluates η_gpe using either
	 * the Farid-Hranilovic or Miao approximation based on whether W_ST > D_rx/2 [Morellato2027].
	 *
	 * @param context Channel/link geometry and physical parameters.
	 * @param rytov_variance Rytov variance of the channel.
	 * @param wander_variance Turbulence-induced beam wandering variance <r_c^2>.
	 * @return Dimensionless geometric and pointing efficiency η_gpe in [0, 1].
	 *
	 * @see [Morellato2027] B. Morellato, R. Di Bella et al., "An FSO Channel Model for ns-3"
	 */
	double GetPointingEfficiency(const QdlFsoPropagationContext& context,
								 double rytov_variance,
								 double wander_variance);

	/**
	 * @brief Computes pointing efficiency using the Farid-Hranilovic approximation [Farid2007].
	 *
	 * Used in the regime where the short-term beam footprint exceeds the receiver aperture radius
	 * (W_ST > D_rx/2).
	 *
	 * @param D_rx Receiver aperture diameter in meters.
	 * @param r Radial displacement offset at the receiver plane in meters.
	 * @param W_ST Short-term beam waist radius in meters.
	 * @return Pointing efficiency η_gpe.
	 *
	 * @see [Farid2007] A. A. Farid and S. Hranilovic, "Outage Capacity Optimization for Free-Space Optical Links With
	 * Pointing Errors"
	 */
	double GetPointingEfficiencyFarid(double D_rx, double r, double W_ST);

	/**
	 * @brief Computes pointing efficiency using the Miao et al. point-aperture approximation [Miao2023].
	 *
	 * Used in the regime where the short-term beam footprint is smaller than or equal to the receiver
	 * aperture radius (W_ST <= D_rx/2).
	 *
	 * @param D_rx Receiver aperture diameter in meters.
	 * @param r Radial displacement offset at the receiver plane in meters.
	 * @param W_ST Short-term beam waist radius in meters.
	 * @param mode Evaluation variant/mode string (e.g., "smoothed").
	 * @return Pointing efficiency η_gpe.
	 *
	 * @see [Miao2023] L. Miao et al., "New Results for the Pointing Errors Model in Two Asymptotic Cases"
	 */
	double GetPointingEfficiencyMiao(double D_rx, double r, double W_ST, std::string mode);

	/**
	 * @brief Samples the stochastic radial displacement r at the receiver plane.
	 *
	 * Samples per-axis displacements x ~ N(μ_x, σ_pe,x) and y ~ N(μ_y, σ_pe,y), combining angular pointing jitter
	 * θ_pe, beam wander variance <r_c^2>, and tracking efficiency η_tr.
	 *
	 * @param context Channel/link geometry and physical parameters.
	 * @param wander_variance Beam wander variance <r_c^2> in m^2.
	 * @return Radial displacement r = sqrt(x^2 + y^2) in meters.
	 */
	double SampleRadialDisplacement(const QdlFsoPropagationContext& context, double wander_variance);

	/**
	 * @brief Get the refractive-index structure parameter (C_n^2) model.
	 * @return A pointer to the underlying QdlFsoCn2Model.
	 */
	Ptr<QdlFsoCn2Model> GetCn2Model() const;

	/**
	 * @brief Set the refractive-index structure parameter (C_n^2) model.
	 * @param model A pointer to a QdlFsoCn2Model.
	 */
	void SetCn2Model(Ptr<QdlFsoCn2Model> model);

	/**
	 * \brief Assigns a fixed random stream index to underlying random variables.
	 * \param stream First stream index to allocate.
	 * \return Number of streams allocated (2: one for X-axis, one for Y-axis).
	 */
	int64_t AssignStreams(int64_t stream);

  protected:
	// inherited from Object
	void DoDispose() override;

  private:
	Ptr<QdlFsoCn2Model> m_cn2Model;		   //!< Model for the refractive-index structure parameter C_n^2(h)
	Ptr<NormalRandomVariable> m_normalRvX; //!< Gaussian RV generator for X-axis offset
	Ptr<NormalRandomVariable> m_normalRvY; //!< Gaussian RV generator for Y-axis offset
};

} // namespace ns3

#endif // QDL_FSO_POINTING_ERROR_MODEL_H
