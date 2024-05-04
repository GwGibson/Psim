#include "psim/outputManager.h"
#include "psim/sensor.h"
#include <chrono>
#include <ctime>
#include <execution>
#include <fstream>
#include <functional>
#include <iomanip>
#include <numeric>
#include <ranges>
#include <sstream>

void OutputManager::exportResults(const fs::path& filepath,
    double time,
    std::size_t num_runs,
    SimulationType type) const {
    // Declare variables to hold configuration and function pointer
    std::string appendString;
    std::string typeString;
    std::function<void(std::ofstream&)> exportFunc;

    // Determine configuration and function based on simulation type
    if (type == SimulationType::SteadyState) {
        appendString = "ss_";
        typeString = "Steady State";
        exportFunc = [this](std::ofstream& output) { steadyStateExport(output); };
    } else {
        appendString = "per_";
        typeString = "Periodic";
        exportFunc = [this](std::ofstream& output) { periodicExport(output); };
    }

    std::ofstream output{ adjustPath(filepath, appendString).string(), std::ios_base::trunc };
    output << typeString << " Results from " << filepath.filename() << " @ " << getCurrentDateTime() << " - Time Taken "
           << time << "[s] over " << num_runs << " runs\n";

    exportFunc(output);
}

std::vector<SensorMeasurements> OutputManager::calculateAverages() const {
    if (measurements_.empty()) {
        return {};// Return an empty vector if no measurements are available
    }

    std::vector<SensorMeasurements> averagedMeasurements(measurements_.begin()->second.size());
    // Sum up all measurements for each sensor across all runs
    for (const auto& sensors : measurements_ | std::views::values) {
        for (size_t i = 0; i < sensors.size(); ++i) {
            averagedMeasurements[i].t_steady += sensors[i].t_steady;
            averagedMeasurements[i].std_t_steady += sensors[i].std_t_steady;
            averagedMeasurements[i].x_flux += sensors[i].x_flux;
            averagedMeasurements[i].std_x_flux += sensors[i].std_x_flux;
            averagedMeasurements[i].y_flux += sensors[i].y_flux;
            averagedMeasurements[i].std_y_flux += sensors[i].std_y_flux;
        }
    }

    const auto numRuns = static_cast<double>(measurements_.size());
    // Compute the average for each measurement and sensor
    for (auto& measurement : averagedMeasurements) {
        measurement.t_steady /= numRuns;
        measurement.std_t_steady /= numRuns;
        measurement.x_flux /= numRuns;
        measurement.std_x_flux /= numRuns;
        measurement.y_flux /= numRuns;
        measurement.std_y_flux /= numRuns;
    }

    return averagedMeasurements;
}

void OutputManager::steadyStateExport(std::ofstream& output) const {
    const auto measurements = calculateAverages();
    for (const auto& measurement : measurements) {
        output << measurement.t_steady << ' ' << measurement.std_t_steady << ' ' << measurement.x_flux << ' '
               << measurement.std_x_flux << ' ' << measurement.y_flux << ' ' << measurement.std_y_flux << '\n';
    }
}

// This is useless for a steady-state simulation since the system does not really evolve in the final run
// and previous runs may not provide an 'accurate' picture as the scattering rates etc. will be incorrect.
void OutputManager::periodicExport(std::ofstream& output) const {
    const auto measurements = calculateAverages();
    const std::size_t num_sensors = measurements.size();
    const std::size_t measurement_steps = measurements.back().final_temps.size();
    for (std::size_t step = 0; step < measurement_steps - step_interval_ + 1; step += step_interval_) {
        output << step + step_interval_ / 2 << '\n';
        output << num_sensors << '\n';
        for (const auto& measurement : measurements) {// Measurement data from each sensor
            // Get average temperature over the number of step_intervals
            const auto t_start = std::cbegin(measurement.final_temps) + static_cast<int>(step);
            const auto t_end = t_start + static_cast<int>(step_interval_);
            const auto temp = std::accumulate(t_start, t_end, 0.) / static_cast<double>(step_interval_);
            // Get average fluxes over the number of step intervals
            const auto f_start = std::cbegin(measurement.final_fluxes) + static_cast<int>(step);
            const auto f_end = f_start + static_cast<int>(step_interval_);
            const auto x_flux = std::transform_reduce(std::execution::seq,
                                    f_start,
                                    f_end,
                                    0.,
                                    std::plus{},
                                    [](const auto& flux_pair) { return flux_pair[0]; })
                                / static_cast<double>(step_interval_);
            const auto y_flux = std::transform_reduce(std::execution::seq,
                                    f_start,
                                    f_end,
                                    0.,
                                    std::plus{},
                                    [](const auto& flux_pair) { return flux_pair[1]; })
                                / static_cast<double>(step_interval_);
            output << temp << ' ' << x_flux << ' ' << y_flux << '\n';
        }
    }
}

void OutputManager::addMeasurement(std::size_t runId, SensorMeasurements&& measurement) noexcept {
    measurements_[runId].emplace_back(std::move(measurement));
}

void OutputManager::sortMeasurements(std::size_t runId) noexcept {
    std::ranges::sort(measurements_[runId], [](const auto& m1, const auto& m2) { return m1.id < m2.id; });
}

std::filesystem::path OutputManager::adjustPath(const fs::path& filepath, const std::string& prepend) {
    auto new_path = filepath;
    new_path.replace_extension(".txt");
    const auto filename = new_path.filename().string();
    new_path.replace_filename(prepend + filename);
    return new_path;
}

std::string OutputManager::getCurrentDateTime() {
    const auto now = std::chrono::system_clock::now();
    const auto rounded_down = std::chrono::floor<std::chrono::seconds>(now);
    const auto rounded_down_time_t = std::chrono::system_clock::to_time_t(rounded_down);
    std::tm tm_time;
#ifdef _WIN32
    if (localtime_s(&tm_time, &rounded_down_time_t) != 0) { return "Unknown Time"; }
#else
    if (localtime_r(&rounded_down_time_t, &tm_time) == nullptr) { return "Unknown Time"; }
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_time, "%Y-%m-%d %X");
    return oss.str();
}
