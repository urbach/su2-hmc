// Copyright © 2016 Martin Ueding <dev@martin-ueding.de>

#include "sanity-checks.hpp"

#include <cmath>

double constexpr tolerance = 1e-10;

bool is_zero(Matrix const &mat) {
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

bool is_equal(Matrix const &mat1, Matrix const &mat2) {
    return is_zero(mat1 - mat2);
}

bool is_hermitian(Matrix const &mat) {
    return is_zero(mat - mat.adjoint().eval());
}

bool is_unity(Matrix const &mat) {
    Matrix copy = mat;
    copy(0, 0) -= std::complex<double>{1, 0};
    copy(1, 1) -= std::complex<double>{1, 0};
    return is_zero(copy);
}

bool is_unitary(Matrix const &mat) {
    return is_unity(mat * mat.adjoint().eval());
}

bool is_zero(double const &d) {
    return d < tolerance;
}

bool is_zero(std::complex<double> const &c) {
    return is_zero(c.real()) && is_zero(c.imag());
}

bool is_real(std::complex<double> const &c) {
    return is_zero(c.imag());
}

bool is_traceless(Matrix const &mat) {
    return is_zero(mat.trace());
}

bool is_equal(double const d1, double const d2) {
    return is_zero(d1 - d2);
}

bool is_unit_determinant(Matrix const &mat) {
    return is_zero(mat.determinant() - 1.0);
}
