#include "psim/sensor.h"

Sensor::Sensor(std::size_t ID,// NOLINT
    const Material& material,
    SimulationType type,
    std::size_t num_measurements,
    double t_init)
    : ID_{ ID }
    , updateMutex_{ std::make_unique<std::mutex>() } {
    switch (type) {
    case SimulationType::SteadyState:
        controller_ = std::make_unique<SteadyStateController>(material, t_init);
        break;
    case SimulationType::Periodic:
        controller_ = std::make_unique<PeriodicController>(material, t_init);
        break;
    case SimulationType::Transient:
        controller_ = std::make_unique<TransientController>(material, t_init, num_measurements);
        break;
    default:
        throw std::runtime_error(std::string("Invalid simulation type.\n"));
    }
    inc_energy_.resize(num_measurements);
    inc_flux_.resize(num_measurements);
}

void Sensor::initialUpdate(Phonon& p, const Material::Table& table) const noexcept {// NOLINT
    controller_->initialUpdate(p, table);
}

void Sensor::initialUpdate(Phonon& p) const noexcept {// NOLINT
    controller_->initialUpdate(p);
}

void Sensor::scatterUpdate(Phonon& p) const noexcept {// NOLINT
    controller_->scatterUpdate(p);
}

bool Sensor::resetRequired(double t_final, std::vector<double>&& final_temps) const noexcept {
    return controller_->resetRequired(t_final, std::move(final_temps));
}

void Sensor::updateHeatParams(const Phonon& p, std::size_t step) noexcept {// NOLINT
    const auto sign = p.getSign();
    const auto [vx, vy] = p.getVelVector();
    std::scoped_lock lg(*updateMutex_);// NOLINT
    inc_energy_[step] += sign;
    // Track net velocities in each cell for flux calculations
    auto& v = inc_flux_[step];// NOLINT
    v[0] += vx * sign;
    v[1] += vy * sign;
}

void Sensor::reset(bool full_reset) noexcept {
    controller_->reset(full_reset);
    // Reset incoming flux values to 0.
    for (auto& flux_array : inc_flux_) { std::ranges::fill(flux_array, 0.); }
    // Reset incoming energies to 0.
    std::ranges::fill(inc_energy_, 0);
}
