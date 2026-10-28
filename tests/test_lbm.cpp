#include "lbm/lattice.hpp"
#include "lbm/simulator.hpp"
#include "vtk_writer.hpp"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(double a, double b, double tolerance = 1e-11) {
    return std::abs(a - b) <= tolerance;
}

void test_d3q19_definition() {
    double weight_sum = 0.0;
    for (std::size_t q = 0; q < lbm::Q; ++q) {
        weight_sum += lbm::w[q];
        const auto opposite = lbm::opposite[q];
        require(opposite < lbm::Q, "opposite index is in range");
        require(lbm::opposite[opposite] == q, "opposite mapping is an involution");
        require(near(lbm::c[q].x + lbm::c[opposite].x, 0.0) &&
                near(lbm::c[q].y + lbm::c[opposite].y, 0.0) &&
                near(lbm::c[q].z + lbm::c[opposite].z, 0.0), "opposite vectors cancel");
    }
    require(near(weight_sum, 1.0), "D3Q19 weights sum to one");
    require(near(lbm::w[0], 1.0 / 3.0), "rest weight is 1/3");
    for (std::size_t q = 1; q <= 6; ++q)
        require(near(lbm::w[q], 1.0 / 18.0), "axis weights are 1/18");
    for (std::size_t q = 7; q < lbm::Q; ++q)
        require(near(lbm::w[q], 1.0 / 36.0), "diagonal weights are 1/36");

    const double rho = 1.23;
    const lbm::Vec3 u{0.03, -0.02, 0.01};
    double reconstructed_rho = 0.0, mx = 0.0, my = 0.0, mz = 0.0;
    for (std::size_t q = 0; q < lbm::Q; ++q) {
        const double f = lbm::equilibrium(q, rho, u);
        reconstructed_rho += f;
        mx += f * lbm::c[q].x;
        my += f * lbm::c[q].y;
        mz += f * lbm::c[q].z;
    }
    require(near(reconstructed_rho, rho), "equilibrium recovers density");
    require(near(mx, rho * u.x) && near(my, rho * u.y) && near(mz, rho * u.z),
            "equilibrium recovers momentum");
}

void test_uniform_equilibrium_is_steady() {
    lbm::Simulator simulation(7, 5, 4, 0.8);
    simulation.initialize_equilibrium(1.1, {0.02, -0.01, 0.005});
    const double expected_mass = 1.1 * static_cast<double>(simulation.cell_count());
    simulation.run(20);
    require(near(simulation.total_mass(), expected_mass, 1e-10), "uniform equilibrium mass remains constant");
    for (std::size_t z = 0; z < simulation.nz(); ++z)
        for (std::size_t y = 0; y < simulation.ny(); ++y)
            for (std::size_t x = 0; x < simulation.nx(); ++x) {
                require(near(simulation.density(x, y, z), 1.1, 1e-10), "uniform equilibrium density remains steady");
                const auto u = simulation.velocity(x, y, z);
                require(near(u.x, 0.02, 1e-10) && near(u.y, -0.01, 1e-10) && near(u.z, 0.005, 1e-10),
                        "uniform equilibrium velocity remains steady");
            }
}

void test_mass_conservation_with_density_variation_and_solid() {
    lbm::Simulator simulation(9, 7, 6, 0.9);
    for (std::size_t z = 0; z < simulation.nz(); ++z)
        for (std::size_t y = 0; y < simulation.ny(); ++y)
            for (std::size_t x = 0; x < simulation.nx(); ++x) {
                const double rho = 1.0 + 0.002 * static_cast<double>((x + 2 * y + 3 * z) % 7);
                simulation.set_equilibrium(x, y, z, rho);
            }
    simulation.set_solid(4, 3, 3);
    simulation.set_solid(4, 3, 2);
    const double initial_mass = simulation.total_mass();
    simulation.run(40);
    require(near(simulation.total_mass(), initial_mass, 1e-9), "periodic bounce-back conserves fluid mass");
    for (std::size_t z = 0; z < simulation.nz(); ++z)
        for (std::size_t y = 0; y < simulation.ny(); ++y)
            for (std::size_t x = 0; x < simulation.nx(); ++x)
                if (!simulation.is_solid(x, y, z))
                    require(std::isfinite(simulation.density(x, y, z)) && simulation.density(x, y, z) > 0.0,
                            "fluid density remains positive and finite");
}

void test_vtk_writer() {
    lbm::Simulator simulation(2, 2, 1);
    simulation.initialize_equilibrium(1.0);
    const std::string path = "lbm_test_output.vtk";
    lbm::write_vtk_structured_points(simulation, path);
    std::ifstream input(path);
    require(static_cast<bool>(input), "VTK file was created");
    std::string contents((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    require(contents.find("DATASET STRUCTURED_POINTS") != std::string::npos, "VTK dataset type is present");
    require(contents.find("DIMENSIONS 2 2 1") != std::string::npos, "VTK dimensions are present");
    require(contents.find("SCALARS density double 1") != std::string::npos, "VTK density scalar is present");
    require(contents.find("VECTORS velocity double") != std::string::npos, "VTK velocity vectors are present");
    std::remove(path.c_str());
}

}  // namespace

int main() {
    try {
        test_d3q19_definition();
        test_uniform_equilibrium_is_steady();
        test_mass_conservation_with_density_variation_and_solid();
        test_vtk_writer();
        std::cout << "All LBM tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Test failure: " << error.what() << '\n';
        return 1;
    }
}
