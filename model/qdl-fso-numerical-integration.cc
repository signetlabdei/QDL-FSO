#include "qdl-fso-numerical-integration.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace ns3
{
namespace qdlfso
{

namespace
{
/// Convergence tolerance for the Newton-Raphson Legendre root solver.
constexpr double kNewtonTolerance = 1e-14;
/// Maximum number of Newton-Raphson iterations per root.
constexpr uint32_t kMaxNewtonIterations = 100;
constexpr double kPi = 3.14159265358979323846;

/**
 * \brief Compute the Gauss-Legendre quadrature nodes and weights on
 *        the reference interval [-1, 1] for a given order, using the
 *        Newton-Raphson root-finding method applied to the Legendre
 *        polynomial recurrence.
 *
 * \param numPoints requested number of quadrature points (order),
 *        values < 1 are treated as 1
 * \param nodes [out] the computed quadrature nodes in [-1, 1]
 * \param weights [out] the computed quadrature weights
 */
void
GetGaussLegendreNodesAndWeights(uint32_t numPoints, std::vector<double>& nodes, std::vector<double>& weights)
{
	if (numPoints == 0)
	{
		numPoints = 1;
	}

	nodes.assign(numPoints, 0.0);
	weights.assign(numPoints, 0.0);

	const int32_t n = static_cast<int32_t>(numPoints);
	const int32_t numRoots = (n + 1) / 2; // exploit symmetry about x = 0

	for (int32_t i = 0; i < numRoots; ++i)
	{
		// Initial guess for the i-th root (Chebyshev/asymptotic approximation).
		double x = std::cos(kPi * (static_cast<double>(i) + 0.75) / (static_cast<double>(n) + 0.5));

		double pDeriv = 0.0;
		for (uint32_t iter = 0; iter < kMaxNewtonIterations; ++iter)
		{
			// Evaluate the Legendre polynomial P_n(x) and its derivative
			// via the standard three-term recurrence.
			double pPrev = 1.0; // P_0(x)
			double pCurr = x;	// P_1(x)

			for (int32_t k = 2; k <= n; ++k)
			{
				const double pNext = ((2.0 * k - 1.0) * x * pCurr - (k - 1.0) * pPrev) / static_cast<double>(k);
				pPrev = pCurr;
				pCurr = pNext;
			}

			// Derivative of P_n at x, from P_n and P_{n-1}.
			pDeriv = static_cast<double>(n) * (x * pCurr - pPrev) / (x * x - 1.0);

			const double dx = pCurr / pDeriv;
			x -= dx;

			if (std::fabs(dx) < kNewtonTolerance)
			{
				break;
			}
		}

		const double weight = 2.0 / ((1.0 - x * x) * pDeriv * pDeriv);

		nodes[i] = -x;
		nodes[n - 1 - i] = x;
		weights[i] = weight;
		weights[n - 1 - i] = weight;
	}
}
} // namespace

double
GaussLegendreCompositeIntegrate(const std::function<double(double)>& func,
								double a,
								double b,
								uint32_t numPanels,
								uint32_t pointsPerPanel)
{
	if (numPanels == 0)
	{
		numPanels = 1;
	}

	if (a == b)
	{
		return 0.0;
	}

	bool swapped = false;
	if (a > b)
	{
		std::swap(a, b);
		swapped = true;
	}

	// Precompute the reference nodes/weights once and reuse them across
	// all panels, mapping into each panel's sub-interval.
	std::vector<double> nodes;
	std::vector<double> weights;
	GetGaussLegendreNodesAndWeights(pointsPerPanel, nodes, weights);

	const double panelWidth = (b - a) / static_cast<double>(numPanels);
	double total = 0.0;

	for (uint32_t panel = 0; panel < numPanels; ++panel)
	{
		const double panelA = a + static_cast<double>(panel) * panelWidth;
		const double panelB = panelA + panelWidth;

		const double halfWidth = 0.5 * (panelB - panelA);
		const double midpoint = 0.5 * (panelB + panelA);

		double panelSum = 0.0;
		for (std::size_t i = 0; i < nodes.size(); ++i)
		{
			const double x = halfWidth * nodes[i] + midpoint;
			panelSum += weights[i] * func(x);
		}

		total += halfWidth * panelSum;
	}

	if (swapped)
	{
		total = -total;
	}

	return total;
}

} // namespace qdlfso
} // namespace ns3
