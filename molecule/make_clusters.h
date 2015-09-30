#pragma once
#ifndef MPQC_MOLECULE_MAKECLUSTERS_H
#define MPQC_MOLECULE_MAKECLUSTERS_H

#include "molecule_fwd.h"
#include <memory>
#include <vector>

namespace mpqc {
namespace molecule {

std::vector<std::shared_ptr<Cluster>>
attach_hydrogens_kmeans(Molecule const &, unsigned long);

std::vector<std::shared_ptr<Cluster>>
kmeans(Molecule const &, unsigned long);

} // namespace molecule
} // namespace mpqc

#endif // MPQC_MOLECULE_MAKECLUSTERS_H
