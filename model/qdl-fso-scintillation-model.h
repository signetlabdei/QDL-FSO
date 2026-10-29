#ifndef QDL_FSO_SCINTILLATION_MODEL_H
#define QDL_FSO_SCINTILLATION_MODEL_H

#include "ns3/object.h"
#include "ns3/pointer.h"
#include "ns3/random-variable-stream.h"
#include "ns3/type-id.h"

namespace ns3
{

struct QdlFsoPropagationContext;

/**
 * @ingroup qdl-fso
 *
 * @brief Abstract base class for atmospheric scintillation models in the qdl-fso module [Morellato2027].
 *
 * Scintillation models represent the stochastic irradiance fluctuations \f$ \eta_{scint} \f$ caused by turbulent eddies
 * in the optical propagation path [Andrews2005]. This class defines the general interface for sampling normalized
 * irradiance factor values \f$ (\mathbb{E}[\eta_{scint}] = 1)\f$.
 *
 * Subclasses implement specific statistical distributions (such as Gamma-Gamma) to generate random
 * irradiance samples based on link geometry and turbulence conditions.
 *
 * @see [Morellato2027] B. Morellato, R. Di Bella et al., "An FSO Channel Model for ns-3"
 * @see [Andrews2005] L. C. Andrews and R. L. Phillips, "Laser Beam Propagation through Random Media"
 */
class QdlFsoScintillationModel : public Object
{
  public:
	/**
	 * @brief Get the type ID.
	 * @return the object TypeId
	 */
	static TypeId GetTypeId();

	/**
	 * @brief QdlFsoScintillationModel constructor
	 */
	QdlFsoScintillationModel();

	/**
	 * @brief ~QdlFsoScintillationModel destructor
	 */
	~QdlFsoScintillationModel() override;

	/**
	 * @brief Sample the stochastic scintillation factor \f$\eta_{scint}\f$ for the given context.
	 *
	 * @param context Channel/link geometry and physical parameters.
	 * @param rytov_variance Rytov variance \f$ \sigma_R^2 \f$ of the channel.
	 * @return A random scintillation factor \f$ \eta_{scint} \f$ normalized such that \f$ \mathbb{E}[\eta_{scint}] =
	 * 1\f$.
	 */
	double GetIrradiance(const QdlFsoPropagationContext& context, double rytov_variance);

	/**
	 * @brief Assigns a fixed random stream index to underlying random variables.
	 * @param stream First stream index to allocate.
	 * @return Number of allocated streams.
	 */
	virtual int64_t AssignStreams(int64_t stream) = 0;

  protected:
	// inherited from Object
	void DoDispose() override;

  private:
	/**
	 * @brief Pure virtual method for subclasses to sample normalized irradiance.
	 *
	 * @param context Channel/link geometry and physical parameters.
	 * @param rytov_variance Rytov variance \f$ \sigma_R^2 \f$ of the channel.
	 * @return A random scintillation factor \f$ \eta_{scint} \f$.
	 */
	virtual double DoGetIrradiance(const QdlFsoPropagationContext& context, double rytov_variance) = 0;
};

/**
 * @ingroup qdl-fso
 *
 * @brief Gamma-Gamma scintillation model for optical irradiance fluctuations [Andrews2005].
 *
 * Models irradiance fluctuations as the product of two independent Gamma random variables:
 * - \f$ X \sim Gamma(\alpha, 1/\alpha) \f$, representing large-scale turbulent eddies.
 * - \f$ Y \sim Gamma(\beta, 1/\beta) \f$, representing small-scale turbulent eddies.
 *
 * The shape parameters \f$ \alpha = 1/\sigma_x^2 \f$ and \f$ \beta = 1/\sigma_y^2 \f$ are derived from the Rytov
 * variance \f$ \sigma_R^2 \f$ using plane-wave (horizontal/downlink) or spherical-wave (uplink) formulations.
 * When the receiver aperture diameter \f$ D_{rx} \f$ exceeds the spatial coherence radius \f$ \rho_c = \sqrt{L/k} \f$,
 * aperture averaging is applied to reduce scintillation variances.
 *
 * @see [Andrews2005] L. C. Andrews and R. L. Phillips, "Laser Beam Propagation through Random Media"
 */
class QdlFsoGammaGammaScintillationModel : public QdlFsoScintillationModel
{
  public:
	/**
	 * @brief Get the type ID.
	 * @return the object TypeId
	 */
	static TypeId GetTypeId();

	/**
	 * @brief QdlFsoGammaGammaScintillationModel constructor
	 */
	QdlFsoGammaGammaScintillationModel();

	/**
	 * @brief ~QdlFsoGammaGammaScintillationModel destructor
	 */
	~QdlFsoGammaGammaScintillationModel() override;

	/**
	 * @brief Computes the Gamma-Gamma shape parameters \f$ (\alpha, \beta) \f$ for a given channel context.
	 *
	 * Calculates large-scale \f$ \sigma_x^2 \f$ and small-scale \f$ \sigma_y^2 \f$ variances from \f$ \sigma_R^2\f$.
	 * Automatically applies aperture-averaged formulations if enabled and \f$ D_{rx} > \sqrt{L/k} \f$.
	 *
	 * @param context Channel/link geometry and physical parameters.
	 * @param rytov_variance Rytov variance \f$ \sigma_R^2 \f$ of the channel.
	 * @return A pair \f$ (\alpha, \beta) \f$ representing the shape parameters for the Gamma-Gamma distribution.
	 */
	std::pair<double, double> ComputeAlphaBeta(const QdlFsoPropagationContext& context, double rytov_variance);

	/**
	 * @brief Assigns a fixed random stream index to underlying random variables.
	 * @param stream First stream index to allocate.
	 * @return Number of streams allocated (2: one for large-scale X, one for small-scale Y).
	 */
	int64_t AssignStreams(int64_t stream) override;

	// ---- JUST FOR DEBUGGING ----
	/**
	 * @brief Sample irradiance given explicit shape parameters \f$ \alpha \f$ and \f$ \beta \f$.
	 * Used for testing and validation only.
	 *
	 * @param alpha Large-scale shape parameter.
	 * @param beta Small-scale shape parameter.
	 * @return Normalized irradiance sample \f$ I = X \cdot Y with \mathbb{E}[I] = 1 \f$.
	 */
	double GetIrradianceGivenAlphaBeta(double alpha, double beta);

	/**
	 * @brief Evaluates the theoretical Gamma-Gamma probability density function (PDF) at irradiance value I.
	 *
	 * Uses modified Bessel functions to compute the theoretical PDF \f$ f(I; \alpha, \beta) \f$.
	 *
	 * Used for testing and validation only.
	 *
	 * @param I Irradiance level.
	 * @param alpha Large-scale shape parameter.
	 * @param beta Small-scale shape parameter.
	 * @return Probability density f(I).
	 */
	double GetTheoreticalPdf(double I, double alpha, double beta);
	// ----------------------------

  protected:
	// inherited from Object
	void DoDispose() override;

  private:
	/**
	 * @brief Calculates normalized irradiance sample \f$ I \sim GammaGamma(\alpha, \beta) \f$ with \f$ \mathbb{E}[I] =
	 * 1\f$.
	 *
	 * @param context Channel/link geometry and physical parameters.
	 * @param rytov_variance Rytov variance \f$ \sigma_R^2 \f$ of the channel.
	 * @return Stochastic irradiance sample \f$ \eta_{scint} \f$.
	 */
	double DoGetIrradiance(const QdlFsoPropagationContext& context, double rytov_variance) override;

	Ptr<GammaRandomVariable>
		m_gammaRvLargeA; //!< Large-scale turbulence eddy RV \f$ (X \sim Gamma(\alpha, 1/\alpha)) \f$
	Ptr<GammaRandomVariable> m_gammaRvSmallB; //!< Small-scale turbulence eddy RV \f$ (Y \sim Gamma(\beta, 1/\beta)) \f$

	bool m_enableApertureAveraging; //!< Flag controlling whether aperture-averaging reduction is applied
};

} // namespace ns3

#endif // QDL_FSO_SCINTILLATION_MODEL_H
