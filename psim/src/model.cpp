#include "psim/model.h"
#include "psim/cell.h"// for Cell
#include "psim/compositeSurface.h"// for CompositeSurface
#include "psim/material.h"// for Material
#include "psim/modelSimulator.h"// for ModelSimulator
#include "psim/outputManager.h"// for OutputManager
#include "psim/sensorInterpreter.h"// for SensorInterpreter
#include "psim/surface.h"// for EmitSurface
#include <algorithm>// for find_if, max, minmax_element
#include <array>// for array
#include <cmath>// for fabs
#include <execution>// for for_each, seq, par
#include <functional>// for plus
#include <iostream>// for operator<<, cout, ostream, basic...
#include <iterator>// for end, cbegin, cend, begin
#include <new>// for bad_alloc
#include <stdexcept>// for runtime_error
#include <tuple>// for tie, tuple
#include <type_traits>// for enable_if_t

namespace {
// Maximum number of simulations resets
constexpr std::size_t MAX_ITERS{ 5 };
// Threshold (percentage) of sensors that must be stable for the system to be considered stable
constexpr std::size_t RESET_THRESHOLD{ 90 };
// Threshold (percentage*1000) that t_eq must be within between runs for the system to be considered stable
// 5 means t_eq must be within 0.5% of previous t_eq
constexpr std::size_t TEQ_THRESHOLD{ 5 };
// Epsilon on the temperature bounds that are to be used when using a numerical inversion to calculate
// the temperature of each sensor based on the energy associated with them
constexpr double TEMP_BOUND_EPS{ 10. };
constexpr double PHASOR_TEMP_BOUND_EPS{ TEMP_BOUND_EPS * 100. };
// Percentage of measurement steps that will be used for steady state calculations
constexpr double SS_STEPS_PERCENT{ 0.1 };
// Temperature intervals to create distribution tables. A smaller interval may increase precision
// but will take up more memory. Primarily a concern for transient simulations
constexpr float TEMP_INTERVAL{ 1. };
}// namespace

using Point = Geometry::Point;
using Line = Geometry::Line;

Model::Model(std::size_t num_cells,// NOLINT
    std::size_t num_sensors,
    std::size_t measurement_steps,
    std::size_t num_phonons,
    double simulation_time,// NOLINT
    double t_eq,
    bool phasor_sim)
    : num_cells_{ num_cells }
    , measurement_steps_{ measurement_steps }
    , simulation_time_{ simulation_time }
    , num_phonons_{ num_phonons }
    , t_eq_{ t_eq }
    , phasor_sim_{ phasor_sim }
    , simulator_{ measurement_steps, simulation_time, phasor_sim }
    , interpreter_{}
    , addMeasurementMutex_{ std::make_unique<std::mutex>() } {
    cells_.reserve(num_cells);
    sensors_.reserve(num_sensors);
}

void Model::setSimulationType(SimulationType type, std::size_t step_interval) {
    sim_type_ = type;
    // Throwing here so the user doesn't run a full simulation only to realize after that they
    // are using the incorrect settings.
    if (type == SimulationType::Transient || type == SimulationType::Periodic) {
        start_step_ = static_cast<std::size_t>(static_cast<double>(measurement_steps_)
                                               - static_cast<double>(measurement_steps_) * SS_STEPS_PERCENT);// NOLINT
        if (step_interval == 0) {
            throw std::runtime_error(
                std::string("Step interval of 0 is invalid for transient and periodic simulations.\n"));
        }
    }
    if ((type == SimulationType::Transient && t_eq_ == 0)) {
        throw std::runtime_error(std::string("Transient simulations must be run using the deviational approach.\n"));
    }
    if (type == SimulationType::SteadyState) {
        simulator_.setStepAdjustment(
            static_cast<std::size_t>(static_cast<double>(measurement_steps_)
                                     - static_cast<double>(measurement_steps_) * SS_STEPS_PERCENT));// NOLINT
        if (step_interval > 0) {
            throw std::runtime_error(std::string("Step interval > 0 is invalid for steady-state simulations\n"));
        }
    }
    outputManager_.setStepInterval(step_interval);
}

void Model::addMaterial(const std::string& material_name, const Material& material) {
    const auto exists = materials_.find(material_name);
    if (exists != std::end(materials_)) {
        throw std::runtime_error(std::string("A duplicate material name was detected.\n"));
    }
    materials_.emplace(material_name, material);
}

// Assumes the sensor material exists in the materials_ map
void Model::addSensor(std::size_t ID,// NOLINT
    const std::string& material_name,
    double t_init,
    Sensor::SimulationType type) {
    auto sensor = std::find_if(std::begin(sensors_), std::end(sensors_), [&ID](auto& s) {// NOLINT
        return s.getID() == ID;
    });
    if (sensor == std::end(sensors_)) {
        const std::size_t steps_to_record = (type == Sensor::SimulationType::SteadyState) ? static_cast<std::size_t>(
                                                static_cast<double>(measurement_steps_) * SS_STEPS_PERCENT)
                                                                                          : measurement_steps_;// NOLINT
        sensors_.emplace_back(ID, materials_.at(material_name), type, steps_to_record, t_init);
    } else {
        throw std::runtime_error(std::string("Sensor with this ID already exists\n"));
    }
}

void Model::addCell(Geometry::Triangle&& triangle, std::size_t sensor_ID, double spec) {
    if (cells_.size() >= num_cells_) { throw std::runtime_error(std::string("Too many cells\n")); }
    cells_.emplace_back(triangle, getSensor(sensor_ID), spec);
    Cell& inc_cell = cells_.back();
    std::size_t identical_cells = 0;// Should only be 1 identical cell (check against itself)
    for (auto& cell : cells_) {
        if (inc_cell != cell) {
            // Throws if the incoming cell is incompatible with any existing cells
            // TODO: Misses cases when all 3 points of a cell are on the edges of another cell
            inc_cell.validate(cell);
            inc_cell.findTransitionSurface(cell);
        } else if (++identical_cells == 2) {// else { ++identical_cells; if (identical_cells == 2) { throw } }
            throw std::runtime_error(std::string("Duplicate cell detected.\n"));
        }
    }
}

// Not used if the python interface is used
void Model::addCell(Point&& p1, Point&& p2, std::size_t sensor_ID, double spec) {// NOLINT
    if (p1.x == p2.x || p1.y == p2.y) {
        throw std::runtime_error(std::string("These points do not specify a rectangle\n"));
    }
    addCell(Geometry::Triangle{ p1, Point(p1.x, p2.y), Point(p2.x, p1.y) }, sensor_ID, spec);
    addCell(Geometry::Triangle{ p2, Point(p2.x, p1.y), Point(p1.x, p2.y) }, sensor_ID, spec);
}

// TODO: Fix emit surface placement failing when incoming surface is not exact in rare circumstances
bool Model::setEmitSurface(const Point& p1, const Point& p2, double temp, double duration, double start_time) {// NOLINT

    if ((start_time < 0. || start_time >= simulation_time_)
        || (duration < 0. || duration > simulation_time_ - start_time)) {
        throw std::runtime_error(std::string("Transient Surface start_time or duration specifications are invalid.\n"));
    }
    if ((start_time > 0. || duration < simulation_time_) && sim_type_ != SimulationType::Transient) {
        throw std::runtime_error(std::string("Cannot add a transient surface to a non transient simulation.\n"));
    }
    for (auto& cell : cells_) {
        if (cell.setEmitSurface(Line{ p1, p2 }, temp, duration, start_time)) { return true; }
    }
    return false;
}

void Model::runSimulation() {
    // TODO: could run checks here to verify there is at least 1 sensor/cell etc.
    const auto [min, max] = setTemperatureBounds();
    initializeMaterialTables(min, max);

    auto refresh = [this]() {
        auto total_energy = getTotalInitialEnergy();
        double energy_per_phonon = total_energy / static_cast<double>(num_phonons_);// NOLINT
        interpreter_.setParams(t_eq_, energy_per_phonon);
        return std::pair{ total_energy, energy_per_phonon };
    };

    auto [total_energy, energy_per_phonon] = refresh();

    std::size_t iter = 0;
    bool reset_required = true;
    while (reset_required && ++iter <= MAX_ITERS) {
        simulator_.initPhononBuilders(cells_, t_eq_, energy_per_phonon);
        simulator_.runSimulation(t_eq_);
        // Check if sensor temperatures are stable
        if (const auto new_t_eq = resetRequired(); new_t_eq && iter < MAX_ITERS && !phasor_sim_) {
            reset();
            t_eq_ = *new_t_eq;
            std::cout << "system not stable\n";
            std::cout << "updated t_eq: " << t_eq_ << '\n';
        } else {
            reset_required = false;
        }
        std::tie(total_energy, energy_per_phonon) = refresh();
    }
    if (iter >= MAX_ITERS) { std::cout << "System did not stabilize!!\n"; }// Should log this or include in output title
    storeResults();
}

void Model::exportResults(const fs::path& filepath, double time) const {
    outputManager_.steadyStateExport(filepath, time);
    // If transient or periodic simulation -> do periodic export (This is useless for ss simulation that require resets)
    if (sim_type_ != SimulationType::SteadyState) { outputManager_.periodicExport(filepath, time); }
}

Sensor& Model::getSensor(std::size_t ID) {// NOLINT
    auto sensor = std::find_if(std::begin(sensors_), std::end(sensors_), [&ID](auto& s) {// NOLINT
        return s.getID() == ID;
    });
    if (sensor == std::end(sensors_)) { throw std::runtime_error(std::string("Sensor does not exist\n")); }
    return *sensor;
}

double Model::getTotalInitialEnergy() const noexcept {
    return std::transform_reduce(
        std::execution::seq, std::cbegin(cells_), std::cend(cells_), 0., std::plus{}, [&](const auto& cell) {
            return cell.getInitEnergy(t_eq_) + cell.getEmitEnergy(t_eq_);
        });
}

std::pair<double, double> Model::setTemperatureBounds() noexcept {
    std::vector<double> temperatures;
    for (const auto& cell : cells_) {
        temperatures.push_back(cell.getInitTemp());
        for (const auto& surface : cell.getBoundaries()) {
            const auto& emit_surfaces = surface.getEmitSurfaces();
            std::transform(std::cbegin(emit_surfaces),
                std::cend(emit_surfaces),
                std::back_inserter(temperatures),
                [](const auto& emit_surface) { return emit_surface.getTemp(); });
        }
    }
    const auto [min, max] = std::minmax_element(std::cbegin(temperatures), std::cend(temperatures));
    const auto bound = (phasor_sim_) ? PHASOR_TEMP_BOUND_EPS : TEMP_BOUND_EPS;
    interpreter_.setBounds(std::max(*min - bound, 0.), *max + bound);// NOLINT
    return { *min, *max };
}


void Model::initializeMaterialTables(double low_temp, double high_temp) {
    for (auto& element : materials_) { element.second.initializeTables(low_temp, high_temp, TEMP_INTERVAL); }
    for (auto& sensor : sensors_) { sensor.updateTables(); }
    for (auto& cell : cells_) { cell.updateEmitTables(); }
}


double Model::avgTemp() const {
    const double total_area = std::transform_reduce(
        std::execution::seq, std::cbegin(sensors_), std::cend(sensors_), 0., std::plus{}, [](const auto& sensor) {
            return sensor.getArea();
        });
    return std::transform_reduce(
        std::execution::seq, std::cbegin(sensors_), std::cend(sensors_), 0., std::plus{}, [&](const auto& sensor) {
            return sensor.getSteadyTemp() * sensor.getArea() / total_area;
        });
}

void Model::storeResults() noexcept {
    std::for_each(std::execution::par, std::cbegin(sensors_), std::cend(sensors_), [&](const auto& sensor) {
        auto measurement = interpreter_.scaleHeatParams(sensor);
        std::scoped_lock lg(*addMeasurementMutex_);// NOLINT
        outputManager_.addMeasurement(std::move(measurement));
    });
    outputManager_.sortMeasurements();
}

std::optional<double> Model::resetRequired() noexcept {
    auto t_diff = [](const auto& t_final, const auto& t_init) {
        return std::fabs(t_final - t_init) / t_init * 1000 > TEQ_THRESHOLD;// NOLINT
    };

    const std::size_t total_sensors = sensors_.size();
    std::size_t stable_sensors = 0;
    for (auto& sensor : sensors_) {
        if (sim_type_ != SimulationType::Transient) {
            // The average temperature of the last 10% of measurement steps
            if (sensor.resetRequired(interpreter_.getFinalTemp(sensor, start_step_))) { ++stable_sensors; }
        } else {
            // Get the temperature at each measurement step
            if (sensor.resetRequired(0., interpreter_.getFinalTemps(sensor))) { ++stable_sensors; }
        }
    }
    std::cout << "Stable sensors: " << stable_sensors << '\n';
    const auto new_t_eq = (t_eq_ == 0. || sim_type_ == SimulationType::Transient) ? t_eq_ : avgTemp();
    // if either the sensors or t_eq are not stable, return true indicated a reset is required
    return (stable_sensors * 100 / total_sensors < RESET_THRESHOLD) || t_diff(new_t_eq, t_eq_)// NOLINT
               ? std::make_optional(new_t_eq)
               : std::nullopt;
}

void Model::reset() noexcept {
    simulator_.reset();
    if (sim_type_ == SimulationType::Transient) {
        for_each(std::execution::par, std::begin(sensors_), std::end(sensors_), [](auto& sensor) { sensor.reset(); });
    } else {
        for (auto& sensor : sensors_) { sensor.reset(); }
    }
}
