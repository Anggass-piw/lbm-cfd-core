# Lattice Boltzmann CFD Core (MVP)

A small, dependency-free C++17 CPU implementation of a three-dimensional D3Q19 lattice Boltzmann solver. It is intended as an educational and extensible starting point, not as a production CFD package.

## Included

- Standard D3Q19 discrete velocities, weights, and opposite-direction table.
- Direction-major (structure-of-arrays) population storage on a 3-D Cartesian grid.
- Single-relaxation-time BGK collision and pull streaming.
- Periodic outer boundaries and link-wise halfway bounce-back at user-marked solid cells.
- Macroscopic density and velocity fields, fluid mass reporting, and legacy ASCII VTK `STRUCTURED_POINTS` output.
- A small obstacle-flow demo and dependency-free CTest coverage.

## Build and test

Requirements: CMake 3.16 or newer and a C++17 compiler. No third-party libraries are needed.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Run the sample

```sh
./build/lbm_demo
```

The default run advances a 32 × 20 × 20 lattice for 100 time steps with a small spherical solid and writes `lbm_output.vtk` to the current directory. To choose dimensions, step count, and output path:

```sh
./build/lbm_demo 48 24 24 250 result.vtk
```

The numeric arguments (`nx`, `ny`, `nz`, and `steps`) must be positive base-10 integers without a sign or surrounding whitespace; zero and values that do not fit in `size_t` are rejected.

The VTK file can be opened in ParaView or another reader supporting legacy VTK structured points. Density and velocity are point fields; solid cells are written with zero macroscopic values.

## Library sketch

```cpp
#include "lbm/simulator.hpp"
#include "vtk_writer.hpp"

lbm::Simulator solver(32, 20, 20, 0.8); // tau must be > 0.5
solver.initialize_equilibrium(1.0, {0.04, 0.0, 0.0});
solver.set_solid(10, 10, 10);
solver.run(100);
lbm::write_vtk_structured_points(solver, "result.vtk");
```

`lbm::Simulator` is declared in the public `include/lbm` headers. The optional VTK writer is declared in `src/vtk_writer.hpp` in this MVP; consumers can also use the library without it. Coordinates are zero-based. Setting a cell solid excludes it from fluid mass and macroscopic output. Call `set_equilibrium` to initialize individual cells.

## MVP limitations

- All outer faces are periodic; there are no inlet/outlet, pressure, or velocity boundary-condition models.
- BGK uses a single relaxation time. No MRT, LES, turbulence model, or advanced collision operator is implemented.
- The implementation is scalar CPU code. It does not claim AVX-512 or other SIMD acceleration.
- Bounce-back is a basic link-wise treatment for stationary solids; curved-boundary accuracy and moving walls are not addressed.
- This is not validated as production CFD. Stability and accuracy depend on lattice resolution, relaxation time, and low-Mach-number assumptions.
- The demo is intentionally small and has no configuration-file, checkpoint/restart, or performance-tuning system.

## Roadmap ideas

1. Add explicit inlet/outlet and wall boundary-condition schemes with dedicated tests.
2. Add configurable initial fields, geometry import, and checkpoint/restart.
3. Measure and document performance before introducing optional parallelism or SIMD.
4. Add validation cases (e.g. Poiseuille flow and lid-driven cavity) and compare against analytic/reference results.
5. Consider alternative collision models and richer VTK output after validation foundations are established.
