#include "Utils.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <random>
#include <limits>
#include <cstdint>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#include <io.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

namespace Banking {
namespace Utils {

    void initConsole() {
#ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut != INVALID_HANDLE_VALUE) {
            DWORD dwMode = 0;
            if (GetConsoleMode(hOut, &dwMode)) {
                dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                SetConsoleMode(hOut, dwMode);
            }
        }
#endif
    }

    // SHA-256 implementation
    namespace {
        inline uint32_t rotr(uint32_t x, uint32_t n) {
            return (x >> n) | (x << (32 - n));
        }

        inline uint32_t choose(uint32_t e, uint32_t f, uint32_t g) {
            return (e & f) ^ (~e & g);
        }

        inline uint32_t majority(uint32_t a, uint32_t b, uint32_t c) {
            return (a & b) ^ (a & c) ^ (b & c);
        }

        inline uint32_t sig0(uint32_t x) {
            return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
        }

        inline uint32_t sig1(uint32_t x) {
            return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
        }

        inline uint32_t theta0(uint32_t x) {
            return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
        }

        inline uint32_t theta1(uint32_t x) {
            return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
        }

        const uint32_t K[64] = {
            0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
            0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
            0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
            0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
            0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
            0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
            0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
            0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
            0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
            0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
            0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
            0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
            0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
            0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
            0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
            0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
        };
    }

    std::string sha256(const std::string& input) {
        uint32_t H[8] = {
            0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
            0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
        };

        std::vector<uint8_t> msg(input.begin(), input.end());
        uint64_t bitLen = msg.size() * 8;

        msg.push_back(0x80);
        while ((msg.size() % 64) != 56) {
            msg.push_back(0x00);
        }

        for (int i = 7; i >= 0; --i) {
            msg.push_back(static_cast<uint8_t>((bitLen >> (i * 8)) & 0xFF));
        }

        for (size_t chunk = 0; chunk < msg.size(); chunk += 64) {
            uint32_t W[64];
            for (int t = 0; t < 16; ++t) {
                W[t] = (static_cast<uint32_t>(msg[chunk + t * 4]) << 24) |
                       (static_cast<uint32_t>(msg[chunk + t * 4 + 1]) << 16) |
                       (static_cast<uint32_t>(msg[chunk + t * 4 + 2]) << 8) |
                       (static_cast<uint32_t>(msg[chunk + t * 4 + 3]));
            }
            for (int t = 16; t < 64; ++t) {
                W[t] = theta1(W[t - 2]) + W[t - 7] + theta0(W[t - 15]) + W[t - 16];
            }

            uint32_t a = H[0];
            uint32_t b = H[1];
            uint32_t c = H[2];
            uint32_t d = H[3];
            uint32_t e = H[4];
            uint32_t f = H[5];
            uint32_t g = H[6];
            uint32_t h = H[7];

            for (int t = 0; t < 64; ++t) {
                uint32_t T1 = h + sig1(e) + choose(e, f, g) + K[t] + W[t];
                uint32_t T2 = sig0(a) + majority(a, b, c);
                h = g;
                g = f;
                f = e;
                e = d + T1;
                d = c;
                c = b;
                b = a;
                a = T1 + T2;
            }

            H[0] += a;
            H[1] += b;
            H[2] += c;
            H[3] += d;
            H[4] += e;
            H[5] += f;
            H[6] += g;
            H[7] += h;
        }

        std::ostringstream oss;
        for (int i = 0; i < 8; ++i) {
            oss << std::hex << std::setfill('0') << std::setw(8) << H[i];
        }
        return oss.str();
    }

    std::string getCurrentDateTime() {
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::tm tm_now;
#ifdef _WIN32
        localtime_s(&tm_now, &now_time);
#else
        localtime_r(&now_time, &tm_now);
#endif
        std::ostringstream oss;
        oss << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    std::string generateIBAN(uint64_t sequenceNumber) {
        // Format: UA29305299 + 19 digits (total 29 chars)
        std::ostringstream oss;
        oss << "UA" << std::setfill('0') << std::setw(2) << (sequenceNumber % 89 + 10)
            << "305299"
            << std::setfill('0') << std::setw(19) << sequenceNumber;
        return oss.str();
    }

    std::string generateCardNumber(uint64_t sequenceNumber) {
        // Standard 16 digit format: 4441 5500 XXXX XXXX
        std::ostringstream oss;
        uint64_t base = 4441550000000000ULL + (sequenceNumber % 100000000ULL);
        std::string raw = std::to_string(base);
        while (raw.length() < 16) raw = "4" + raw;
        return raw.substr(0, 4) + "-" + raw.substr(4, 4) + "-" + raw.substr(8, 4) + "-" + raw.substr(12, 4);
    }

    std::string generateTransactionId() {
        static uint64_t counter = 1000;
        auto now = std::chrono::system_clock::now().time_since_epoch();
        uint64_t millis = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
        std::ostringstream oss;
        oss << "TX-" << (millis % 10000000) << "-" << (++counter);
        return oss.str();
    }

    std::string formatMoney(double amount, const std::string& currency) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << amount << " " << currency;
        return oss.str();
    }

    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

    std::string readPassword(const std::string& prompt) {
        std::cout << prompt;
        std::string password;
#ifdef _WIN32
        if (!_isatty(_fileno(stdin))) {
            if (!std::getline(std::cin, password)) {
                if (std::cin.eof()) exit(0);
            }
            return trim(password);
        }

        int ch;
        while ((ch = _getch()) != '\r' && ch != '\n' && ch != EOF) {
            if (ch == '\b') {
                if (!password.empty()) {
                    password.pop_back();
                    std::cout << "\b \b";
                }
            } else if (ch >= 32 && ch <= 126) {
                password.push_back(static_cast<char>(ch));
                std::cout << '*';
            }
        }
        std::cout << "\n";
#else
        if (!isatty(STDIN_FILENO)) {
            if (!std::getline(std::cin, password)) {
                if (std::cin.eof()) exit(0);
            }
            return trim(password);
        }

        termios oldt;
        tcgetattr(STDIN_FILENO, &oldt);
        termios newt = oldt;
        newt.c_lflag &= ~ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        if (!std::getline(std::cin, password)) {
            if (std::cin.eof()) exit(0);
        }
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        std::cout << "\n";
#endif
        return trim(password);
    }

    double readDouble(const std::string& prompt, double minVal, double maxVal) {
        while (true) {
            std::cout << prompt;
            std::string line;
            if (!std::getline(std::cin, line)) {
                if (std::cin.eof()) exit(0);
                return minVal;
            }
            line = trim(line);
            std::replace(line.begin(), line.end(), ',', '.');

            try {
                size_t idx = 0;
                double val = std::stod(line, &idx);
                if (idx == line.length() && val >= minVal && val <= maxVal) {
                    return val;
                }
            } catch (...) {}

            std::cout << Color::RED << " [!] Помилка: Введіть число від " 
                      << std::fixed << std::setprecision(2) << minVal 
                      << " до " << maxVal << Color::RESET << "\n";
        }
    }

    int readInt(const std::string& prompt, int minVal, int maxVal) {
        while (true) {
            std::cout << prompt;
            std::string line;
            if (!std::getline(std::cin, line)) {
                if (std::cin.eof()) exit(0);
                return minVal;
            }
            line = trim(line);
            try {
                size_t idx = 0;
                int val = std::stoi(line, &idx);
                if (idx == line.length() && val >= minVal && val <= maxVal) {
                    return val;
                }
            } catch (...) {}

            std::cout << Color::RED << " [!] Помилка: Оберіть пункт від " 
                      << minVal << " до " << maxVal << Color::RESET << "\n";
        }
    }

    std::string readString(const std::string& prompt, bool allowEmpty) {
        while (true) {
            std::cout << prompt;
            std::string line;
            if (!std::getline(std::cin, line)) {
                if (std::cin.eof()) exit(0);
                return "";
            }
            line = trim(line);
            if (allowEmpty || !line.empty()) {
                return line;
            }
            std::cout << Color::RED << " [!] Поле не може бути порожнім." << Color::RESET << "\n";
        }
    }

    void clearScreen() {
#ifdef _WIN32
        system("cls");
#else
        system("clear");
#endif
    }

    void renderBankLogo() {
        std::cout << Color::CYAN << Color::BOLD
                  << "  ╭─────────────────────────────────────────────────────────────────────────────╮\n"
                  << "  │   ██████╗ ███████╗███╗   ███╗██╗███╗   ██╗██╗    ██████╗  █████╗ ███╗   ██╗ │\n"
                  << "  │  ██╔════╝ ██╔════╝████╗ ████║██║████╗  ██║██║    ██╔══██╗██╔══██╗████╗  ██║ │\n"
                  << "  │  ██║  ███╗█████╗  ██╔████╔██║██║██╔██╗ ██║██║    ██████╔╝███████║██╔██╗ ██║ │\n"
                  << "  │  ██║   ██║██╔══╝  ██║╚██╔╝██║██║██║╚██╗██║██║    ██╔══██╗██╔══██║██║╚██╗██║ │\n"
                  << "  │  ╚██████╔╝███████╗██║ ╚═╝ ██║██║██║ ╚████║██║    ██████╔╝██║  ██║██║ ╚████║ │\n"
                  << "  │   ╚═════╝ ╚══════╝╚═╝     ╚═╝╚═╝╚═╝  ╚═══╝╚═╝    ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═══╝ │\n"
                  << "  │  " << Color::BRIGHT_WHITE << "                   N A T I O N A L   B A N K   •   U A                     " << Color::CYAN << "│\n"
                  << "  ╰─────────────────────────────────────────────────────────────────────────────╯\n"
                  << Color::RESET;
    }

    void renderCard(const std::string& cardNumber, 
                    const std::string& cardHolder, 
                    const std::string& expiry, 
                    double balance, 
                    const std::string& currency, 
                    bool isFrozen) {
        std::string statusBadge = isFrozen 
            ? (Color::RED + "● ЗАБЛОКОВАНО" + Color::RESET)
            : (Color::BRIGHT_GREEN + "● АКТИВНА" + Color::RESET);

        std::string holderUpper = cardHolder;
        for (char& c : holderUpper) c = (char)::toupper((unsigned char)c);
        if (holderUpper.length() > 24) holderUpper = holderUpper.substr(0, 21) + "...";
        while (holderUpper.length() < 24) holderUpper += " ";

        std::string balStr = formatMoney(balance, currency);
        while (balStr.length() < 16) balStr += " ";

        std::cout << "\n"
                  << "  " << Color::BLUE << "╭──────────────────────────────────────────────────────────────╮\n"
                  << "  │ " << Color::BRIGHT_WHITE << Color::BOLD << "GEMINI PRIVATE BANKING" 
                  << Color::BLUE << "                      " 
                  << Color::BRIGHT_YELLOW << "VISA " << Color::WHITE << "Infinite" << Color::BLUE << " │\n"
                  << "  │                                                              │\n"
                  << "  │   " << Color::BRIGHT_YELLOW << "[ ▣ CHIP ]" 
                  << Color::BLUE << "                 " 
                  << Color::CYAN << "•))) Contactless Pay" << Color::BLUE << "            │\n"
                  << "  │                                                              │\n"
                  << "  │   " << Color::BRIGHT_WHITE << Color::BOLD << cardNumber << Color::BLUE << "                               │\n"
                  << "  │   " << Color::DIM << "CVV: •••" << Color::RESET << Color::BLUE << "   "
                  << Color::WHITE << "ТЕРМІН: " << expiry << Color::BLUE << "       "
                  << "СТАТУС: " << statusBadge << Color::BLUE << "       │\n"
                  << "  │   " << Color::BRIGHT_CYAN << holderUpper << Color::BLUE << " "
                  << Color::BRIGHT_GREEN << Color::BOLD << balStr << Color::BLUE << " │\n"
                  << "  ╰──────────────────────────────────────────────────────────────╯" 
                  << Color::RESET << "\n\n";
    }

    void renderReceipt(const std::string& txId,
                       const std::string& typeName,
                       double amount,
                       double balanceAfter,
                       const std::string& counterparty,
                       const std::string& description,
                       const std::string& timestamp) {
        std::cout << "\n" << Color::BRIGHT_CYAN
                  << "  ╔══════════════════════════════════════════════════════════════╗\n"
                  << "  ║                 ФІСКАЛЬНИЙ ЧЕК ТРАНЗАКЦІЇ                    ║\n"
                  << "  ║                   GEMINI NATIONAL BANK                       ║\n"
                  << "  ╠══════════════════════════════════════════════════════════════╣\n"
                  << Color::RESET;

        std::cout << "  ║  " << Color::WHITE << "ID транзакції:  " << Color::BRIGHT_YELLOW << std::left << std::setw(39) << txId << Color::RESET << Color::BRIGHT_CYAN << "║\n";
        std::cout << "  ║  " << Color::WHITE << "Дата та час:    " << Color::BRIGHT_WHITE  << std::left << std::setw(39) << timestamp << Color::RESET << Color::BRIGHT_CYAN << "║\n";
        std::cout << "  ║  " << Color::WHITE << "Тип операції:   " << Color::BRIGHT_MAGENTA<< std::left << std::setw(39) << typeName << Color::RESET << Color::BRIGHT_CYAN << "║\n";
        
        std::string cp = counterparty;
        if (cp.length() > 38) cp = cp.substr(0, 35) + "...";
        std::cout << "  ║  " << Color::WHITE << "Контрагент:     " << Color::BRIGHT_CYAN   << std::left << std::setw(39) << cp << Color::RESET << Color::BRIGHT_CYAN << "║\n";

        std::cout << "  ║  " << Color::WHITE << "Сума операції:  " << Color::BRIGHT_GREEN << Color::BOLD << std::left << std::setw(39) << formatMoney(amount) << Color::RESET << Color::BRIGHT_CYAN << "║\n";
        std::cout << "  ║  " << Color::WHITE << "Комісія банку:  " << Color::GREEN        << std::left << std::setw(39) << "0.00 грн (0%)" << Color::RESET << Color::BRIGHT_CYAN << "║\n";
        std::cout << "  ║  " << Color::WHITE << "Залишок коштів: " << Color::BRIGHT_WHITE  << std::left << std::setw(39) << formatMoney(balanceAfter) << Color::RESET << Color::BRIGHT_CYAN << "║\n";

        std::string desc = description;
        if (desc.length() > 38) desc = desc.substr(0, 35) + "...";
        std::cout << "  ║  " << Color::WHITE << "Призначення:    " << Color::DIM           << std::left << std::setw(39) << desc << Color::RESET << Color::BRIGHT_CYAN << "║\n";
        std::cout << "  ║  " << Color::WHITE << "Результат:      " << Color::BRIGHT_GREEN  << std::left << std::setw(39) << "[✓ УСПІШНО ПРОВЕДЕНО]" << Color::RESET << Color::BRIGHT_CYAN << "║\n";

        std::cout << Color::BRIGHT_CYAN
                  << "  ╚══════════════════════════════════════════════════════════════╝\n"
                  << Color::RESET << "\n";
    }

    void pause() {
        std::cout << "\n" << Color::CYAN << "Натисніть Enter для продовження..." << Color::RESET;
        std::string dummy;
        if (!std::getline(std::cin, dummy)) {
            if (std::cin.eof()) exit(0);
        }
    }

    std::string escapeJSON(const std::string& s) {
        std::ostringstream o;
        for (char c : s) {
            switch (c) {
                case '"': o << "\\\""; break;
                case '\\': o << "\\\\"; break;
                case '\b': o << "\\b"; break;
                case '\f': o << "\\f"; break;
                case '\n': o << "\\n"; break;
                case '\r': o << "\\r"; break;
                case '\t': o << "\\t"; break;
                default:
                    if ('\x00' <= c && c <= '\x1f') {
                        o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)c;
                    } else {
                        o << c;
                    }
            }
        }
        return o.str();
    }

    std::string unescapeJSON(const std::string& s) {
        std::string res;
        for (size_t i = 0; i < s.length(); ++i) {
            if (s[i] == '\\' && i + 1 < s.length()) {
                char next = s[++i];
                switch (next) {
                    case '"': res += '"'; break;
                    case '\\': res += '\\'; break;
                    case 'n': res += '\n'; break;
                    case 'r': res += '\r'; break;
                    case 't': res += '\t'; break;
                    case 'b': res += '\b'; break;
                    case 'f': res += '\f'; break;
                    default: res += next; break;
                }
            } else {
                res += s[i];
            }
        }
        return res;
    }

} // namespace Utils
} // namespace Banking
