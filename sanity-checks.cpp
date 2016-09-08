// Copyright © 2016 Martin Ueding <dev@martin-ueding.de>

#include "sanity-checks.hpp"

#include <cmath>

double const tolerance = 1e-5;

bool is_zero(Eigen::Matrix2cd const &mat) {
    for (int row = 0; row < 2; ++row) {
        for (int col = 0; col < 2; ++col) {
            if (std::abs(mat(row, col).real()) > tolerance)
                return false;
            if (std::abs(mat(row, col).imag()) > tolerance)
                return false;
        }
    }
    return true;
}

bool is_equal(Eigen::Matrix2cd const &mat1, Eigen::Matrix2cd const &mat2) {
    return is_zero(mat1 - mat2);
}

bool is_hermitian(Eigen::Matrix2cd const &mat) {
    return is_zero(mat - mat.adjoint().eval());
}

bool is_unity(Eigen::Matrix2cd const &mat) {
    Eigen::Matrix2cd copy = mat;
    copy(0, 0) -= std::complex<double>{1, 0};
    copy(1, 1) -= std::complex<double>{1, 0};
    return is_zero(copy);
}

bool is_unitary(Eigen::Matrix2cd const &mat) {
    return is_unity(mat * mat.adjoint().eval());
}
