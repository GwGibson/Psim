#include "psim/compositeSurface.h"
#include "psim/geometry.h"
#include <ranges>
#include <sstream>

class Cell;
class Material;
class Phonon;

using Line = Geometry::Line;

CompositeSurface::CompositeSurface(Surface&& main_surface)
    : main_surface_{ std::move(main_surface) } {
}

void CompositeSurface::updateEmitSurfaceTables() noexcept {
    for (auto& es : emit_sub_surfaces_) { es.updateTable(); }// NOLINT
}

bool CompositeSurface::addEmitSurface(const Line& surface_line,
    Cell& cell,
    const Material& mat,
    double temp,// NOLINT
    int norm_sign,
    double duration,
    double start_time) {
    if (verifySurfaceLine(surface_line)) {
        emit_sub_surfaces_.emplace_back(
            surface_line, cell, main_surface_.getSpecularity(), norm_sign, mat, temp, duration, start_time);
        // Ensure normal is correct
        emit_sub_surfaces_.back().setNormal(main_surface_.getNormal());
        return true;
    }
    return false;
}

bool CompositeSurface::addTransitionSurface(const Line& surface_line, Cell& cell, int norm_sign) {
    if (verifySurfaceLine(surface_line)) {
        // Transition surfaces cause the phonon to diffusely (spec=0) backscatter when they lack a corresponding
        // state in the new material (frequency too high)
        transition_sub_surfaces_.emplace_back(surface_line, cell, 0., norm_sign);
        return true;
    }
    return false;
}

void CompositeSurface::handlePhonon(Phonon& p, const Point& poi, double step_time) const noexcept {// NOLINT
    // Search transitions surfaces first since, in most scenarios, this will be the most likely impact surface
    if (const auto transition_it = std::ranges::find_if(transition_sub_surfaces_,
            [&poi](const auto& t_surface) { return t_surface.getSurfaceLine().contains(poi); });
        transition_it != std::cend(transition_sub_surfaces_)) {
        transition_it->handlePhonon(p);
        return;
    }
    // Check emit surfaces next
    const auto emit_it = std::ranges::find_if(
        emit_sub_surfaces_, [&poi](const auto& e_surface) { return e_surface.getSurfaceLine().contains(poi); });

    if (emit_it != std::cend(emit_sub_surfaces_)) {
        emit_it->handlePhonon(p, step_time);
        return;
    }

    // If the phonon didn't impact a transition or emit surface, it must have hit the main (boundary) surface
    main_surface_.boundaryHandlePhonon(p);
}

bool CompositeSurface::verifySurfaceLine(const Line& surface_line) const {
    const auto& main = main_surface_.getSurfaceLine();
    // If the incoming surface is contained within the primary surface
    if (main.contains(surface_line)) {
        for (const auto& ts : transition_sub_surfaces_) {// NOLINT
            // If the incoming subsurface overlaps with an existing subsurface -> invalid user specifications
            if (const auto& existing = ts.getSurfaceLine(); surface_line.overlaps(existing)) {
                throw CompositeSurfaceError(existing, surface_line);
            }
        }
        for (const auto& es : emit_sub_surfaces_) {// NOLINT
            // If the incoming subsurface overlaps with an existing subsurface -> invalid user specifications
            if (const auto& existing = es.getSurfaceLine(); surface_line.overlaps(existing)) {
                throw CompositeSurfaceError(existing, surface_line);
            }
        }
        return true;
    }
    return false;
}

std::ostream& operator<<(std::ostream& os, const CompositeSurface& surface) {// NOLINT
    os << "Main Surface: " << surface.main_surface_.getSurfaceLine() << '\n';
    for (const auto& ts : surface.transition_sub_surfaces_) {// NOLINT
        os << '\t' << "Transition Surface: " << ts.getSurfaceLine() << '\n';
    }
    for (const auto& es : surface.emit_sub_surfaces_) {// NOLINT
        os << '\t' << "Emit Surface: " << es.getSurfaceLine() << '\n';
    }
    return os;
}

CompositeSurfaceError::CompositeSurfaceError(Line main, Line inc)
    : main_{ std::move(main) }
    , inc_{ std::move(inc) } {
    std::ostringstream os;// NOLINT
    os << "An existing surface conflicts with the location of the incoming surface.\n";
    os << "Existing surface points are: " << main_.p1 << ' ' << main_.p2 << '\n';
    os << "Incoming surface points are: " << inc_.p1 << ' ' << inc_.p2 << '\n';
    setMessage(os.str());
}
