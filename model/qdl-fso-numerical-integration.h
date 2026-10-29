#ifndef QDL_FSO_NUMERICAL_INTEGRATION_H
#define QDL_FSO_NUMERICAL_INTEGRATION_H

#include <cstdint>
#include <functional>

namespace ns3
{
namespace qdlfso
{

/**
 * @ingroup qdl-fso
 *
 * @brief Integrate a function over [a, b] using composite
 *        Gauss-Legendre quadrature, subdividing [a, b] into panels
 *        for improved accuracy over wide or oscillatory intervals.
 *
 * Increase numPanels and/or pointsPerPanel to improve accuracy.
 *
 * @param func the integrand function f(x)
 * @param a lower bound of integration
 * @param b upper bound of integration
 * @param numPanels number of sub-intervals (panels) to split [a, b] into
 * @param pointsPerPanel number of Gauss-Legendre points used per panel
 * @return the approximate value of \f$ \int_a^b f(x)\,dx \f$
 */
double GaussLegendreCompositeIntegrate(const std::function<double(double)>& func,
									   double a,
									   double b,
									   uint32_t numPanels = 10,
									   uint32_t pointsPerPanel = 5);

} // namespace qdlfso
} // namespace ns3

#endif // QDL_FSO_NUMERICAL_INTEGRATION_H
