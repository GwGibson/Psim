#ifndef PSIM_COMPOSITESURFACE_H
#define PSIM_COMPOSITESURFACE_H

#include "surface.h"

class Cell;
class Material;
class Phonon;

class CompositeSurface {
public:
    using Point = Geometry::Point;
    using Line = Geometry::Line;

    explicit CompositeSurface(Surface&& main_surface);

    [[nodiscard]] const Line& getSurfaceLine() const noexcept {
        return main_surface_.getSurfaceLine();
    }
    [[nodiscard]] bool contains(const Point& point) const noexcept {
        return main_surface_.contains(point);
    }
    [[nodiscard]] const auto& getEmitSurfaces() const noexcept {
        return emit_sub_surfaces_;
    }
    void updateEmitSurfaceTables() noexcept;

    [[nodiscard]] bool addEmitSurface(const Line&,
        Cell&,
        const Material&,
        double temp,
        int norm_sign,
        double duration,
        double start_time);
    [[nodiscard]] bool addTransitionSurface(const Line& surface_line, Cell& cell, int norm_sign);

    void handlePhonon(Phonon& p, const Point& poi, double step_time) const noexcept;// NOLINT

    friend std::ostream& operator<<(std::ostream& os, const CompositeSurface& surface);// NOLINT

private:
    Surface main_surface_;
    // TODO: std::variant<EmitSurface, TransitionSurface> seems like it would be good here
    // Don't see a compelling reason to use a heterogeneous container here.
    // Number of sub-surfaces should not increase in the foreseeable future.
    std::vector<TransitionSurface> transition_sub_surfaces_;
    std::vector<EmitSurface> emit_sub_surfaces_;
    /**
     * Verifies the incoming surface can be placed at the input location.
     * Throws if there is already a transition or emitting surface overlapping the incoming surface location.
     * @param surface_line - The location of the incoming surface
     * @return True if the surface can be placed at the given location and false if the incoming surface is not fully
     * contained in this surface
     */
    [[nodiscard]] bool verifySurfaceLine(const Line& surface_line) const;// can throw
};

class CompositeSurfaceError : public std::exception {
public:
    CompositeSurfaceError(Geometry::Line main, Geometry::Line inc);
    [[nodiscard]] const char* what() const noexcept override {
        return message_.c_str();
    }

protected:
    void setMessage(std::string_view message) {
        message_ = message;
    }

private:
    std::string message_;
    Geometry::Line main_;
    Geometry::Line inc_;
};

#endif// PSIM_COMPOSITESURFACE_H
