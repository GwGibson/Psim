#include "psim/inputManager.h"
#include "psim/model.h"
#include "psim/timer.h"
#include <exception>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <vector>


// NOLINTNEXTLINE(bugprone-exception-escape)
int main(int argc, char* argv[]) {
    if (argc > 1) {
        try {
            const std::vector<std::string> filenames(argv + 1, argv + argc);
            for (const auto& filename : filenames) {
                const std::filesystem::path filepath = filename;
                if (auto model = InputManager::deserialize(filepath); model) {
                    Timer timer;
                    model->runSimulation();
                    const auto time = timer.get_time_diff();
                    std::cout << "Time Taken: " << time << "[s]\n";
                    model->exportResults(filepath, time);
                } else {
                    std::cerr << "There was an error reading the data from the file at " << filepath << '\n';
                }
            }
        } catch (const std::exception& e) { std::cerr << e.what() << '\n'; }
    } else {
        std::cout << "Need filenames\n";
    }
    std::cout << "done\n";
}
