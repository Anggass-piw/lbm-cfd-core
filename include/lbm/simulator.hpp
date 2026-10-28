#pragma once

#include "lbm/lattice.hpp"

#include <cstddef>
#include <vector>

namespace lbm {

class Simulator {
public:
    Simulator(std::size_t nx, std::size_t ny, std::size_t nz, double relaxation_time = 0.8);

    std::size_t nx() const noexcept { return nx_; }
    std::size_t ny() const noexcept { return ny_; }
    std::size_t nz() const noexcept { return nz_; }
    std::size_t cell_count() const noexcept { return cells_; }
    double relaxation_time() const noexcept { return tau_; }

    // Initialize every cell to a spatially uniform equilibrium state.
    void initialize_equilibrium(double density = 1.0, Vec3 velocity = {});
    // Set one cell to equilibrium; useful for initial/boundary conditions and tests.
    void set_equilibrium(std::size_t x, std::size_t y, std::size_t z,
                         double density, Vec3 velocity = {});
    void set_solid(std::size_t x, std::size_t y, std::size_t z, bool solid = true);
    bool is_solid(std::size_t x, std::size_t y, std::size_t z) const;

    // One BGK collision followed by periodic pull-streaming and halfway bounce-back
    // for links whose upstream cell is marked solid.
    void step();
    void run(std::size_t steps);

    double density(std::size_t x, std::size_t y, std::size_t z) const;
    Vec3 velocity(std::size_t x, std::size_t y, std::size_t z) const;
    double total_mass() const noexcept;

private:
    std::size_t index(std::size_t x, std::size_t y, std::size_t z) const;
    void check_coordinates(std::size_t x, std::size_t y, std::size_t z) const;
    void update_macroscopic();

    std::size_t nx_, ny_, nz_, cells_;
    double tau_, omega_;
    // Population fields are direction-major: f[q * cell_count + cell].
    std::vector<double> f_, post_, next_;
    std::vector<double> rho_, ux_, uy_, uz_;
    std::vector<unsigned char> solid_;
};

}  // namespace lbm
