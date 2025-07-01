#include "../tests/unit_tests/helpers/MockSimulator.h" // Temporarily using mock simulator to run cli 
#include "headers/cli/CLI.h"
#include "headers/Simulator.h"
#include "headers/util/LibsodiumRandomSource.h"
#include "headers/util/PseudoRandomSource.h"
#include <iostream>
#include <memory>

/**
 * @brief The main entry point for the Neuron Simulator application.
 * @details This function is responsible for initializing the core `Simulator` object
 * and the `CLI` object. It then starts the command-line interface, which takes over
 * the main thread until the user quits.
 */
int main(int argc, char* argv[]) {
    // Use a smart pointer to manage the Simulator's lifetime and share it safely.
    //auto mockSimulator = std::make_shared<MockSimulator>();

    // Create the CLI object, passing it the simulator instance.
    //auto cli = std::make_unique<CLI>(mockSimulator);
    // LibsodiumRandomSource src(); 
    // std::unique_ptr<RandomSource> pseudoSource = ;
    Randomizer* rand = new Randomizer(std::make_unique<LibsodiumRandomSource>());
    //Randomizer* rand = new Randomizer(std::make_unique<PseudoRandomSource>());
    auto simulator = std::make_shared<Simulator>("", rand );

    // Create the CLI object, passing it the simulator instance.
    auto cli = std::make_unique<CLI>(simulator);

    // Run the CLI. This will block the main thread until the user quits.
    cli->run();

    std::cout << "Application will now exit." << std::endl;

    return 0;
}

      