/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010-2019, Georgia Tech Research Corporation,
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file    SOn.h
 * @brief   n*n matrix representation of SO(n), template on N, which can be
 * Eigen::Dynamic
 * @author  Frank Dellaert
 * @date    March 2019
 */

#pragma once

#include <gtsam/base/Lie.h>
#include <gtsam/base/Manifold.h>

#include <Eigen/Core>
#include <boost/random.hpp>

#include <iostream>
#include <string>

namespace gtsam {

namespace internal {
/// Calculate dimensionality of SO<N> manifold, or return Dynamic if so
constexpr int DimensionSO(int N) {
  return (N < 0) ? Eigen::Dynamic : N * (N - 1) / 2;
}

// Calculate N^2 at compile time, or return Dynamic if so
constexpr int NSquaredSO(int N) { return (N < 0) ? Eigen::Dynamic : N * N; }
}  // namespace internal

/**
 * Manifold of special orthogonal rotation matrices SO<N>.
 * Template paramater N can be a fizxed integer or can be Eigen::Dynamic
 */
template <int N>
class SO : public LieGroup<SO<N>, internal::DimensionSO(N)> {
 public:
  enum { dimension = internal::DimensionSO(N) };
  using MatrixNN = Eigen::Matrix<double, N, N>;
  using VectorN2 = Eigen::Matrix<double, internal::NSquaredSO(N), 1>;
  using MatrixDD = Eigen::Matrix<double, dimension, dimension>;

 protected:
  MatrixNN matrix_;  ///< Rotation matrix

  // enable_if_t aliases, used to specialize constructors/methods, see
  // https://www.fluentcpp.com/2018/05/18/make-sfinae-pretty-2-hidden-beauty-sfinae/
  template <int N_>
  using IsDynamic = boost::enable_if_t<N_ == Eigen::Dynamic, void>;
  template <int N_>
  using IsFixed = boost::enable_if_t<N_ >= 2, void>;
  template <int N_>
  using IsSO3 = boost::enable_if_t<N_ == 3, void>;

 public:
  /// @name Constructors
  /// @{

  /// Construct SO<N> identity for N >= 2
  template <int N_ = N, typename = IsFixed<N_>>
  SO() : matrix_(MatrixNN::Identity()) {}

  /// Construct SO<N> identity for N == Eigen::Dynamic
  template <int N_ = N, typename = IsDynamic<N_>>
  explicit SO(size_t n = 0) {
    if (n == 0) throw std::runtime_error("SO: Dimensionality not known.");
    matrix_ = Eigen::MatrixXd::Identity(n, n);
  }

  /// Constructor from Eigen Matrix
  template <typename Derived>
  explicit SO(const Eigen::MatrixBase<Derived>& R) : matrix_(R.eval()) {}

  /// Constructor from AngleAxisd
  template <int N_ = N, typename = IsSO3<N_>>
  SO(const Eigen::AngleAxisd& angleAxis) : matrix_(angleAxis) {}

  /// Random SO(n) element (no big claims about uniformity)
  template <int N_ = N, typename = IsDynamic<N_>>
  static SO Random(boost::mt19937& rng, size_t n = 0) {
    if (n == 0) throw std::runtime_error("SO: Dimensionality not known.");
    // This needs to be re-thought!
    static boost::uniform_real<double> randomAngle(-M_PI, M_PI);
    const size_t d = SO::Dimension(n);
    Vector xi(d);
    for (size_t j = 0; j < d; j++) {
      xi(j) = randomAngle(rng);
    }
    return SO::Retract(xi);
  }

  /// Random SO(N) element (no big claims about uniformity)
  template <int N_ = N, typename = IsFixed<N_>>
  static SO Random(boost::mt19937& rng) {
    // By default, use dynamic implementation above. Specialized for SO(3).
    return SO(SO<Eigen::Dynamic>::Random(rng, N).matrix());
  }

  /// @}
  /// @name Standard methods
  /// @{

  /// Return matrix
  const MatrixNN& matrix() const { return matrix_; }

  size_t rows() const { return matrix_.rows(); }
  size_t cols() const { return matrix_.cols(); }

  /// @}
  /// @name Testable
  /// @{

  void print(const std::string& s) const {
    std::cout << s << matrix_ << std::endl;
  }

  bool equals(const SO& other, double tol) const {
    return equal_with_abs_tol(matrix_, other.matrix_, tol);
  }

  /// @}
  /// @name Group
  /// @{

  /// Multiplication
  SO operator*(const SO& other) const { return SO(matrix_ * other.matrix_); }

  /// SO<N> identity for N >= 2
  template <int N_ = N, typename = IsFixed<N_>>
  static SO identity() {
    return SO();
  }

  /// SO<N> identity for N == Eigen::Dynamic
  template <int N_ = N, typename = IsDynamic<N_>>
  static SO identity(size_t n = 0) {
    return SO(n);
  }

  /// inverse of a rotation = transpose
  SO inverse() const { return SO(matrix_.transpose()); }

  /// @}
  /// @name Manifold
  /// @{

  using TangentVector = Eigen::Matrix<double, dimension, 1>;
  using ChartJacobian = OptionalJacobian<dimension, dimension>;

  /// Return compile-time dimensionality: fixed size N or Eigen::Dynamic
  static int Dim() { return dimension; }

  // Calculate manifold dimensionality for SO(n).
  // Available as dimension or Dim() for fixed N.
  static size_t Dimension(size_t n) { return n * (n - 1) / 2; }

  // Calculate ambient dimension n from manifold dimensionality d.
  static size_t AmbientDim(size_t d) { return (1 + std::sqrt(1 + 8 * d)) / 2; }

  // Calculate run-time dimensionality of manifold.
  // Available as dimension or Dim() for fixed N.
  size_t dim() const { return Dimension(matrix_.rows()); }

  /**
   * Hat operator creates Lie algebra element corresponding to d-vector, where d
   * is the dimensionality of the manifold. This function is implemented
   * recursively, and the d-vector is assumed to laid out such that the last
   * element corresponds to so(2), the last 3 to so(3), the last 6 to so(4)
   * etc... For example, the vector-space isomorphic to so(5) is laid out as:
   *   a b c d | u v w | x y | z
   * where the latter elements correspond to "telescoping" sub-algebras:
   *   0 -z  y -w  d
   *   z  0 -x  v -c
   *  -y  x  0 -u  b
   *   w -v  u  0 -a
   *  -d  c -b  a  0
   * This scheme behaves exactly as expected for SO(2) and SO(3).
   */
  static Matrix Hat(const Vector& xi);

  /**
   * Inverse of Hat. See note about xi element order in Hat.
   */
  static Vector Vee(const Matrix& X);

  // Chart at origin
  struct ChartAtOrigin {
    /**
     * Retract uses Cayley map. See note about xi element order in Hat.
     * Deafault implementation has no Jacobian implemented
     */
    static SO Retract(const TangentVector& xi, ChartJacobian H = boost::none);

    /**
     * Inverse of Retract. See note about xi element order in Hat.
     */
    static TangentVector Local(const SO& R, ChartJacobian H = boost::none);
  };

  // Return dynamic identity DxD Jacobian for given SO(n)
  template <int N_ = N, typename = IsDynamic<N_>>
  static MatrixDD IdentityJacobian(size_t n) {
    const size_t d = Dimension(n);
    return MatrixDD::Identity(d, d);
  }

  /// @}
  /// @name Lie Group
  /// @{

  MatrixDD AdjointMap() const {
    throw std::runtime_error(
        "SO<N>::AdjointMap only implemented for SO3 and SO4.");
  }

  /**
   * Exponential map at identity - create a rotation from canonical coordinates
   */
  static SO Expmap(const TangentVector& omega, ChartJacobian H = boost::none) {
    throw std::runtime_error("SO<N>::Expmap only implemented for SO3 and SO4.");
  }

  /**
   * Log map at identity - returns the canonical coordinates of this rotation
   */
  static TangentVector Logmap(const SO& R, ChartJacobian H = boost::none) {
    throw std::runtime_error("SO<N>::Logmap only implemented for SO3 and SO4.");
  }

  // template <int N_ = N, typename = IsSO3<N_>>
  static Matrix3 LogmapDerivative(const Vector3& omega);

  // inverse with optional derivative
  using LieGroup<SO<N>, internal::DimensionSO(N)>::inverse;

  /// @}
  /// @name Other methods
  /// @{

  /**
   * Return vectorized rotation matrix in column order.
   * Will use dynamic matrices as intermediate results, but returns a fixed size
   * X and fixed-size Jacobian if dimension is known at compile time.
   * */
  VectorN2 vec(OptionalJacobian<internal::NSquaredSO(N), dimension> H =
                   boost::none) const;
  /// @}
};

using SOn = SO<Eigen::Dynamic>;

/*
 * Fully specialize compose and between, because the derivative is unknowable by
 * the LieGroup implementations, who return a fixed-size matrix for H2.
 */

using DynamicJacobian = OptionalJacobian<Eigen::Dynamic, Eigen::Dynamic>;

template <>
SOn LieGroup<SOn, Eigen::Dynamic>::compose(const SOn& g, DynamicJacobian H1,
                                           DynamicJacobian H2) const;

template <>
SOn LieGroup<SOn, Eigen::Dynamic>::between(const SOn& g, DynamicJacobian H1,
                                           DynamicJacobian H2) const;

/*
 * Define the traits. internal::LieGroup provides both Lie group and Testable
 */

template <int N>
struct traits<SO<N>> : public internal::LieGroup<SO<N>> {};

template <int N>
struct traits<const SO<N>> : public internal::LieGroup<SO<N>> {};

}  // namespace gtsam

#include "SOn-inl.h"