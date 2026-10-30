#include "lbm/simulator.hpp"
#include "vtk_writer.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

namespace {

std::size_t parse_size(const char* text, const char* name) {
    const std::string_view value(text);
    std::size_t parsed = 0;
    const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed, 10);
    if (value.empty() || result.ec != std::errc{} ||
        result.ptr != value.data() + value.size() || parsed == 0) {
        throw std::invalid_argument(std::string(name) +
                                    " must be a positive decimal integer that fits in size_t");
    }
    return parsed;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        if (argc > 6) {
            std::cerr << "Usage: lbm_demo [nx ny nz steps output.vtk]\n";
            return 2;
        }
        const std::size_t nx = argc > 1 ? parse_size(argv[1], "nx") : 32;
        const std::size_t ny = argc > 2 ? parse_size(argv[2], "ny") : 20;
        const std::size_t nz = argc > 3 ? parse_size(argv[3], "nz") : 20;
        const std::size_t steps = argc > 4 ? parse_size(argv[4], "steps") : 100;
        const std::string output = argc > 5 ? argv[5] : "lbm_output.vtk";

        lbm::Simulator simulation(nx, ny, nz, 0.8);
        simulation.initialize_equilibrium(1.0, {0.04, 0.0, 0.0});

        // A small stationary spherical obstacle. The outer domain is periodic.
        const double cx = static_cast<double>(nx) * 0.30;
        const double cy = (static_cast<double>(ny) - 1.0) * 0.5;
        const double cz = (static_cast<double>(nz) - 1.0) * 0.5;
        const double radius = std::max(1.0, static_cast<double>(std::min({nx, ny, nz})) / 8.0);
        for (std::size_t z = 0; z < nz; ++z)
            for (std::size_t y = 0; y < ny; ++y)
                for (std::size_t x = 0; x < nx; ++x) {
                    const double dx = static_cast<double>(x) - cx;
                    const double dy = static_cast<double>(y) - cy;
                    const double dz = static_cast<double>(z) - cz;
                    if (dx * dx + dy * dy + dz * dz <= radius * radius)
                        simulation.set_solid(x, y, z);
                }

        const double initial_mass = simulation.total_mass();
        simulation.run(steps);
        lbm::write_vtk_structured_points(simulation, output);
        std::cout << "Completed " << steps << " steps on " << nx << 'x' << ny << 'x' << nz
                  << " grid. Fluid mass: " << simulation.total_mass()
                  << " (initial " << initial_mass << ")\nWrote " << output << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "lbm_demo: " << error.what() << '\n';
        return 1;
    }
}
