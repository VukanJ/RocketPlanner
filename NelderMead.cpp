#include "NelderMead.h"

#include <algorithm>
#include <cmath>

static double computeCentroid(const std::vector<std::vector<double>>& simplex,
                              std::vector<double>& centroid, int exclude) {
    int n = centroid.size();
    std::fill(centroid.begin(), centroid.end(), 0.0);
    for (int i = 0; i < (int)simplex.size(); ++i) {
        if (i == exclude) continue;
        for (int j = 0; j < n; ++j) {
            centroid[j] += simplex[i][j];
        }
    }
    double inv = 1.0 / (simplex.size() - 1);
    for (int j = 0; j < n; ++j) {
        centroid[j] *= inv;
    }
    return inv;
}

std::vector<double> minimize(
    const std::function<double(const std::vector<double>&)>& f,
    const std::vector<double>& x0,
    const NelderMeadParams& params)
{
    int n = x0.size();
    if (n == 0) return {};
    if (n == 1) {
        // Golden-section search with bracket expansion
        const double phi = 0.6180339887498949;
        double a = x0[0] - params.initialStep;
        double b = x0[0] + params.initialStep;
        double xm = x0[0];
        double fm = f({xm});

        // Expand bracket until x0 is lower than both endpoints
        for (int i = 0; i < 20; ++i) {
            double fa = f({a});
            double fb = f({b});
            if (fm <= fa && fm <= fb) break;
            double span = b - a;
            a -= 0.5 * span;
            b += 0.5 * span;
        }

        double c = b - phi * (b - a);
        double d = a + phi * (b - a);
        double fc = f({c});
        double fd = f({d});

        for (int iter = 0; iter < params.maxIter; ++iter) {
            double fScale = std::max(1.0, std::min(std::abs(fc), std::abs(fd)));
            if (std::abs(b - a) < params.tol * fScale) break;

            if (fc < fd) {
                b = d; d = c;
                fd = fc;
                c = b - phi * (b - a);
                fc = f({c});
            } else {
                a = c; c = d;
                fc = fd;
                d = a + phi * (b - a);
                fd = f({d});
            }
        }
        return {(a + b) / 2.0};
    }

    // Downhill simplex method for n > 1

    // Build initial simplex
    std::vector<std::vector<double>> simplex(n + 1, x0);
    for (int i = 0; i < n; ++i) {
        simplex[i + 1][i] += params.initialStep;
    }

    std::vector<double> fx(n + 1);
    for (int i = 0; i <= n; ++i) {
        fx[i] = f(simplex[i]);
    }

    // If every vertex is non-finite (e.g. all INFINITY), the optimizer
    // cannot make progress — NaN arithmetic would silently run all iterations.
    bool allNonFinite = true;
    for (int i = 0; i <= n; ++i) {
        if (std::isfinite(fx[i])) { allNonFinite = false; break; }
    }
    if (allNonFinite) return simplex[0];

    std::vector<double> centroid(n);
    std::vector<double> reflected(n);
    std::vector<double> expanded(n);
    std::vector<double> contracted(n);

    double prevBest = fx[0];
    for (int iter = 0; iter < params.maxIter; ++iter) {
        // Order by function value (lowest = best)
        for (int i = 0; i < n; ++i) {
            for (int j = i + 1; j <= n; ++j) {
                if (fx[j] < fx[i]) {
                    std::swap(fx[i], fx[j]);
                    std::swap(simplex[i], simplex[j]);
                }
            }
        }

        // Check convergence: range of function values
        double fRange = std::abs(fx[n] - fx[0]);
        double fScale = std::max(1.0, std::abs(fx[0]));
        if (!std::isfinite(fRange) || fRange < params.tol * fScale) {
            break;
        }

        // Early termination: relative change in best value below threshold
        if (params.bestValueTol > 0.0 && iter > 0) {
            double reldiff = std::abs(fx[0] - prevBest) / std::max(1.0, std::abs(fx[0]));
            if (!std::isfinite(reldiff) || reldiff < params.bestValueTol) break;
        }
        prevBest = fx[0];

        // Centroid of all but worst
        computeCentroid(simplex, centroid, n);

        // Reflection
        for (int j = 0; j < n; ++j) {
            reflected[j] = centroid[j] + params.alpha * (centroid[j] - simplex[n][j]);
        }
        double fReflected = f(reflected);

        if (fReflected < fx[0]) {
            // Best so far — try expansion
            for (int j = 0; j < n; ++j) {
                expanded[j] = centroid[j] + params.gamma * (reflected[j] - centroid[j]);
            }
            double fExpanded = f(expanded);
            if (fExpanded < fReflected) {
                simplex[n] = expanded;
                fx[n] = fExpanded;
            } else {
                simplex[n] = reflected;
                fx[n] = fReflected;
            }
        } else if (fReflected < fx[n - 1]) {
            // Better than second-worst — accept reflection
            simplex[n] = reflected;
            fx[n] = fReflected;
        } else {
            // Contraction
            if (fReflected < fx[n]) {
                // Outside contraction
                for (int j = 0; j < n; ++j) {
                    contracted[j] = centroid[j] + params.rho * (reflected[j] - centroid[j]);
                }
                double fContracted = f(contracted);
                if (fContracted < fReflected) {
                    simplex[n] = contracted;
                    fx[n] = fContracted;
                } else {
                    // Shrink
                    for (int i = 1; i <= n; ++i) {
                        for (int j = 0; j < n; ++j) {
                            simplex[i][j] = simplex[0][j] + params.sigma * (simplex[i][j] - simplex[0][j]);
                        }
                        fx[i] = f(simplex[i]);
                    }
                }
            } else {
                // Inside contraction
                for (int j = 0; j < n; ++j) {
                    contracted[j] = centroid[j] - params.rho * (centroid[j] - simplex[n][j]);
                }
                double fContracted = f(contracted);
                if (fContracted < fx[n]) {
                    simplex[n] = contracted;
                    fx[n] = fContracted;
                } else {
                    // Shrink
                    for (int i = 1; i <= n; ++i) {
                        for (int j = 0; j < n; ++j) {
                            simplex[i][j] = simplex[0][j] + params.sigma * (simplex[i][j] - simplex[0][j]);
                        }
                        fx[i] = f(simplex[i]);
                    }
                }
            }
        }
    }

    // Best vertex is at index 0 after the final ordering (re-order once more)
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j <= n; ++j) {
            if (fx[j] < fx[i]) {
                std::swap(fx[i], fx[j]);
                std::swap(simplex[i], simplex[j]);
            }
        }
    }
    return simplex[0];
}
