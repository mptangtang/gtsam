/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation, 
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file    Testable.h
 * @brief   Concept check for values that can be used in unit tests
 * @author  Frank Dellaert
 * 
 * The necessary functions to implement for Testable are defined
 * below with additional details as to the interface.
 * The concept checking function will check whether or not
 * the function exists in derived class and throw compile-time errors.
 * 
 * print with optional string naming the object
 *     void print(const std::string& name) const = 0;
 * 
 * equality up to tolerance
 * tricky to implement, see NoiseModelFactor1 for an example
 * equals is not supposed to print out *anything*, just return true|false
 *     bool equals(const Derived& expected, double tol) const = 0;
 * 
 */

// \callgraph

#pragma once

#include <boost/shared_ptr.hpp>
#include <boost/concept_check.hpp>
#include <stdio.h>
#include <string>

#define GTSAM_PRINT(x)((x).print(#x))

namespace gtsam {

  // Forward declaration
  template <typename T> struct traits_x;

  /**
   * A testable concept check that should be placed in applicable unit
   * tests and in generic algorithms.
   *
   * See macros for details on using this structure
   * @addtogroup base
   * @tparam T is the objectype this constrains to be testable - assumes print() and equals()
   */
  template <class T>
  class IsTestable {
    T t;
    bool r1,r2;
  public:

    BOOST_CONCEPT_USAGE(IsTestable) {
      // check print function, with optional string
      traits_x<T>::Print(t, std::string());
      traits_x<T>::Print(t);

      // check print, with optional threshold
      double tol = 1.0;
      r1 = traits_x<T>::Equals(t,t,tol);
      r2 = traits_x<T>::Equals(t,t);
    }
  }; // \ Testable

  inline void print(float v, const std::string& s = "") {
    printf("%s%f\n",s.c_str(),v);
  }
  inline void print(double v, const std::string& s = "") {
    printf("%s%lf\n",s.c_str(),v);
  }

  /** Call equal on the object */
  template<class T>
  inline bool equal(const T& obj1, const T& obj2, double tol) {
    return traits_x<T>::Equals(obj1,obj2, tol);
  }

  /** Call equal without tolerance (use default tolerance) */
  template<class T>
  inline bool equal(const T& obj1, const T& obj2) {
    return traits_x<T>::Equals(obj1,obj2);
  }

  /**
   * This template works for any type with equals
   */
  template<class V>
  bool assert_equal(const V& expected, const V& actual, double tol = 1e-9) {
    if (traits_x<V>::Equals(actual,expected, tol))
      return true;
    printf("Not equal:\n");
    traits_x<V>::Print(expected,"expected:\n");
    traits_x<V>::Print(actual,"actual:\n");
    return false;
  }

  /**
   * Template to create a binary predicate
   */
  template<class V>
  struct equals : public std::binary_function<const V&, const V&, bool> {
    double tol_;
    equals(double tol = 1e-9) : tol_(tol) {}
    bool operator()(const V& expected, const V& actual) {
      return (traits_x<V>::Equals(actual, expected, tol_));
    }
  };

  /**
   * Binary predicate on shared pointers
   */
  template<class V>
  struct equals_star : public std::binary_function<const boost::shared_ptr<V>&, const boost::shared_ptr<V>&, bool> {
    double tol_;
    equals_star(double tol = 1e-9) : tol_(tol) {}
    bool operator()(const boost::shared_ptr<V>& expected, const boost::shared_ptr<V>& actual) {
      if (!actual || !expected) return false;
      return (traits_x<V>::Equals(*actual,*expected, tol_));
    }
  };

  /// Requirements on type to pass it to Testable template below
  template<typename T>
  struct HasTestablePrereqs {

    BOOST_CONCEPT_USAGE(HasTestablePrereqs) {
      t->print(str);
      b = t->equals(*s,tol);
    }

    T *t, *s; // Pointer is to allow abstract classes
    bool b;
    double tol;
    std::string str;
  };

  /// A helper that implements the traits interface for GTSAM types.
  /// To use this for your gtsam type, define:
  /// template<> struct traits<Type> : public Testable<Type> { };
  template<typename T>
  struct Testable {

    // Check that T has the necessary methods
    BOOST_CONCEPT_ASSERT((HasTestablePrereqs<T>));

    static void Print(const T& m, const std::string& str = "") {
      m.print(str);
    }
    static bool Equals(const T& m1, const T& m2, double tol = 1e-8) {
      return m1.equals(m2, tol);
    }
  };

} // \namespace gtsam

/**
 * Macros for using the TestableConcept
 *  - An instantiation for use inside unit tests
 *  - A typedef for use inside generic algorithms
 *
 * NOTE: intentionally not in the gtsam namespace to allow for classes not in
 * the gtsam namespace to be more easily enforced as testable
 * @deprecated please use BOOST_CONCEPT_ASSERT and
 */
#define GTSAM_CONCEPT_TESTABLE_INST(T) template class gtsam::IsTestable<T>;
#define GTSAM_CONCEPT_TESTABLE_TYPE(T) typedef gtsam::IsTestable<T> _gtsam_Testable_##T;
