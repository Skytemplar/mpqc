//
// Created by Chong Peng on 2/16/16.
//

#ifndef TILECLUSTERCHEM_ORBITAL_REGISTRY_H
#define TILECLUSTERCHEM_ORBITAL_REGISTRY_H

#include "formula_registry.h"
#include "orbital_space.h"


namespace mpqc{

/**
 *  \brief map OrbitalIndex to Value object
 */
template <typename Value>
class OrbitalRegistry : public Registry<OrbitalIndex,Value>{
public:

    using Key = OrbitalIndex;
    using value_type = typename Registry<Key,Value>::value_type;
    using element_type = typename Registry<Key,Value>::element_type;
    using iterator = typename Registry<Key,Value>::iterator;
    using const_iterator = typename Registry<Key,Value>::const_iterator;

    OrbitalRegistry()= default;
    OrbitalRegistry(const element_type& map) : Registry<Key,Value>(map){}

    /// add Value that has mo_key() function as key type
    void add(const Value& val){
        this->insert(val.mo_key(),val);
    }

    /// add by Key and Value
    void add(const Key& key, const Value& val){
        this->insert(key, val);
    }
};

/**
 * Typedef of OrbitalSpaceRegistry
 * A Registry that map OrbitalIndex to OrbitalSpace class
 */
template <typename Array>
using OrbitalSpaceRegistry = OrbitalRegistry<OrbitalSpace<Array>>;

/**
 * Typedef of OrbitalBasisRegistry
 * A Registry that map OrbitalIndex to Basis
 */
using OrbitalBasisRegistry = OrbitalRegistry<std::shared_ptr<mpqc::basis::Basis>>;

} // end of namespace mpqc


#endif //TILECLUSTERCHEM_ORBITAL_REGISTRY_H
