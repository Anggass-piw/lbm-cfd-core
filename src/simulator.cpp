#include "lbm/simulator.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace lbm {
namespace {

std::size_t checked_cell_count(std::size_t nx, std::size_t ny, std::size_t nz) {
    if (nx == 0 || ny == 0 || nz == 0) {
        throw std::invalid_argument("grid dimensions must all be positive");
    }
    if (nx > std::numeric_limits<std::size_t>::max() / ny ||
        nx * ny > std::numeric_limits<std::size_t>::max() / nz) {
        throw std::overflow_error("grid dimensions overflow addressable cell count");
    }
    return nx * ny * nz;
}

std::size_t upstream_coordinate(std::size_t coordinate, std::size_t extent, double direction) {
    if (direction > 0.0) return coordinate == 0 ? extent - 1 : coordinate - 1;
    if (direction < 0.0) return coordinate + 1 == extent ? 0 : coordinate + 1;
    return coordinate;
}

}  // namespace

Simulator::Simulator(std::size_t nx, std::size_t ny, std::size_t nz, double relaxation_time)
    : nx_(nx), ny_(ny), nz_(nz), cells_(checked_cell_count(nx, ny, nz)),
      tau_(relaxation_time), omega_(1.0 / relaxation_time),
      f_(Q * cells_), post_(Q * cells_), next_(Q * cells_),
      rho_(cells_, 1.0), ux_(cells_, 0.0), uy_(cells_, 0.0), uz_(cells_, 0.0),
      solid_(cells_, 0) {
    if (!std::isfinite(tau_) || tau_ <= 0.5) {
        throw std::invalid_argument("relaxation time tau must be finite and greater than 0.5");
    }
    initialize_equilibrium();
}

std::size_t Simulator::index(std::size_t x, std::size_t y, std::size_t z) const {
    return (z * ny_ + y) * nx_ + x;
}

void Simulator::check_coordinates(std::size_t x, std::size_t y, std::size_t z) const {
    if (x >= nx_ || y >= ny_ || z >= nz_) {
        throw std::out_of_range("grid coordinate is out of range");
    }
}

void Simulator::initialize_equilibrium(double density_value, Vec3 velocity_value) {
    if (!std::isfinite(density_value) || density_value <= 0.0 ||
        !std::isfinite(velocity_value.x) || !std::isfinite(velocity_value.y) ||
        !std::isfinite(velocity_value.z)) {
        throw std::invalid_argument("equilibrium density must be positive and state values finite");
    }
    for (std::size_t cell = 0; cell < cells_; ++cell) {
        rho_[cell] = density_value;
        ux_[cell] = velocity_value.x;
        uy_[cell] = velocity_value.y;
        uz_[cell] = velocity_value.z;
        for (std::size_t q = 0; q < Q; ++q) {
            f_[q * cells_ + cell] = equilibrium(q, density_value, velocity_value);
        }
    }
}

void Simulator::set_equilibrium(std::size_t x, std::size_t y, std::size_t z,
                                double density_value, Vec3 velocity_value) {
    check_coordinates(x, y, z);
    if (!std::isfinite(density_value) || density_value <= 0.0 ||
        !std::isfinite(velocity_value.x) || !std::isfinite(velocity_value.y) ||
        !std::isfinite(velocity_value.z)) {
        throw std::invalid_argument("equilibrium density must be positive and state values finite");
    }
    const auto cell = index(x, y, z);
    rho_[cell] = density_value;
    ux_[cell] = velocity_value.x;
    uy_[cell] = velocity_value.y;
    uz_[cell] = velocity_value.z;
    for (std::size_t q = 0; q < Q; ++q) {
        f_[q * cells_ + cell] = equilibrium(q, density_value, velocity_value);
    }
}

void Simulator::set_solid(std::size_t x, std::size_t y, std::size_t z, bool solid) {
    check_coordinates(x, y, z);
    solid_[index(x, y, z)] = static_cast<unsigned char>(solid);
}

bool Simulator::is_solid(std::size_t x, std::size_t y, std::size_t z) const {
    check_coordinates(x, y, z);
    return solid_[index(x, y, z)] != 0;
}

void Simulator::step() {
    // BGK collision. Solid nodes do not participate in fluid collision.
    for (std::size_t cell = 0; cell < cells_; ++cell) {
        if (solid_[cell]) {
            for (std::size_t q = 0; q < Q; ++q) {
                post_[q * cells_ + cell] = f_[q * cells_ + cell];
            }
            continue;
        }
        double rho = 0.0, mx = 0.0, my = 0.0, mz = 0.0;
        for (std::size_t q = 0; q < Q; ++q) {
            const double fq = f_[q * cells_ + cell];
            rho += fq;
            mx += fq * c[q].x;
            my += fq * c[q].y;
            mz += fq * c[q].z;
        }
        if (!(rho > 0.0) || !std::isfinite(rho)) {
            throw std::runtime_error("non-positive or non-finite density encountered");
        }
        const Vec3 u{mx / rho, my / rho, mz / rho};
        for (std::size_t q = 0; q < Q; ++q) {
            const auto offset = q * cells_ + cell;
            post_[offset] = f_[offset] - omega_ * (f_[offset] - equilibrium(q, rho, u));
        }
    }

    // Pull-stream from periodic upstream neighbors. A solid upstream link reflects
    // the opposite post-collision population back into the current fluid cell.
    for (std::size_t z = 0; z < nz_; ++z) {
        for (std::size_t y = 0; y < ny_; ++y) {
            for (std::size_t x = 0; x < nx_; ++x) {
                const auto cell = index(x, y, z);
                if (solid_[cell]) {
                    for (std::size_t q = 0; q < Q; ++q) {
                        next_[q * cells_ + cell] = f_[q * cells_ + cell];
                    }
                    continue;
                }
                for (std::size_t q = 0; q < Q; ++q) {
                    const auto upstream_x = upstream_coordinate(x, nx_, c[q].x);
                    const auto upstream_y = upstream_coordinate(y, ny_, c[q].y);
                    const auto upstream_z = upstream_coordinate(z, nz_, c[q].z);
                    const auto source = index(upstream_x, upstream_y, upstream_z);
                    if (solid_[source]) {
                        next_[q * cells_ + cell] = post_[opposite[q] * cells_ + cell];
                    } else {
                        next_[q * cells_ + cell] = post_[q * cells_ + source];
                    }
                }
            }
        }
    }
    f_.swap(next_);
    update_macroscopic();
}

void Simulator::run(std::size_t steps) {
    for (std::size_t i = 0; i < steps; ++i) step();
}

void Simulator::update_macroscopic() {
    for (std::size_t cell = 0; cell < cells_; ++cell) {
        if (solid_[cell]) {
            rho_[cell] = 0.0;
            ux_[cell] = uy_[cell] = uz_[cell] = 0.0;
            continue;
        }
        double rho = 0.0, mx = 0.0, my = 0.0, mz = 0.0;
        for (std::size_t q = 0; q < Q; ++q) {
            const double fq = f_[q * cells_ + cell];
            rho += fq;
            mx += fq * c[q].x;
            my += fq * c[q].y;
            mz += fq * c[q].z;
        }
        rho_[cell] = rho;
        if (rho > 0.0 && std::isfinite(rho)) {
            ux_[cell] = mx / rho;
            uy_[cell] = my / rho;
            uz_[cell] = mz / rho;
        } else {
            ux_[cell] = uy_[cell] = uz_[cell] = 0.0;
        }
    }
}

double Simulator::density(std::size_t x, std::size_t y, std::size_t z) const {
    check_coordinates(x, y, z);
    return rho_[index(x, y, z)];
}

Vec3 Simulator::velocity(std::size_t x, std::size_t y, std::size_t z) const {
    check_coordinates(x, y, z);
    const auto cell = index(x, y, z);
    return {ux_[cell], uy_[cell], uz_[cell]};
}

double Simulator::total_mass() const noexcept {
    double sum = 0.0;
    for (std::size_t cell = 0; cell < cells_; ++cell) {
        if (!solid_[cell]) {
            for (std::size_t q = 0; q < Q; ++q) sum += f_[q * cells_ + cell];
        }
    }
    return sum;
}

}  // namespace lbm
