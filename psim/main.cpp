#include "psim/inputManager.h"// for InputManager
#include "psim/model.h"// for Model
#include "psim/timer.h"// for Timer
#include <exception>// for exception
#include <filesystem>// for operator<<, path
#include <iostream>// for char_traits, operator<<, ostream, bas...
#include <optional>// for optional
#include <sstream>// for basic_stringbuf<>::int_type, basic_st...
#include <string>// for string, basic_string
#include <vector>// for vector


// NOLINTNEXTLINE(bugprone-exception-escape)
int main(int argc, char* argv[]) {
    if (argc > 1) {
        try {
            std::vector<std::string> filenames(argv + 1, argv + argc);
            for (const auto& filename : filenames) {
                const std::filesystem::path filepath = filename;
                if (auto model = InputManager::deserialize(filepath); model) {
                    auto& m = *model;// NOLINT
                    Timer timer;
                    m.runSimulation();
                    const auto time = timer.get_time_diff();
                    std::cout << "Time Taken: " << time << "[s]\n";
                    m.exportResults(filepath, time);
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
