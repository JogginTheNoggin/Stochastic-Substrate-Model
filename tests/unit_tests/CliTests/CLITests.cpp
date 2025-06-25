#include "gtest/gtest.h"
#include "helpers/MockSimulator.h" // The mock simulator for testing
#include "headers/cli/CLI.h"           // The class we are testing
#include <memory>
#include <string>
#include <sstream>

/**
 * @class CLITest
 * @brief Test fixture for the CLI class.
 * @details This fixture sets up a new MockSimulator and a new CLI instance for each
 * test case, ensuring that tests are isolated from one another.
 */
class CLITest : public ::testing::Test {
protected:
    std::shared_ptr<MockSimulator> mockSim;
    std::unique_ptr<CLI> cli;

    // SetUp is called before each test
    void SetUp() override {
        mockSim = std::make_shared<MockSimulator>();
        cli = std::make_unique<CLI>(mockSim);
    }

    // TearDown is called after each test
    void TearDown() override {
        // Smart pointers handle cleanup automatically
    }

    /**
     * @brief Helper function to process a command string through the CLI's public interface.
     * @param command The command string to process, as if typed by a user.
     */
    void process(const std::string& command) {
        // Create a stringstream containing the command to simulate user input.
        std::stringstream inputStream(command);
        // Call the public run method with our simulated stream.
        // Since getline stops on newline, run will process one command and exit the loop.
        cli->run(inputStream);
    }
};

// --- Test Cases for Each Command ---

TEST_F(CLITest, Command_LoadConfig) {
    process("load-config ./my/network.json");
    EXPECT_EQ(mockSim->lastCall, MockSimulator::LastCall::LOAD_CONFIG);
    EXPECT_EQ(mockSim->lastPath, "./my/network.json");
    EXPECT_EQ(mockSim->callCount, 1);
}

TEST_F(CLITest, Command_LoadConfig_NoPath) {
    process("load-config");
    EXPECT_EQ(mockSim->callCount, 0); // Should not call the simulator method
}

TEST_F(CLITest, Command_SaveConfig) {
    process("save-config ./my/snapshot.json");
    EXPECT_EQ(mockSim->lastCall, MockSimulator::LastCall::SAVE_CONFIG);
    EXPECT_EQ(mockSim->lastPath, "./my/snapshot.json");
    EXPECT_EQ(mockSim->callCount, 1);
}

TEST_F(CLITest, Command_NewNetwork) {
    process("new-network 150");
    EXPECT_EQ(mockSim->lastCall, MockSimulator::LastCall::NEW_NETWORK);
    EXPECT_EQ(mockSim->lastNumOperators, 150);
    EXPECT_EQ(mockSim->callCount, 1);
}

TEST_F(CLITest, Command_NewNetwork_InvalidArg) {
    process("new-network abc");
    EXPECT_EQ(mockSim->callCount, 0); // Should fail parsing and not call method
}

TEST_F(CLITest, Command_Run_WithSteps) {
    // Get the future from the promise BEFORE starting the command
    auto future = mockSim->runPromise.get_future();

    process("run 500");

    // Wait for the detached thread to call the mock's run method and fulfill the promise.
    // This synchronizes the test and prevents the race condition.
    future.get();

    EXPECT_EQ(mockSim->lastCall, MockSimulator::LastCall::RUN_STEPS);
    EXPECT_EQ(mockSim->lastNumSteps, 500);
    // Note: The mock doesn't actually run a thread, so we just check if the method was called.
}

TEST_F(CLITest, Command_Run_UntilInactive) {
    // Get the future from the promise BEFORE starting the command
    auto future = mockSim->runPromise.get_future();

    process("run");

    // Wait for the detached thread to signal that it has started
    future.get();

    EXPECT_EQ(mockSim->lastCall, MockSimulator::LastCall::RUN_INACTIVE);
}

TEST_F(CLITest, Command_Pause) {
    process("pause");
    EXPECT_EQ(mockSim->lastCall, MockSimulator::LastCall::REQUEST_STOP);
    EXPECT_TRUE(mockSim->stopRequested);
}

TEST_F(CLITest, Command_Stop) {
    process("stop");
    EXPECT_EQ(mockSim->lastCall, MockSimulator::LastCall::REQUEST_STOP);
    EXPECT_TRUE(mockSim->stopRequested);
}

TEST_F(CLITest, Command_SubmitText) {
    process("submit-text Hello world!");
    EXPECT_EQ(mockSim->lastCall, MockSimulator::LastCall::SUBMIT_TEXT);
    EXPECT_EQ(mockSim->lastSubmittedText, "Hello world!");
}

TEST_F(CLITest, Command_SubmitText_WithQuotes) {
    // The current parsing logic doesn't handle quotes, it just takes the rest of the line.
    process("submit-text \"Quoted text\"");
    EXPECT_EQ(mockSim->lastCall, MockSimulator::LastCall::SUBMIT_TEXT);
    EXPECT_EQ(mockSim->lastSubmittedText, "\"Quoted text\"");
}

TEST_F(CLITest, Command_GetOutput) {
    // This test also verifies that the mock's return value is handled, though we can't see cout.
    process("get-output");
    EXPECT_EQ(mockSim->lastCall, MockSimulator::LastCall::GET_OUTPUT);
    EXPECT_EQ(mockSim->callCount, 1);
}

TEST_F(CLITest, Command_Status) {
    process("status");
    EXPECT_EQ(mockSim->lastCall, MockSimulator::LastCall::GET_STATUS);
    EXPECT_EQ(mockSim->callCount, 1);
}

TEST_F(CLITest, Command_PrintNetwork) {
    process("print-network");
    EXPECT_EQ(mockSim->lastCall, MockSimulator::LastCall::GET_JSON);
    EXPECT_EQ(mockSim->callCount, 1);
}

TEST_F(CLITest, Command_LogFrequency) {
    process("log-frequency 25");
    EXPECT_EQ(mockSim->lastCall, MockSimulator::LastCall::SET_LOG_FREQ);
    EXPECT_EQ(mockSim->lastLogFrequency, 25);
}

TEST_F(CLITest, Command_LogFrequency_InvalidArg) {
    process("log-frequency xyz");
    EXPECT_EQ(mockSim->callCount, 0); // Should fail parsing
    
    mockSim->reset();
    
    process("log-frequency -5");
    EXPECT_EQ(mockSim->callCount, 0); // Should fail validation (must be positive)
}


TEST_F(CLITest, Command_Quit) {
    process("quit");
    // Check that quit signals the simulator to stop
    EXPECT_EQ(mockSim->lastCall, MockSimulator::LastCall::REQUEST_STOP);
    EXPECT_TRUE(mockSim->stopRequested);
}

TEST_F(CLITest, Command_Unknown) {
    process("unknown-command");
    // No method on the simulator should be called for an unknown command.
    EXPECT_EQ(mockSim->lastCall, MockSimulator::LastCall::NONE);
    EXPECT_EQ(mockSim->callCount, 0);
}

TEST_F(CLITest, Command_EmptyInput) {
    process("");
    // No method on the simulator should be called for an empty input line.
    EXPECT_EQ(mockSim->lastCall, MockSimulator::LastCall::NONE);
    EXPECT_EQ(mockSim->callCount, 0);
}