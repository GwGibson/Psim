#include "psim/material.h"
#include "psim/phonon.h"// for Phonon, Phonon::RelaxRates
#include "psim/utils.h"// for urand
#include <algorithm>// for transform, generate, max, lower_bound
#include <cmath>// for pow, sqrt, exp, isnan, sinh
#include <functional>// for multiplies
#include <iterator>// for cbegin, cend, begin, end, distance
#include <numeric>// for accumulate
#include <utility>// for pair

namespace {
constexpr double HBAR = 1.054517e-34;
constexpr double BOLTZ = 1.38065e-23;
}// namespace

using Array = Material::Array;
using Table = Material::Table;
using Polar = Material::Polar;

Material::Material(std::size_t mat_id, const DispersionData& disp_data, const RelaxationData& relax_data)
    : id_{ mat_id }
    , b_l_{ relax_data.b_l }
    , b_tn_{ relax_data.b_tn }
    , b_tu_{ relax_data.b_tu }
    , b_i_{ relax_data.b_i }
    , w_{ relax_data.w }
    , w_max_la_{ disp_data.w_max_la }
    , w_max_ta_{ disp_data.w_max_ta }
    , freq_width_{ std::max(w_max_la_, w_max_ta_) / NUM_FREQ_BINS } {
    const auto& [LA_coeffs, TA_coeffs, _unused, _unused1] = disp_data;
    // Build frequency Table
    std::generate(std::begin(frequencies_), std::end(frequencies_), [n = -1, this]() mutable {// NOLINT
        return (n += 2) * freq_width_ / 2.;// NOLINT
    });

    // Build velocity and state density tables
    std::size_t index = 0;
    for (const auto& freq : frequencies_) {
        const double la_gv = getGv(freq, LA_coeffs);
        velocities_la_[index] = la_gv;
        densities_la_[index] = pow(getK(freq, LA_coeffs), 2) / 2. / pow(Utils::PI, 2) / la_gv;// NOLINT
        const double ta_gv = getGv(freq, TA_coeffs);
        if (!std::isnan(ta_gv)) {
            velocities_ta_[index] = ta_gv;
            // Do not divide by 2 since the TA branch is doubly degenerate
            // Can account for this in the density of states array to save computations elsewhere
            densities_ta_[index] = pow(getK(freq, TA_coeffs), 2) / pow(Utils::PI, 2) / ta_gv;
        }
        ++index;
    }
}

// Returns the scattering rates in the following order [N_rate, U_rate, I_rate]
Phonon::RelaxRates Material::relaxRates(double temp, double freq, Polar polarization) const noexcept {
    return std::array{ tauNInv(temp, freq, polarization), tauUInv(temp, freq, polarization), tauIInv(freq) };
}

// Returns the scattering rates in the following order [N_rate, U_rate, I_rate]
Phonon::RelaxRates Material::relaxRates(std::size_t freq_index, Polar polarization, double temp) const {
    const auto freq = frequencies_[freq_index];
    return std::array{ tauNInv(temp, freq, polarization), tauUInv(temp, freq, polarization), tauIInv(freq) };
}

std::pair<std::size_t, Polar> Material::freqIndex(const Table& dist) noexcept {
    std::size_t low = 0;
    std::size_t high = dist.size() - 1;
    auto mid = low + (high - low) / 2;
    const double rand = Utils::urand();
    // Perform bisection search
    while (high - low > 1) {
        rand < dist[mid].first ? high = mid : low = mid;
        mid = (low + high) / 2;// NOLINT
    }
    return { high, Utils::urand() <= dist[high].second ? Polar::LA : Polar::TA };
}

double Material::getFreq(std::size_t index) const noexcept {
    return (full_simulation_) ? frequencies_[index]
                              : frequencies_[index] + (2. * Utils::urand() - 1.) * freq_width_ / 2.;// NOLINT
}

double Material::getVel(std::size_t index, Polar polar) const noexcept {
    return (polar == Polar::LA) ? velocities_la_[index] : velocities_ta_[index];
}

double Material::getK(double freq, std::array<double, 3> coeffs) {
    const double d = pow(coeffs[1], 2) - 4. * coeffs[0] * (coeffs[2] - freq);// NOLINT
    const double a = (-coeffs[1] - sqrt(d)) / (2. * coeffs[0]);// NOLINT
    const double b = (-coeffs[1] + sqrt(d)) / (2. * coeffs[0]);// NOLINT
    return (a < b) ? a : b;
}

double Material::getGv(double freq, std::array<double, 3> coeffs) {
    return 2. * coeffs[0] * getK(freq, coeffs) + coeffs[1];// NOLINT
}

double Material::theoreticalEnergy(double temp, bool pseudo) const noexcept {
    return (pseudo) ? scatterEnergy(temp) : baseData(temp).second;
}

void Material::initializeTables(double low_temp, double high_temp, float temp_interval) {
    const auto num_steps = static_cast<std::size_t>((high_temp - low_temp) / static_cast<double>(temp_interval));
    temps_.resize(num_steps, 0.);

    std::generate(std::begin(temps_), std::end(temps_), [n = -1., &low_temp, &temp_interval]() mutable {// NOLINT
        return low_temp + static_cast<double>(temp_interval) * (n += 1);
    });
    temps_.push_back(high_temp);

    for (const auto& temp : temps_) {
        // TODO: This can be optimized - but not too important since only called once
        const Array la_base = phononDist(temp, Polar::LA);
        const Array ta_base = phononDist(temp, Polar::TA);
        const auto heat_capacity = std::accumulate(std::cbegin(la_base), std::cend(la_base), 0.)
                                   + std::accumulate(std::cbegin(ta_base), std::cend(ta_base), 0.);

        base_tables_.emplace_back(buildCumulDist(la_base, ta_base), heat_capacity);
        auto emitData = cumulDistEmit(phononDist(temp, Polar::LA), phononDist(temp, Polar::TA));
        emit_tables_.emplace_back(emitData.first, emitData.second);
        auto scatterData = cumulDistScatter(phononDist(temp, Polar::LA), phononDist(temp, Polar::TA), temp);
        scatter_tables_.emplace_back(scatterData.first, scatterData.second);
    }
}

std::pair<const Table*, double> Material::baseData(double temp) const {
    const auto& table = base_tables_[(getTempIndex(temp))];
    return { &table.table, table.cumul_sum };
}

std::pair<const Table*, double> Material::emitData(double temp) const {
    const auto& table = emit_tables_[(getTempIndex(temp))];
    return { &table.table, table.cumul_sum };
}

std::pair<const Table*, double> Material::scatterData(double temp) const {
    const auto& table = scatter_tables_[(getTempIndex(temp))];
    return { &table.table, table.cumul_sum };
}

std::pair<Table, double> Material::cumulDistEmit(Array&& la_dist, Array&& ta_dist) const {
    auto transform = [](auto& dist, const auto& velocities) {
        std::transform(
            std::cbegin(dist), std::cend(dist), std::cbegin(velocities), std::begin(dist), std::multiplies<>());
    };
    transform(la_dist, velocities_la_);
    transform(ta_dist, velocities_ta_);
    return { buildCumulDist(la_dist, ta_dist),
        std::accumulate(std::cbegin(la_dist), std::cend(la_dist), 0.)
            + std::accumulate(std::cbegin(ta_dist), std::cend(ta_dist), 0.) };
}

std::pair<Table, double> Material::cumulDistScatter(Array&& la_dist, Array&& ta_dist, double temp) const {
    auto transform = [&, this](auto& dist, const auto& polar) {
        std::transform(std::cbegin(dist),
            std::cend(dist),
            std::cbegin(frequencies_),
            std::begin(dist),
            [&](const auto& elem, const auto& freq) {
                const auto scatter_rates = relaxRates(temp, freq, polar);
                return elem * std::accumulate(std::cbegin(scatter_rates), std::cend(scatter_rates), 0.);
            });
    };
    transform(la_dist, Polar::LA);
    transform(ta_dist, Polar::TA);
    return { buildCumulDist(la_dist, ta_dist),
        std::accumulate(std::cbegin(la_dist), std::cend(la_dist), 0.)
            + std::accumulate(std::cbegin(ta_dist), std::cend(ta_dist), 0.) };
}

Table Material::buildCumulDist(const Array& t1, const Array& t2) {// NOLINT
    Table cumul_dist;
    double const cumul_sum =
        std::accumulate(std::cbegin(t1), std::cend(t1), 0.) + std::accumulate(std::cbegin(t2), std::cend(t2), 0.);
    cumul_dist[0] = { (t1[0] + t2[0]) / cumul_sum, t1[0] / (t1[0] + t2[0]) };
    for (std::size_t i = 1; i < NUM_FREQ_BINS; ++i) {
        cumul_dist[i] = { cumul_dist[i - 1].first + (t1[i] + t2[i]) / cumul_sum, t1[i] / (t1[i] + t2[i]) };
    }
    return cumul_dist;
}

// Returns an array where each entry represents the total phonon energy that the corresponding
// frequency bin contributes or the heat capacity in the case of deviational simulation.
// The sum of all the entries is the total energy or heat capacity per volume in this material
Array Material::phononDist(double temp, Polar polarization) const {
    // Build bose-einstein Array (including (HBAR * freq) in original distribution - hw/(exp(hw/(k_b*T)-1)
    Array bose_einstein;
    const auto& densities = (polarization == Polar::LA) ? densities_la_ : densities_ta_;
    // This will need revision if the optical branches are added
    std::transform(std::cbegin(frequencies_),
        std::cend(frequencies_),
        std::cbegin(densities),
        std::begin(bose_einstein),
        [&, const_calc = HBAR / (BOLTZ * temp)](const auto freq, const auto density) {
            double dist = freq * HBAR / expm1(const_calc * freq) * freq_width_ * density;
            // Use BE derivative for phonon distributions
            if (!full_simulation_) {
                dist *= const_calc * freq * exp(const_calc * freq) / (expm1(const_calc * freq) * temp);
            }
            // Do not need to account for double degeneracy in the TA branch here since it is taken care of when
            // the density of states array is created
            return dist;
        });
    return bose_einstein;
}

// Normal scattering
double Material::tauNInv(double temp, double freq, Polar polarization) const noexcept {
    const auto tau_inv = [&]() {
        switch (polarization) {
        case Polar::LA:
            return b_l_ * freq * freq * pow(temp, 3);
        case Polar::TA:
            if (freq < w_) { return b_tn_ * freq * pow(temp, 4); }
            return 0.;
        default:
            return 0.;
        }
    }();
    return tau_inv;
}

// Umklapp scattering
double Material::tauUInv(double temp, double freq, Polar polarization) const noexcept {
    const auto tau_inv = [&]() {
        switch (polarization) {
        case Polar::LA:
            return b_l_ * freq * freq * pow(temp, 3);
        case Polar::TA:
            if (freq >= w_) { return b_tu_ * freq * freq / sinh(HBAR * freq / (temp * BOLTZ)); }
            return 0.;
        default:
            return 0.;
        }
    }();
    return tau_inv;
}

// Impurity Scattering
double Material::tauIInv(double freq) const noexcept {
    return b_i_ * pow(freq, 4);
}

std::size_t Material::getTempIndex(double temp) const noexcept {
    const auto index = static_cast<std::size_t>(
        std::distance(std::cbegin(temps_), std::lower_bound(std::cbegin(temps_), std::cend(temps_), temp)));
    const auto maxIndex = temps_.size() - 1;
    return (index > maxIndex) ? maxIndex : index;
}
