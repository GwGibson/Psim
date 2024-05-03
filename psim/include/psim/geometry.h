#ifndef PSIM_GEOMETRY_H
#define PSIM_GEOMETRY_H

#include <array>
#include <exception>
#include <limits>
#include <optional>
#include <ostream>
#include <string>
#include <utility>


namespace Geometry {

inline constexpr double GEOEPS = std::numeric_limits<double>::epsilon() * 1E9;
struct Point;
using PointPair = std::pair<Point, Point>;

struct Vector2D {
    double x;
    double y;
};

struct Point {
    double x;
    double y;
};

bool operator==(const Point& lhs, const Point& rhs);
bool operator!=(const Point& lhs, const Point& rhs);
std::ostream& operator<<(std::ostream& os, const Point& point);// NOLINT
Point operator-(const Point& lhs, const Point& rhs);

struct Line {
    Line(Point pt1, Point pt2);
    Point p1;
    Point p2;

    // maybe only store the length and bounding box
    double slope;
    double intercept;
    std::pair<Point, Point> boundingBox;
    double length;

    [[nodiscard]] std::pair<Point, Point> getPoints() const noexcept {
        return { p1, p2 };
    }

    [[nodiscard]] bool overlaps(const Line& other) const noexcept;
    [[nodiscard]] bool contains(const Line& other) const noexcept;
    [[nodiscard]] bool contains(const Point& point) const noexcept;
    [[nodiscard]] bool intersects(const Line& other) const noexcept;
    // norm_sign controls the direction of the normal vector. It can be used
    // to ensure the normal vector always points into a surface.
    // Should be 1 if the polygon is constructed in a clockwise fashion and -1 otherwise
    [[nodiscard]] Vector2D normal(int norm_sign = 1) const noexcept;
    // Returns the point of intersection if this line intersects with the input line.
    // Return std::nullopt if the lines do not intersect
    [[nodiscard]] std::optional<Point> getIntersection(const Line& other) const noexcept;
    [[nodiscard]] Point getRandPoint(double r1) const noexcept;// NOLINT

    bool operator==(const Line& rhs) const {
        return p1 == rhs.p1 && p2 == rhs.p2;
    }
    bool operator>(const Line& rhs) const {
        return length > rhs.length;
    }
};

std::ostream& operator<<(std::ostream& os, const Line& line);// NOLINT

struct Triangle {
    Triangle(Point pt1, Point pt2, Point pt3);
    Point p1;
    Point p2;
    Point p3;

    [[nodiscard]] std::array<Line, 3> lines() const noexcept;

    [[nodiscard]] double area() const noexcept;
    [[nodiscard]] bool intersects(const Triangle& other) const noexcept;
    // Returns true if any point of the incoming triangle is contained in this triangle
    [[nodiscard]] bool contains(const Triangle& other) const noexcept;
    // Return false if the point is on an edge of the triangle
    [[nodiscard]] bool contains(const Point& p) const noexcept;// NOLINT
    [[nodiscard]] bool isClockwise() const noexcept;
    [[nodiscard]] Point getRandPoint(double r1, double r2) const noexcept;// NOLINT

    bool operator==(const Triangle& rhs) const;
};
std::ostream& operator<<(std::ostream& os, const Triangle& triangle);// NOLINT

/**
 * @brief Check if two floating-point values are approximately equal within a specified tolerance.
 *
 * @param a The first value to compare.
 * @param b The second value to compare.
 * @param epsilon The tolerance used for the comparison (default: EPSILON).
 * @return true if the values are approximately equal within the given tolerance, false otherwise.
 */
[[nodiscard]] inline bool approxEqual(double a, double b, double epsilon = GEOEPS) noexcept {// NOLINT
    return std::abs(a - b) < epsilon;
}

class ShapeError : public std::exception {
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

class LineError : public ShapeError {
public:
    explicit LineError(Line line);

private:
    Line line_;
};

class TriangleError : public ShapeError {
public:
    explicit TriangleError(const Geometry::Triangle& triangle);

private:
    Triangle triangle_;
};

}// namespace Geometry

#endif// PSIM_GEOMETRY_H
