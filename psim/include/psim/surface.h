#ifndef PSIM_SURFACE_H
#define PSIM_SURFACE_H

#include "geometry.h"
#include "material.h"

class Phonon;
class Cell;

// Intentional design choice to avoid virtual functions due to 'requiring' dummy variables and dummy functions.
// This will greatly hinder the ability to integrate new surface types, but I don't see there being any need
// for additional surface types.
// TODO - Inherit from Geometry::Line
class Surface {
public:
    using Vector2D = Geometry::Vector2D;
    using Point = Geometry::Point;
    using Line = Geometry::Line;

    Surface(Line surface_line, Cell& cell, double specularity, int norm_sign);
    virtual ~Surface() = default;
    Surface(const Surface&) = default;
    Surface(Surface&&) noexcept = default;
    Surface& operator=(const Surface&) = delete;
    Surface& operator=(Surface&&) = delete;

    void boundaryHandlePhonon(Phonon& p) const noexcept;// NOLINT
    void redirectPhonon(Phonon& p) const noexcept;// NOLINT

    [[nodiscard]] const Line& getSurfaceLine() const noexcept {
        return surface_line_;
    }
    [[nodiscard]] bool contains(const Point& point) const noexcept {
        return surface_line_.contains(point);
    }
    [[nodiscard]] Point getRandPoint(double r1) const noexcept {
        return surface_line_.getRandPoint(r1);
    }// NOLINT
    [[nodiscard]] double getLength() const noexcept {
        return surface_line_.length;
    }
    [[nodiscard]] double getSpecularity() const noexcept {
        return specularity_;
    }

    [[nodiscard]] Vector2D getNormal() const noexcept {
        return normal_;
    }
    void setNormal(Vector2D normal) noexcept {
        normal_ = normal;
    }

    bool operator==(const Surface& rhs) const {
        return (surface_line_ == rhs.surface_line_);
    }
    bool operator>(const Surface& rhs) const {
        return surface_line_ > rhs.surface_line_;
    }

private:
    const Line surface_line_;

protected:
    Cell& cell_;
    Vector2D normal_;
    const double specularity_;
};

// While an emitting surface is not emitting phonons (transient case) it acts as a boundary surface
class EmitSurface : public Surface {
public:
    EmitSurface(Line surface_line,
        Cell& cell,
        double specularity,
        int norm_sign,
        const Material& mat,
        double temp,
        double duration,
        double start_time);
    void handlePhonon(Phonon& p, double step_time) const noexcept;// NOLINT
    [[nodiscard]] double getEmitDuration() const noexcept {
        return duration_;
    }
    [[nodiscard]] double getTemp() const noexcept {
        return temp_;
    }
    [[nodiscard]] const Material::Table& getTable() const noexcept {
        return *emit_table_;
    }
    [[nodiscard]] double getPhononTime() const noexcept;
    void updateTable() {
        emit_table_ = material_.emitTable(temp_);
    }

protected:
    const Material& material_;
    double temp_;
    const Material::Table* emit_table_{ nullptr };
    double duration_;
    double start_time_;
};

// Can act as a boundary surface during material interface interactions
class TransitionSurface : public Surface {
public:
    using Surface::Surface;
    void handlePhonon(Phonon& p) const noexcept;// NOLINT
};

#endif// PSIM_SURFACE_H
