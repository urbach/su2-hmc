// Copyright © 2016 Martin Ueding <dev@martin-ueding.de>


#include "hybrid-monte-carlo.hpp"

#include "pauli-matrices.hpp"
#include "sanity-checks.hpp"

#include <unsupported/Eigen/MatrixFunctions>

#include <cassert>
#include <iostream>
#include <random>

Eigen::Matrix2cd generate_from_gaussian(std::mt19937 &engine,
                                        std::normal_distribution<double> &dist) {
    std::vector<double> coefficients = {dist(engine), dist(engine), dist(engine)};
    auto const pauli_matrices = PauliMatrices::get_instance();
    Eigen::Matrix2cd algebra_element;
    for (int i = 0; i < 3; ++i) {
        Eigen::Matrix2cd scaled_generator = coefficients[i] * pauli_matrices.get(i);
        algebra_element += scaled_generator;
    }


    return algebra_element;
}

Configuration make_hot_start(int const length_space,
                             int const length_time,
                             double const std,
                             int const seed) {
    std::mt19937 engine(seed);
    std::normal_distribution<double> dist(0, std);

    Configuration links(length_space, length_time);
    randomize_group(links, engine, dist);
    return links;
}

void randomize_algebra(Configuration &links,
                       std::mt19937 &engine,
                       std::normal_distribution<double> &dist) {
    for (int n1 = 0; n1 < links.length_time; ++n1) {
        for (int n2 = 0; n2 < links.length_space; ++n2) {
            for (int n3 = 0; n3 < links.length_space; ++n3) {
                for (int n4 = 0; n4 < links.length_space; ++n4) {
                    for (int mu = 0; mu < 4; ++mu) {
                        auto const next = generate_from_gaussian(engine, dist);
                        assert(is_hermitian(next));
                        links(n1, n2, n3, n4, mu) = next;
                    }
                }
            }
        }
    }
}

void randomize_group(Configuration &links,
                     std::mt19937 &engine,
                     std::normal_distribution<double> &dist) {
    for (int n1 = 0; n1 < links.length_time; ++n1) {
        for (int n2 = 0; n2 < links.length_space; ++n2) {
            for (int n3 = 0; n3 < links.length_space; ++n3) {
                for (int n4 = 0; n4 < links.length_space; ++n4) {
                    for (int mu = 0; mu < 4; ++mu) {
                        auto const exponent = std::complex<double>{0, 1} *
                                              generate_from_gaussian(engine, dist);
                        auto const next = exponent.exp();
                        assert(is_unitary(next);
                        links(n1, n2, n3, n4, mu) = next;
                    }
                }
            }
        }
    }
}

void md_step(Configuration &links,
             Configuration &momenta,
             Configuration &momenta_half,
             std::mt19937 &engine,
             std::normal_distribution<double> &dist,
             double const time_step,
             double const beta) {
// Update `momenta_half`.
#pragma omp parallel for
    for (int n1 = 0; n1 < links.length_time; ++n1) {
        for (int n2 = 0; n2 < links.length_space; ++n2) {
            for (int n3 = 0; n3 < links.length_space; ++n3) {
                for (int n4 = 0; n4 < links.length_space; ++n4) {
                    for (int mu = 0; mu < 4; ++mu) {
                        momenta_half(n1, n2, n3, n4, mu) = compute_new_momentum(
                            n1, n2, n3, n4, mu, links, momenta, time_step, beta);
                    }
                }
            }
        }
    }
// Update `links`.
#pragma omp parallel for
    for (int n1 = 0; n1 < links.length_time; ++n1) {
        for (int n2 = 0; n2 < links.length_space; ++n2) {
            for (int n3 = 0; n3 < links.length_space; ++n3) {
                for (int n4 = 0; n4 < links.length_space; ++n4) {
                    for (int mu = 0; mu < 4; ++mu) {
                        links(n1, n2, n3, n4, mu) = compute_new_link(
                            n1, n2, n3, n4, mu, links, momenta_half, time_step);
                    }
                }
            }
        }
    }
// Update `momenta`.
#pragma omp parallel for
    for (int n1 = 0; n1 < links.length_time; ++n1) {
        for (int n2 = 0; n2 < links.length_space; ++n2) {
            for (int n3 = 0; n3 < links.length_space; ++n3) {
                for (int n4 = 0; n4 < links.length_space; ++n4) {
                    for (int mu = 0; mu < 4; ++mu) {
                        momenta(n1, n2, n3, n4, mu) = compute_new_momentum(
                            n1, n2, n3, n4, mu, links, momenta_half, time_step, beta);
                    }
                }
            }
        }
    }
}

Eigen::Matrix2cd compute_new_momentum(int const n1,
                                      int const n2,
                                      int const n3,
                                      int const n4,
                                      int const mu,
                                      Configuration const &links,
                                      Configuration const &momenta,
                                      double const time_step,
                                      double const beta) {
    // Copy old momentum.
    Eigen::Matrix2cd result = momenta(n1, n2, n3, n4, mu);
    result +=
        time_step / 2 * compute_momentum_derivative(n1, n2, n3, n4, mu, links, beta);
    return result;
}

Eigen::Matrix2cd get_staples(int const n1,
                             int const n2,
                             int const n3,
                             int const n4,
                             int const mu,
                             Configuration const &links) {
    Eigen::Matrix2cd staples;
    // XXX Perhaps use std::array here.
    std::vector<int> const old_coords{n1, n2, n3, n4};
    for (int nu = 0; nu < 4; ++nu) {
        if (nu == mu) {
            continue;
        }
        auto coords = old_coords;

        auto &link3 = links(coords, nu);
        ++coords[mu];
        auto &link1 = links(coords, nu);
        --coords[mu];
        ++coords[nu];
        auto &link2 = links(coords, mu);

        staples += link1 * link2.adjoint() * link3.adjoint();

        coords = old_coords;

        --coords[nu];
        auto &link6 = links(coords, nu);
        auto &link5 = links(coords, mu);
        ++coords[mu];
        auto &link4 = links(coords, nu);

        staples += link4.adjoint() * link5.adjoint() * link6;
    }
    return staples;
}

Eigen::Matrix2cd compute_momentum_derivative(int const n1,
                                             int const n2,
                                             int const n3,
                                             int const n4,
                                             int const mu,
                                             Configuration const &links,
                                             double const beta) {
    auto staples = get_staples(n1, n2, n3, n4, mu, links);
    Eigen::Matrix2cd result = links(n1, n2, n3, n4, mu) * staples;
    result -= result.adjoint().eval();

    return std::complex<double>{0, 1} * beta / 6.0 * result;
}

Eigen::Matrix2cd compute_new_link(int const n1,
                                  int const n2,
                                  int const n3,
                                  int const n4,
                                  int const mu,
                                  Configuration const &links,
                                  Configuration const &momenta_half,
                                  double const time_step) {
    auto const exponent =
        std::complex<double>{0, 1} * time_step * momenta_half(n1, n2, n3, n4, mu);
    auto const rotation = exponent.exp();
    return rotation * links(n1, n2, n3, n4, mu);
}

Eigen::Matrix2cd get_plaquette(int const n1,
                               int const n2,
                               int const n3,
                               int const n4,
                               int const mu,
                               int const nu,
                               Configuration const &links) {
    std::vector<int> coords{n1, n2, n3, n4};

    auto &link1 = links(coords, mu);
    auto &link4 = links(coords, nu);
    ++coords[mu];
    auto &link2 = links(coords, nu);
    --coords[mu];
    ++coords[nu];
    auto &link3 = links(coords, mu);

    return link1 * link2 * link3.adjoint() * link4.adjoint();
}

double get_plaquette_trace_real(Configuration const &links) {
    double sum = 0.0;

#pragma omp parallel for reduction(+ : sum)
    for (int n1 = 0; n1 < links.length_time; ++n1) {
        for (int n2 = 0; n2 < links.length_space; ++n2) {
            for (int n3 = 0; n3 < links.length_space; ++n3) {
                for (int n4 = 0; n4 < links.length_space; ++n4) {
                    for (int mu = 0; mu < 4; ++mu) {
                        for (int nu = 0; nu < 4; ++nu) {
                            auto const plaquette =
                                get_plaquette(n1, n2, n3, n4, mu, nu, links);
                            double const summand = plaquette.trace().real();
                            assert(std::isfinite(summand));
                            sum += summand;
                        }
                    }
                }
            }
        }
    }

    return sum;
}

double get_energy(Configuration const &links, Configuration const &momenta) {
    double links_part = 0.0;
    links_part += links.get_volume() * 4 * 4;
    links_part -= get_plaquette_trace_real(links);

    double momentum_part = 0.0;
    double momentum_part_imag = 0.0;
#pragma omp parallel for reduction(+ : momentum_part, momentum_part_imag)
    for (int n1 = 0; n1 < links.length_time; ++n1) {
        for (int n2 = 0; n2 < links.length_space; ++n2) {
            for (int n3 = 0; n3 < links.length_space; ++n3) {
                for (int n4 = 0; n4 < links.length_space; ++n4) {
                    for (int mu = 0; mu < 4; ++mu) {
                        auto const &momentum = momenta(n1, n2, n3, n4, mu);
                        auto const trace = (momentum * momentum).trace();
                        auto const summand = trace.real();
                        assert(std::isfinite(summand));
                        momentum_part += summand;
                        momentum_part_imag += trace.imag();
                        // TODO Check that trace.imag() really is zero.
                    }
                }
            }
        }
    }

    std::cout << "Momentum: Re = " << momentum_part << "\t Im = " << momentum_part_imag << std::endl;

    // TODO Include $g_\text s$ and $\beta$ here?
    return links_part + 0.5 * momentum_part;
}
