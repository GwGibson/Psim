#include "psim/outputManager.h"
#include "psim/sensor.h"// for SensorMeasurements
#include <algorithm>// for sort
#include <chrono>// for system_clock
#include <ctime>// for localtime, size_t
#include <execution>// for seq
#include <filesystem>// for path, operator<<
#include <fstream>// basic_ofstream
#include <functional>// for plus
#include <iomanip>// for operator<<, put_time
#include <iterator>// for cbegin, begin, end
#include <memory>// for allocator_traits<>::value_type
#include <numeric>// for accumulate
#include <ostream>// for operator<<, basic_ostream, basic...
#include <sstream>// for basic_stringbuf<>::int_type, bas...


void OutputManager::steadyStateExport(const fs::path& filepath, double time) const {
    std::ofstream output{ adjustPath(filepath, "ss_").string(), std::ios_base::trunc };// Overwrites existing file
    output << "Steady State Results from " << filepath.filename() << " @ " + getCurrentDateTime() << " - Time Taken "
           << time << "[s]\n";
    for (const auto& measurement : measurements_) {
        output << measurement.t_steady << ' ' << measurement.std_t_steady << ' ' << measurement.x_flux << ' '
               << measurement.std_x_flux << ' ' << measurement.y_flux << ' ' << measurement.std_y_flux << '\n';
    }
    output.close();
}

// This is useless for a steady-state simulation since the system does not really evolve in the final run
// and previous runs may not provide an 'accurate' picture as the scattering rates etc. will be incorrect.
void OutputManager::periodicExport(const fs::path& filepath, double time) const {
    std::ofstream output{ adjustPath(filepath, "per_").string(), std::ios_base::trunc };// Overwrites existing file
    output << "Periodic Results in " << step_interval_ << " step intervals from " << filepath.filename()
           << " @ " + getCurrentDateTime() << " - Time Taken " << time << "[s]\n";
    const std::size_t num_sensors = measurements_.size();
    const std::size_t measurement_steps = measurements_.back().final_temps.size();
    for (std::size_t step = 0; step < measurement_steps - step_interval_ + 1; step += step_interval_) {
        output << step + step_interval_ / 2 << '\n';
        output << num_sensors << '\n';
        for (const auto& measurement : measurements_) {// Measurement data from each sensor
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
    output.close();
}

void OutputManager::addMeasurement(SensorMeasurements&& measurement) noexcept {
    measurements_.push_back(measurement);
}

void OutputManager::sortMeasurements() noexcept {
    std::sort(std::begin(measurements_), std::end(measurements_), [](const auto& m1, const auto& m2) {// NOLINT
        return m1.id < m2.id;
    });
}

std::filesystem::path OutputManager::adjustPath(const fs::path& filepath, const std::string& prepend) {
    auto new_path = filepath;
    new_path.replace_extension(".txt");
    auto filename = new_path.filename().string();
    new_path.replace_filename(prepend + filename);
    return new_path;
}

std::string OutputManager::getCurrentDateTime() {
    const auto now = std::chrono::system_clock::now();
    const auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;// NOLINT
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");// NOLINT (function not thread safe)
    return ss.str();
}
