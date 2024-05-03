#ifndef PSIM_MODELSIMULATOR_H
#define PSIM_MODELSIMULATOR_H

#include "phononBuilder.h"
#include <optional>
#include <variant>
#include <vector>

class Cell;
class Phonon;

class ModelSimulator {
public:
    using BuilderObj = std::variant<CellOriginBuilder, SurfaceOriginBuilder, PhasorBuilder>;
    ModelSimulator(std::size_t measurement_steps, double simulation_time, bool phasor_sim);

    void runSimulation(double t_eq);
    void initPhononBuilders(std::vector<Cell>& cells, double t_eq, double eff_energy) noexcept;
    [[nodiscard]] std::optional<double> nextImpact(Phonon& p, double time) const noexcept;// NOLINT
    void reset() noexcept {
        total_phonons_ = 0;
        phonon_builders_.clear();
    }
    void setStepAdjustment(std::size_t step_adjustment) {
        step_adjustment_ = step_adjustment;
    }

private:
    std::vector<BuilderObj> phonon_builders_;
    std::vector<double> step_times_;
    double step_time_;
    bool phasor_sim_;
    std::size_t step_adjustment_{ 0 };
    std::size_t total_phonons_{ 0 };

    void runPhononByPhonon(double t_eq);
    void runUsingBuilders(double t_eq);
    static void scatter(Phonon& p, const std::array<double, 3>& relax_rates) noexcept;// NOLINT
    void simulatePhonon(Phonon&& p, std::size_t measurement_steps) const;// NOLINT
    std::optional<double> handleImpacts(Phonon& p, double drift_time, std::size_t sensor_id) const;// NOLINT
};

#endif// PSIM_MODELSIMULATOR_H
