#ifndef PSIM_CELL_H
#define PSIM_CELL_H

#include "compositeSurface.h"// for CompositeSurface
#include "geometry.h"// for Triangle, Line, Point
#include "material.h"// for Material, Material::Table
#include "sensor.h"// for Sensor
#include <array>// for array
#include <cstddef>// for size_t
#include <exception>// for exception
#include <iosfwd>// for ostream
#include <string>// for string
#include <string_view>// for string_view

class Phonon;


// TODO - Inherit from Geometry::Triangle? Composition seems best for Sensor but requires a lot of forwarding
class Cell {
public:
    using Point = Geometry::Point;
    using Line = Geometry::Line;
    using Triangle = Geometry::Triangle;

    Cell(Triangle cell, Sensor& sensor, double spec = 1.);

    [[nodiscard]] const Material& getMaterial() const noexcept {
        return sensor_.getMaterial();
    }
    [[nodiscard]] std::size_t getMaterialID() const noexcept {
        return sensor_.getMaterial().id();
    }
    [[nodiscard]] std::size_t getSensorID() const noexcept {
        return sensor_.getID();
    }
    [[nodiscard]] double getHeatCapacityAtFreq(std::size_t freq_index) const noexcept {
        return sensor_.getHeatCapacityAtFreq(freq_index);
    }
    [[nodiscard]] double getArea() const noexcept {
        return cell_.area();
    }
    [[nodiscard]] double getInitTemp() const noexcept {
        return sensor_.getInitTemp();
    }
    [[nodiscard]] double getSteadyTemp(std::size_t step = 0) const noexcept {
        return sensor_.getSteadyTemp(step);
    }
    [[nodiscard]] const auto& getBoundaries() const noexcept {
        return boundaries_;
    }
    [[nodiscard]] std::array<Line, 3> getBoundaryLines() const noexcept;

    [[nodiscard]] Point getRandPoint(double r1, double r2) const noexcept {// NOLINT
        return cell_.getRandPoint(r1, r2);
    }
    // Throws if the incoming cell overlaps, contains or is contained within this cell - both cells cannot coexist
    // in the same model
    void validate(const Cell& other) const;
    [[nodiscard]] bool setEmitSurface(const Line& line, double temp, double duration, double start_time);

    [[nodiscard]] double getInitEnergy(double t_eq) const noexcept;
    [[nodiscard]] double getEmitEnergy(double t_eq) const noexcept;

    void initialUpdate(Phonon& p, const Material::Table& table) const noexcept {// NOLINT
        return sensor_.initialUpdate(p, table);
    }
    void initialUpdate(Phonon& p) const noexcept {
        return sensor_.initialUpdate(p);// NOLINT
    }
    void scatterUpdate(Phonon& p) const noexcept {
        return sensor_.scatterUpdate(p);// NOLINT
    }
    void updateEmitTables() noexcept;
    void updateHeatParams(const Phonon& p, std::size_t step) noexcept;// NOLINT
    void findTransitionSurface(Cell& other);
    void handleSurfaceCollision(Phonon& p, const Point& poi, double step_time) const noexcept;// NOLINT

    bool operator==(const Cell& rhs) const;
    bool operator!=(const Cell& rhs) const;

    friend std::ostream& operator<<(std::ostream& os, const Cell& cell);// NOLINT

private:
    Triangle cell_;
    Sensor& sensor_;

    // Unused space on each line defaults to a boundary surface and portions of this boundary surface
    // are allocated to different surface types as necessary
    std::array<CompositeSurface, 3> boundaries_;

    // std::vector<BoundarySurface> inner_surfaces_; // implement later if req'd

    [[nodiscard]] std::array<CompositeSurface, 3> buildCompositeSurfaces(double spec) noexcept;
    [[nodiscard]] bool setTransitionSurface(const Line& line, Cell& cell);
};


class CellError : public std::exception {
public:
    [[nodiscard]] const char* what() const noexcept override {
        return message_.c_str();
    }

protected:
    void setMessage(std::string_view message) {
        message_ = message;
    }

private:
    std::string message_;
};

class IntersectError : public CellError {
public:
    IntersectError(Geometry::Triangle existing, Geometry::Triangle incoming);

private:
    Geometry::Triangle existing_;
    Geometry::Triangle incoming_;
};

class OverlapError : public CellError {
public:
    OverlapError(Geometry::Triangle bigger, Geometry::Triangle smaller);

private:
    Geometry::Triangle bigger_;
    Geometry::Triangle smaller_;
};


#endif// PSIM_CELL_H
