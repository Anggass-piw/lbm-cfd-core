#include "vtk_writer.hpp"

#include <fstream>
#include <iomanip>
#include <stdexcept>

namespace lbm {

void write_vtk_structured_points(const Simulator& simulator, const std::string& path) {
    std::ofstream out(path);
    if (!out) throw std::runtime_error("could not open VTK output file: " + path);
    out << "# vtk DataFile Version 3.0\nD3Q19 LBM scalar and velocity fields\nASCII\n"
        << "DATASET STRUCTURED_POINTS\n"
        << "DIMENSIONS " << simulator.nx() << ' ' << simulator.ny() << ' ' << simulator.nz() << "\n"
        << "ORIGIN 0 0 0\nSPACING 1 1 1\n"
        << "POINT_DATA " << simulator.cell_count() << "\n"
        << "SCALARS density double 1\nLOOKUP_TABLE default\n" << std::setprecision(12);
    for (std::size_t z = 0; z < simulator.nz(); ++z)
        for (std::size_t y = 0; y < simulator.ny(); ++y)
            for (std::size_t x = 0; x < simulator.nx(); ++x)
                out << simulator.density(x, y, z) << '\n';
    out << "VECTORS velocity double\n";
    for (std::size_t z = 0; z < simulator.nz(); ++z)
        for (std::size_t y = 0; y < simulator.ny(); ++y)
            for (std::size_t x = 0; x < simulator.nx(); ++x) {
                const auto u = simulator.velocity(x, y, z);
                out << u.x << ' ' << u.y << ' ' << u.z << '\n';
            }
    if (!out) throw std::runtime_error("failed while writing VTK output file: " + path);
}

}  // namespace lbm
