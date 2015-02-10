#include <memory>
#include <fstream>
#include <algorithm>

#include "include/tbb.h"
#include "include/libint.h"
#include "include/tiledarray.h"
#include "include/btas.h"

#include "utility/make_array.h"

#include "molecule/atom.h"
#include "molecule/cluster.h"
#include "molecule/molecule.h"
#include "molecule/clustering_functions.h"

#include "basis/atom_basisset.h"
#include "basis/basis_set.h"
#include "basis/cluster_shells.h"
#include "basis/basis.h"

#include "integrals/btas_to_ta_tensor.h"
#include "integrals/btas_to_low_rank_tensor.h"
#include "integrals/ta_tensor_to_low_rank_tensor.h"
#include "integrals/integral_engine_pool.h"
#include "integrals/dense_task_integrals.h"
#include "integrals/sparse_task_integrals.h"

#include "purification/purification_devel.h"
#include "purification/sqrt_inv.h"

using namespace tcc;

double tile_size(tensor::TilePimpl<double> const &tile) {
    if (tile.isFull()) {
        return tile.tile().ftile().size();
    } else {
        return tile.tile().lrtile().size();
    }
}

double tile_size(TiledArray::Tensor<double> const &tile) { return 0.0; }

template <typename T, unsigned int DIM, typename TileType, typename Policy>
std::array<double, 3>
array_storage(TiledArray::Array<T, DIM, TileType, Policy> const &A) {

    std::array<double, 3> out = {{0.0, 0.0, 0.0}};
    double &full_size = out[0];
    double &sparse_size = out[1];
    double &low_size = out[2];

    auto const &pmap = A.get_pmap();
    TiledArray::TiledRange const &trange = A.trange();
    const auto end = pmap->end();
    for (auto it = pmap->begin(); it != end; ++it) {
        const TiledArray::Range range = trange.make_tile_range(*it);
        auto const &size_array = range.size();
        auto const size = std::accumulate(size_array.begin(), size_array.end(),
                                          1.0, std::multiplies<double>{});
        full_size += size;

        if (!A.is_zero(*it)) {
            sparse_size += size;
            low_size += tile_size(A.find(*it));
        }
    }

    A.get_world().gop.sum(&out[0], 3);

    out[0] *= 8 * 1e-9;
    out[1] *= 8 * 1e-9;
    out[2] *= 8 * 1e-9;

    if (A.get_world().rank() == 0) {
        std::cout << "Full storage = " << out[0] << " GB. ";
        std::cout << "Sparse storage = " << out[1] << " GB. ";
        std::cout << "Low storage = " << out[2] << " GB. " << std::endl;
    }

    return out;
}

template <typename Policy>
double array_diff(TiledArray::Array<double, 3, TiledArray::Tensor<double>,
                                    Policy> const &other,
                  TiledArray::Array<double, 3, tensor::TilePimpl<double>,
                                    TiledArray::SparsePolicy> const &lr) {
    double out = 0;
    auto const &pmap_ptr = other.get_pmap();
    const auto end = pmap_ptr->end();
    auto it_lr = lr.get_pmap()->begin();

    for (auto it = pmap_ptr->begin(); it != end; ++it, ++it_lr) {
        const auto range = other.trange().make_tile_range(*it);
        const auto rows = range.size()[0];
        const auto cols = range.size()[1] * range.size()[2];
        Eigen::MatrixXd s_tile = Eigen::MatrixXd::Zero(rows, cols);
        Eigen::MatrixXd lr_tile = Eigen::MatrixXd::Zero(rows, cols);

        if (!other.is_zero(*it)) {
            const auto tensor = other.find(*it).get();
            s_tile = TiledArray::eigen_map(tensor, rows, cols);
        }
        if (!lr.is_zero(*it_lr)) {
            tensor::TilePimpl<double> tile_lr = lr.find(*it_lr);
            lr_tile = tile_lr.tile().matrix();
        }
        const auto norm = (s_tile - lr_tile).lpNorm<2>();
        out += norm * norm;
    }

    // sum outs from all nodes
    other.get_world().gop.sum(&out, 1);
    other.get_world().gop.fence();
    out = std::sqrt(out) / other.trange().elements().volume();
    return out;
}


int main(int argc, char *argv[]) {
    auto &world = madness::initialize(argc, argv);
    std::string mol_file = "";
    std::string basis_name = "";
    std::string df_basis_name = "";
    int nclusters = 0;
    int debug = 0;
    if (argc >= 5) {
        mol_file = argv[1];
        basis_name = argv[2];
        df_basis_name = argv[3];
        nclusters = std::stoi(argv[4]);
    } else {
        std::cout << "input is $./program mol_file basis_file df_basis_file "
                     "nclusters \n";
        return 0;
    }
    if (argc == 6) {
        debug = std::stoi(argv[5]);
    }

    auto mol = molecule::read_xyz(mol_file);

    basis::BasisSet bs{basis_name};
    basis::BasisSet df_bs{df_basis_name};

    if (world.rank() == 0) {
        std::cout << mol.nelements() << " elements with " << nclusters
                  << " clusters" << std::endl;
    }

    std::vector<std::shared_ptr<molecule::Cluster>> clusters;
    clusters.reserve(nclusters);
    for (auto &&cluster : mol.attach_H_and_kmeans(nclusters)) {
        clusters.push_back(
            std::make_shared<molecule::Cluster>(std::move(cluster)));
    }

    basis::Basis basis{bs.create_basis(clusters)};
    basis::Basis df_basis{df_bs.create_basis(clusters)};

    auto max_am = std::max(basis.max_am(), df_basis.max_am());
    auto max_nprim = std::max(basis.max_nprim(), df_basis.max_nprim());

    libint2::init();
    world.gop.fence();

    libint2::TwoBodyEngine<libint2::Coulomb> eri{max_nprim,
                                                 static_cast<int>(max_am)};


    auto eri_pool = integrals::make_pool(std::move(eri));

    world.gop.fence();
    auto eri2_dense = integrals::DenseIntegrals(
        world, eri_pool, utility::make_array(df_basis, df_basis),
        integrals::compute_functors::BtasToTaTensor{});

    eri2_dense = pure::inverse_sqrt(eri2_dense);

    auto eri3_dense = integrals::DenseIntegrals(
        world, eri_pool, utility::make_array(df_basis, basis, basis),
        integrals::compute_functors::BtasToTaTensor{});

    eri3_dense("X,a,b") = eri2_dense("X,P") * eri3_dense("P,a,b");

    if (world.rank() == 0) {
        std::cout << "\nConverting to low rank block sparse array "
                     "from dense" << std::endl;
    }
    {
        auto eri3 = TiledArray::to_sparse(eri3_dense);
        auto eri3_low_rank = TiledArray::conversion::to_new_tile_type(
            eri3, integrals::compute_functors::TaToLowRankTensor<3>{1e-8});

        if (world.rank() == 0) {
            std::cout << "Eri3 sparse low rank from dense storage ";
        }
        array_storage(eri3_low_rank);
        double best_eri3_diff = array_diff(eri3_dense, eri3_low_rank);
        if (world.rank() == 0) {
            std::cout << "Block sparse row rank error in the contracted product = "
                      << best_eri3_diff << std::endl;
        }
    }

    const auto thresh = TiledArray::SparseShape<float>::threshold();
    if (world.rank() == 0) {
        std::cout << "Finally the sparse shape threshold was = " << thresh
                  << std::endl;
    }


    world.gop.fence();
    libint2::cleanup();
    madness::finalize();
    return 0;
}
