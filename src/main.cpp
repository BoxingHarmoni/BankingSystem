#include "Utils.hpp"
#include "BankService.hpp"
#include "ConsoleUI.hpp"
#include <iostream>
#include <filesystem>

int main() {
    // Enable UTF-8 encoding and ANSI escape codes on Windows console
    Banking::Utils::initConsole();

    // Ensure data directory exists
    try {
        std::filesystem::create_directories("data");
    } catch (...) {}

    // Initialize Bank Service with persistent JSON storage
    Banking::BankService bank("data/bank_data.json");
    if (!bank.initialize()) {
        std::cerr << Banking::Utils::Color::RED 
                  << "[!] Помилка ініціалізації сховища даних банку!" 
                  << Banking::Utils::Color::RESET << std::endl;
        return 1;
    }

    // Launch interactive console interface
    Banking::ConsoleUI ui(bank);
    ui.run();

    return 0;
}
