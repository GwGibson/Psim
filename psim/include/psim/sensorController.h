#ifndef PSIM_SENSORCONTROLLER_H
#define PSIM_SENSORCONTROLLER_H

#include "material.h"// for Material, Material::Table
#include <cstddef>// for size_t
#include <vector>// for vector

class Phonon;

// Abstract class is perhaps not necessary here
class SensorController {
public:
    SensorController(const Material& material, double t_init, std::size_t num_measurements = 0);
    virtual ~SensorController() = default;
    SensorController(const SensorController&) = default;
    SensorController(SensorController&&) noexcept = default;
    SensorController& operator=(const SensorController&) = delete;
    SensorController& operator=(SensorController&&) noexcept = delete;

    [[nodiscard]] const Material& getMaterial() const noexcept {
        return material_;
    }
    [[nodiscard]] double getHeatCapacityAtFreq(std::size_t freq_index) const noexcept;
    [[nodiscard]] virtual double getHeatCapacity(std::size_t step) const noexcept = 0;
    [[nodiscard]] virtual double getInitTemp() const noexcept = 0;
    [[nodiscard]] virtual double getSteadyTemp(std::size_t step) const noexcept = 0;

    void initialUpdate(Phonon& p, const Material::Table& table) const noexcept;// NOLINT
    void initialUpdate(Phonon& p) const noexcept;// NOLINT
    virtual void updateTables();
    virtual void scatterUpdate(Phonon& p) const noexcept;// NOLINT
    /**
     * Returns true if the input sensor's temperature has not significantly changed over the course of the simulation
     * @param t_final - The temperature at the end of the run
     * @param final_temps - A vector of temperature from the previous measurement steps -> for transient simulations
     * @return - true if the temp of this sensor is unstable (final temp not within some percentage of initial temp)
     */
    [[nodiscard]] virtual bool resetRequired(double t_final, std::vector<double>&&) noexcept;
    virtual void reset() noexcept = 0;

protected:
    const Material& material_;
    double t_init_;
    std::size_t num_measurements_;// For transient surfaces only

    double t_steady_{ 0. };// Steady state temperature of the cell. Used to set the energy tables & heat_capacity_
    double heat_capacity_{ 0. };// Energy per unit volume in full simulations - heat capacity in deviational simulations
    const Material::Table* base_table_{ nullptr };
    const Material::Table* scatter_table_{ nullptr };

    // Transient sensor containers -> not needed for steady state or periodic simulations
    std::vector<const Material::Table*>
        scatter_tables_;// Transient controllers need a scatter table for each measurement step
    std::vector<double> heat_capacities_;
    std::vector<double> steady_temps_;
};

class SteadyStateController : public SensorController {
public:
    using SensorController::SensorController;

    [[nodiscard]] double getHeatCapacity([[maybe_unused]] std::size_t step) const noexcept override {
        return heat_capacity_;
    }
    // This is not a bug -> Init temp for next iteration is the steady temp of the previous iteration
    // Not interested in periodic progression of the system here, will converge faster this way
    [[nodiscard]] double getInitTemp() const noexcept override {
        return t_steady_;
    }
    [[nodiscard]] double getSteadyTemp([[maybe_unused]] std::size_t step) const noexcept override {
        return t_steady_;
    }

    void reset() noexcept override;
};

class PeriodicController : public SensorController {
public:
    using SensorController::SensorController;

    [[nodiscard]] double getHeatCapacity([[maybe_unused]] std::size_t step) const noexcept override {
        return heat_capacity_;
    }
    // Need to restore sensors to their initial temperatures to see the periodic progression of the system
    [[nodiscard]] double getInitTemp() const noexcept override {
        return t_init_;
    }
    [[nodiscard]] double getSteadyTemp([[maybe_unused]] std::size_t step) const noexcept override {
        return t_steady_;
    }

    void reset() noexcept override;
};

// May want to average results of final 10% of runs or something like this
class TransientController : public SensorController {
public:
    using SensorController::SensorController;

    [[nodiscard]] double getHeatCapacity(std::size_t step) const noexcept override {
        return heat_capacities_[step];
    }
    [[nodiscard]] double getInitTemp() const noexcept override {
        return t_init_;
    }
    // 0 if no step and steady_temps_[step] if step specified (t_eq update pointless)
    [[nodiscard]] double getSteadyTemp(std::size_t step) const noexcept override;

    void scatterUpdate(Phonon& p) const noexcept override;// NOLINT
    [[nodiscard]] bool resetRequired([[maybe_unused]] double t_final,
        std::vector<double>&& final_temps) noexcept override;
    void reset() noexcept override;
};

#endif// PSIM_SENSORCONTROLLER_H
