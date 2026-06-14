#ifndef NELDER_MEAD_H
#define NELDER_MEAD_H

#include <vector>
#include <functional>

struct NelderMeadParams {
    double alpha = 1.0;   // reflection coefficient
    double gamma = 2.0;   // expansion coefficient
    double rho = 0.5;     // contraction coefficient
    double sigma = 0.5;   // shrink coefficient
    int maxIter = 200;
    double tol = 1e-6;    // relative tolerance on function values
    double bestValueTol = 0.0; // early exit when relative change in best value < this (0 = disabled)
    double initialStep = 0.1; // step size for initial simplex construction
};

std::vector<double> minimize(
    const std::function<double(const std::vector<double>&)>& f,
    const std::vector<double>& x0,
    const NelderMeadParams& params = NelderMeadParams{});

#endif // NELDER_MEAD_H
