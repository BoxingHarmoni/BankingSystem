#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <iostream>

namespace Banking {
namespace Utils {

    // Console formatting & colors (ANSI 24-bit / 16-color)
    namespace Color {
        const std::string RESET         = "\033[0m";
        const std::string BOLD          = "\033[1m";
        const std::string DIM           = "\033[2m";
        const std::string UNDERLINE     = "\033[4m";
        const std::string INVERSE       = "\033[7m";

        // Standard Colors
        const std::string BLACK         = "\033[30m";
        const std::string RED           = "\033[31m";
        const std::string GREEN         = "\033[32m";
        const std::string YELLOW        = "\033[33m";
        const std::string BLUE          = "\033[34m";
        const std::string MAGENTA       = "\033[35m";
        const std::string CYAN          = "\033[36m";
        const std::string WHITE         = "\033[37m";

        // Bright Foreground Colors
        const std::string BRIGHT_BLACK  = "\033[90m";
        const std::string BRIGHT_RED    = "\033[91m";
        const std::string BRIGHT_GREEN  = "\033[92m";
        const std::string BRIGHT_YELLOW = "\033[93m";
        const std::string BRIGHT_BLUE   = "\033[94m";
        const std::string BRIGHT_MAGENTA= "\033[95m";
        const std::string BRIGHT_CYAN   = "\033[96m";
        const std::string BRIGHT_WHITE  = "\033[97m";

        // Background Colors
        const std::string BG_BLACK      = "\033[40m";
        const std::string BG_RED        = "\033[41m";
        const std::string BG_GREEN      = "\033[42m";
        const std::string BG_YELLOW     = "\033[43m";
        const std::string BG_BLUE       = "\033[44m";
        const std::string BG_MAGENTA    = "\033[45m";
        const std::string BG_CYAN       = "\033[46m";
        const std::string BG_WHITE      = "\033[47m";
        const std::string BG_CARD       = "\033[48;5;236m"; // Dark metallic grey for card
    }

    // Windows console initialization (UTF-8 and ANSI colors)
    void initConsole();

    // Security & Hashing (Self-contained SHA-256)
    std::string sha256(const std::string& input);

    // Date & Time
    std::string getCurrentDateTime();

    // Generators
    std::string generateIBAN(uint64_t sequenceNumber);
    std::string generateCardNumber(uint64_t sequenceNumber);
    std::string generateTransactionId();

    // Formatting
    std::string formatMoney(double amount, const std::string& currency = "грн");
    std::string trim(const std::string& str);

    // Secure console input
    std::string readPassword(const std::string& prompt);
    double readDouble(const std::string& prompt, double minVal = 0.01, double maxVal = 100000000.0);
    int readInt(const std::string& prompt, int minVal, int maxVal);
    std::string readString(const std::string& prompt, bool allowEmpty = false);

    // Screen helpers & Graphical components
    void clearScreen();
    void pause();
    void renderBankLogo();
    void renderCard(const std::string& cardNumber, 
                    const std::string& cardHolder, 
                    const std::string& expiry, 
                    double balance, 
                    const std::string& currency, 
                    bool isFrozen);
    void renderReceipt(const std::string& txId,
                       const std::string& typeName,
                       double amount,
                       double balanceAfter,
                       const std::string& counterparty,
                       const std::string& description,
                       const std::string& timestamp);

    // String escape for JSON
    std::string escapeJSON(const std::string& s);
    std::string unescapeJSON(const std::string& s);

} // namespace Utils
} // namespace Banking
