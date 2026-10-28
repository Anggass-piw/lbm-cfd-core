#pragma once

#include "lbm/simulator.hpp"

#include <string>

namespace lbm {

// Write legacy ASCII VTK STRUCTURED_POINTS with point scalars (density) and
// point vectors (velocity), suitable for ParaView and other VTK readers.
void write_vtk_structured_points(const Simulator& simulator, const std::string& path);

}  // namespace lbm
