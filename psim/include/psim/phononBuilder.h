#ifndef PSIM_PHONONBUILDER_H
#define PSIM_PHONONBUILDER_H

#include "phonon.h"
#include <stack>

class Cell;
class EmitSurface;

// TODO: is abstract class necessary here - using variants in ModelSimulator
class PhononBuilder {
public:
    constexpr PhononBuilder() noexcept = default;
    virtual ~PhononBuilder() = default;
    constexpr PhononBuilder(const PhononBuilder&) = default;
    constexpr PhononBuilder(PhononBuilder&&) noexcept = default;
    PhononBuilder& operator=(const PhononBuilder&) = default;
    PhononBuilder& operator=(PhononBuilder&&) = default;
    /**
     * Caller must verify the phonon builder has phonons to build by using the hasPhonons() function
     * before calling this function.
     * @param t_eq - Equilibrium temperature of the system
     * @return A phonon that is created according to the builder specifications
     */
    [[nodiscard]] virtual Phonon operator()(double t_eq) noexcept = 0;
    [[nodiscard]] virtual bool hasPhonons() const noexcept = 0;

    [[nodiscard]] std::size_t totalPhonons() const noexcept {
        return total_phonons_;
    }

protected:
    std::size_t total_phonons_{ 0 };
};

class CellOriginBuilder : public PhononBuilder {
public:
    CellOriginBuilder() = default;

    [[nodiscard]] Phonon operator()(double t_eq) noexcept override;
    [[nodiscard]] bool hasPhonons() const noexcept override {
        return !cells_.empty();
    }

    void addCellPhonons(Cell* cell, std::size_t num_phonons) noexcept;

private:
    std::stack<std::pair<Cell*, std::size_t>> cells_;
};

class SurfaceOriginBuilder : public PhononBuilder {
public:
    SurfaceOriginBuilder(Cell& cell, const EmitSurface& surface, std::size_t num_phonons);

    [[nodiscard]] Phonon operator()(double t_eq) noexcept override;
    [[nodiscard]] bool hasPhonons() const noexcept override {
        return total_phonons_ > 0;
    }

private:
    Cell& cell_;

protected:
    const EmitSurface& surface_;
};

class PhasorBuilder : public SurfaceOriginBuilder {
public:
    using SurfaceOriginBuilder::SurfaceOriginBuilder;

    [[nodiscard]] Phonon operator()(double t_eq) noexcept override;
};

#endif// PSIM_PHONONBUILDER_H
