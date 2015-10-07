// #include "../molecule/cluster.h"

#include "../molecule/molecule.h"

#include "basis.h"
#include "basis_set.h"
#include "shell_vec_functions.h"

#include "../include/libint.h"
#include "../include/tiledarray.h"

#include <vector>

namespace mpqc {

namespace mol = molecule;

namespace basis {

Basis::Basis() = default;
Basis::~Basis() = default;
Basis::Basis(Basis const &) = default;
Basis::Basis(Basis &&) = default;

Basis &Basis::operator=(Basis const &) = default;
Basis &Basis::operator=(Basis &&) = default;

Basis::Basis(std::vector<ShellVec> shells) : shells_(std::move(shells)) {}

Basis::Basis(std::vector<ShellVec> shells, mol::Molecule const &mol)
        : shells_(std::move(shells)),
          mol_(std::make_shared<mol::Molecule>(mol::Molecule(mol))) {}

Basis::Basis(std::vector<ShellVec> shells,
             std::shared_ptr<mol::Molecule> mol_ptr)
        : shells_(std::move(shells)), mol_(std::move(mol_ptr)) {}

TiledArray::TiledRange1 Basis::create_trange1() const {
    auto blocking = std::vector<int64_t>{0};
    for (auto const &shell_vec : shells_) {
        auto next = blocking.back() + basis::nfunctions(shell_vec);
        blocking.emplace_back(next);
    }

    return TiledArray::TiledRange1(blocking.begin(), blocking.end());
}

int64_t Basis::max_nprim() const {
    int64_t max = 0;
    for (auto const &shell_vec : shells_) {
        const auto current = basis::max_nprim(shell_vec);
        max = std::max(current, max);
    }
    return max;
}

int64_t Basis::max_am() const {
    int64_t max = 0;
    for (auto const &shell_vec : shells_) {
        const auto current = basis::max_am(shell_vec);
        max = std::max(current, max);
    }
    return max;
}

int64_t Basis::nshells() const {
    return std::accumulate(shells_.begin(), shells_.end(), int64_t(0),
                           [](int64_t x, ShellVec const &a) {
        return x + int64_t(a.size());
    });
}


std::vector<ShellVec> const &Basis::cluster_shells() const { return shells_; }

molecule::Molecule const &Basis::molecule() const {
    if (mol_ == nullptr) {
        throw std::logic_error("Basis: pointer to molecule was null.");
    }

    return *mol_;
}

std::ostream &operator<<(std::ostream &os, Basis const &b) {
    unsigned int n = 0;
    for (auto const &shell_vec : b.cluster_shells()) {
        os << "Cluster " << n << "\n";
        ++n;

        for (auto const &shell : shell_vec) {
            os << shell << "\n";
        }
    }

    return os;
}

} // namespace basis
} // namespace basis
