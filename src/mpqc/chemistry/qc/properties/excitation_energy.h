//
// Created by Chong Peng on 3/2/17.
//

#ifndef SRC_MPQC_CHEMISTRY_QC_PROPERTIES_EXCITATION_ENERGY_H_
#define SRC_MPQC_CHEMISTRY_QC_PROPERTIES_EXCITATION_ENERGY_H_

#include <iostream>

#include "mpqc/chemistry/qc/properties/property.h"


namespace mpqc {

template <typename T>
std::basic_ostream<char, std::char_traits<char> >& operator<<(
    std::basic_ostream<char, std::char_traits<char>>& out,
    const std::vector<T>& v) {
  out << "[";
  const auto len = v.size() - 1;
  for (size_t i = 0; i < len; ++i) {
    out << v[i] << ", ";
  }
  // last one
  out << v[len];
  out << "]";
  return out;
}

/**
 * Property that computes Excitation Energy
 */

class ExcitationEnergy : public WavefunctionProperty<std::vector<double>> {
 public:
  using typename WavefunctionProperty<std::vector<double>>::function_base_type;

  class Provider : public math::FunctionVisitorBase<function_base_type> {
   public:
    virtual bool can_evaluate(ExcitationEnergy* ex_energy) = 0;
    /// evaluate Excitation Energy and use set_value to assign the values to \c
    /// ex_energy
    virtual void evaluate(ExcitationEnergy* ex_energy) = 0;
  };

  // clang-format off
  /**
   * @brief The KeyVal constructor
   * @param kv the KeyVal object, it will be queried for all
   *        keywords of the WavefunctionProperty class, as well as the following
   * keywords:
   * | Keyword | Type | Default| Description |
   * |---------|------|--------|-------------|
   * | n_roots | unsigned int | 1 | number of roots to compute |
   *
   * @note This constructor overrides the default target precision to 1e-4 \f$ \sim 3~{\rm meV} \f$.
   */
  // clang-format on
  explicit ExcitationEnergy(const KeyVal& kv);

  /// @return number of roots
  unsigned int n_roots() const;

 private:
  void do_evaluate() override;

  unsigned int n_roots_;
};

}  // namespace mpqc

#endif  // SRC_MPQC_CHEMISTRY_QC_PROPERTIES_EXCITATION_ENERGY_H_
