#include "psim/surface.h"
#include "psim/cell.h"
#include "psim/geometry.h"
#include "psim/phonon.h"
#include "psim/utils.h"

using Point = Geometry::Point;
using Polar = Phonon::Polarization;
using Utils::urand;

Surface::Surface(Line surface_line, Cell& cell, double specularity, int norm_sign)// NOLINT
    : surface_line_{ std::move(surface_line) }
    , cell_{ cell }
    , normal_{ surface_line_.normal(norm_sign) }
    , specularity_{ specularity } {
}

/**
 * Redirects the incoming phonon to a random direction
 * that points away from the impacted surface (Diffuse scatter)
 * @param p - The phonon that interacts with the surface
 */
void Surface::redirectPhonon(Phonon& p) const noexcept {// NOLINT
    // cppcheck-suppress unassignedVariable
    const auto& [nx, ny] = normal_;
    const auto rand = urand();
    const auto new_dx = sqrt(rand);
    const auto new_dy = sqrt(1. - rand) * cos(2. * PI * urand());
    p.setDirection(nx * new_dx - ny * new_dy, ny * new_dx + nx * new_dy);
}

void Surface::boundaryHandlePhonon(Phonon& p) const noexcept {// NOLINT
    if (specularity_ == 1. || urand() < specularity_) {// Reflective Scatter
        // cppcheck-suppress unassignedVariable
        const auto& [nx, ny] = normal_;
        // cppcheck-suppress unassignedVariable
        const auto& [dx, dy] = p.getDirection();
        const auto new_dx = -dx * nx - dy * ny;
        const auto new_dy = -dx * ny + dy * nx;
        p.setDirection(nx * new_dx - ny * new_dy, ny * new_dx + nx * new_dy);
    } else {// Diffuse Scatters
        redirectPhonon(p);
    }
}

EmitSurface::EmitSurface(Line surface_line,
    Cell& cell,
    double specularity,
    int norm_sign,
    const Material& mat,
    double temp,// NOLINT
    double duration,
    double start_time)// NOLINT
    : Surface{ std::move(surface_line), cell, specularity, norm_sign }
    , material_{ mat }
    , temp_{ temp }
    , duration_{ duration }
    , start_time_{ start_time } {
}

void EmitSurface::handlePhonon(Phonon& p, double step_time) const noexcept {// NOLINT
    const auto phonon_time = static_cast<double>(p.getLifeStep()) * step_time;
    (phonon_time < start_time_ || phonon_time + step_time > start_time_ + duration_) ? boundaryHandlePhonon(p)
                                                                                     : p.setCell(nullptr);
}

double EmitSurface::getPhononTime() const noexcept {
    return start_time_ + duration_ * urand();
}

void TransitionSurface::handlePhonon(Phonon& p) const noexcept {// NOLINT
    // Material is the same between sensor areas
    if (p.getCellMaterialID() == cell_.getMaterialID()) {
        p.setCell(&cell_);
    } else {// Phonon is passing from one material to another
        const auto& material = cell_.getMaterial();
        // Find maximum frequency allowable in the new material
        const auto max_freq = [&p, &material]() {
            switch (p.getPolar()) {
            case Polar::LA:
                return material.max_freq_la();
            case Polar::TA:
                return material.max_freq_ta();
            default:
                return std::max(material.max_freq_la(), material.max_freq_ta());
            }
        }();
        // TODO: Finish
        // Lazy evaluate whether the phonon is transmitted
        /*
        auto isTransmitted = [this](const Phonon& p){
            const auto freq_index = p.getFreqIndex();
            const auto c1 = cell_.getHeatCapacityAtFreq(freq_index);
            const auto c2 = p.getCellHeatCapacityAtFreq(freq_index);
            return false;
        };
        */
        // Transition surfaces cause the phonon to diffusely (spec=0) backscatter when they lack a corresponding
        // state in the new material (frequency too high)
        if (p.getFreq() > max_freq) {// Or transmission fails
            redirectPhonon(p);// Backscatters into the same cell
        } else {
            // TODO: if phonon cell can pass into new material -> work to do
            // Material interface - hard part

            p.setCell(&cell_);
        }
    }
}
