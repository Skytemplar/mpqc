//
// Created by Chong Peng on 2/16/16.
//

#ifndef MPQC_ORBITAL_SPACE_H
#define MPQC_ORBITAL_SPACE_H

#include <memory>

#include <mpqc/util/expression/orbital_index.h>
#include <mpqc/chemistry/qc/basis/basis.h>
#include "operator.h"

namespace mpqc{

/**
 *  \brief Class that represent MO to AO coefficients
 *
 */

template <typename Array>
class OrbitalSpace{
public:

    OrbitalSpace() = default;

    /**
     * Constructor
     *
     *  @prama mo_index  OrbitalIndex that represent mo space
     *  @prama ao_index  OrbitalIndex that represent ao space
     *  @prama tarray    TiledArray type Array
     */

    OrbitalSpace(const OrbitalIndex& mo_index, const OrbitalIndex& ao_index, const Array& tarray)
            : mo_index_(mo_index), ao_index_(ao_index), coefs_(tarray)
    {}

    ~OrbitalSpace()= default;

    /// return mo_index
    OrbitalIndex &mo_key(){
        return mo_index_;
    }

    /// return mo_index
    const OrbitalIndex &mo_key() const {
        return mo_index_;
    }

    /// return ao_index
    OrbitalIndex &ao_key(){
        return ao_index_;
    }

    /// return ao_index
    const OrbitalIndex &ao_key() const {
        return ao_index_;
    }

    /// return coefficient
    Array& array() {
        return coefs_;
    }

    /// return coefficient
    const Array& array() const {
        return coefs_;
    }

    /// interface to TA::Array () function
    TA::expressions::TsrExpr<Array, true>
            operator()(const std::string& vars){
        return coefs_(vars);
    };

    /// interface to TA::Array () function
    TA::expressions::TsrExpr<const Array,true>
    operator()(const std::string& vars) const {
        return coefs_(vars);
    };

private:

    OrbitalIndex mo_index_;
    OrbitalIndex ao_index_;
    Array coefs_;

};

}

#endif //MPQC_ORBITAL_SPACE_H
