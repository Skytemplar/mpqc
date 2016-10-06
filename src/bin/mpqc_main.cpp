/// New MPQC Main file with KeyVal

#include "../../include/tiledarray.h"

#include "../../utility/parallel_file.h"
#include "../../utility/parallel_print.h"

#include <mpqc/chemistry/qc/properties/energy.h>
#include <mpqc/chemistry/qc/wfn/wfn.h>
#include <mpqc/util/keyval/keyval.hpp>

// include linkage file
#include <mpqc/chemistry/qc/wfn/linkage.h>
#include <mpqc/chemistry/qc/mbpt/linkage.h>
#include <mpqc/chemistry/qc/scf/linkage.h>
#include <mpqc/chemistry/molecule/linkage.h>

#include <sstream>
#include <clocale>

using namespace mpqc;

int try_main(int argc, char *argv[], madness::World &world) {
  if (argc != 2) {
    std::cout << "usage: " << argv[0] << " <input_file.json>" << std::endl;
    throw std::invalid_argument("no input file given");
  }

  std::stringstream ss;
  utility::parallel_read_file(world, argv[1], ss);

  KeyVal kv;
  kv.read_json(ss);
  kv.assign("world", &world);

  auto threshold = 1e-11;  // Hardcode for now.
  TiledArray::SparseShape<float>::threshold(threshold);

  libint2::initialize();

  auto wfn = kv.keyval("wfn").class_ptr<qc::Wfn>();

//  auto energy_prop = qc::Energy(kv);
//  auto energy_prop_ptr = &energy_prop;

  double val = wfn->value();
  utility::print_par(world,"Wfn energy is: ", val, "\n");


  libint2::finalize();
  madness::finalize();

  return 0;
}

int main(int argc, char *argv[]) {
  int rc = 0;

  auto &world = madness::initialize(argc, argv);
  mpqc::utility::print_par(world, "MADNESS process total size: ", world.size(),
                           "\n");

  std::setlocale(LC_ALL, "en_US.UTF-8");
  std::cout << std::setprecision(15);
  std::wcout.sync_with_stdio(false);
  std::wcerr.sync_with_stdio(false);
  std::wcout.imbue(std::locale("en_US.UTF-8"));
  std::wcerr.imbue(std::locale("en_US.UTF-8"));
  std::wcout.sync_with_stdio(true);
  std::wcerr.sync_with_stdio(true);

  try {

    try_main(argc, argv, world);

  } catch (TiledArray::Exception &e) {
    std::cerr << "!! TiledArray exception: " << e.what() << "\n";
    rc = 1;
  } catch (madness::MadnessException &e) {
    std::cerr << "!! MADNESS exception: " << e.what() << "\n";
    rc = 1;
  } catch (SafeMPI::Exception &e) {
    std::cerr << "!! SafeMPI exception: " << e.what() << "\n";
    rc = 1;
  } catch (std::exception &e) {
    std::cerr << "!! std exception: " << e.what() << "\n";
    rc = 1;
  } catch (...) {
    std::cerr << "!! exception: unknown exception\n";
    rc = 1;
  }

  return rc;
}
