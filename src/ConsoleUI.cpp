#include "ConsoleUI.hpp"
#include "Utils.hpp"
#include <iostream>
#include <iomanip>

namespace Banking {

using namespace Utils;

ConsoleUI::ConsoleUI(BankService& bank)
    : bank_(bank) {}

void ConsoleUI::showHeader(const std::string& title) {
    clearScreen();
    renderBankLogo();

    if (!title.empty()) {
        std::cout << "  " << Color::BG_BLUE << Color::BRIGHT_WHITE << Color::BOLD 
                  << "  ● " << title << "  " << Color::RESET << "\n\n";
    }
}

void ConsoleUI::run() {
    while (true) {
        showHeader("ГОЛОВНЕ МЕНЮ БАНКУ");

        std::cout << "  " << Color::CYAN << "╭─────────────────────────────────────────────────────────────╮\n"
                  << "  │ " << Color::WHITE << Color::BOLD << "Оберіть розділ для продовження роботи:                     " << Color::CYAN << "│\n"
                  << "  ├──────┬──────────────────────────────────────────────────────┤\n"
                  << "  │ " << Color::BRIGHT_GREEN  << "[1]" << Color::CYAN << "  │  " << Color::BRIGHT_GREEN << "Увійти в особистий кабінет клієнта / адміна          " << Color::CYAN << "│\n"
                  << "  │ " << Color::BRIGHT_CYAN   << "[2]" << Color::CYAN << "  │  " << Color::BRIGHT_CYAN  << "Відкрити новий рахунок (Онлайн реєстрація)          " << Color::CYAN << "│\n"
                  << "  │ " << Color::BRIGHT_YELLOW << "[3]" << Color::CYAN << "  │  " << Color::WHITE        << "Про систему та тестові акаунти                      " << Color::CYAN << "│\n"
                  << "  │ " << Color::BRIGHT_RED    << "[4]" << Color::CYAN << "  │  " << Color::BRIGHT_RED   << "Завершити роботу з банком                           " << Color::CYAN << "│\n"
                  << "  ╰──────┴──────────────────────────────────────────────────────╯" << Color::RESET << "\n\n";

        int choice = readInt("  Ваш вибір (1-4): ", 1, 4);

        if (choice == 1) {
            handleLogin();
        } else if (choice == 2) {
            handleRegister();
        } else if (choice == 3) {
            handleAbout();
        } else if (choice == 4) {
            clearScreen();
            std::cout << Color::BRIGHT_GREEN << "\n  [✓] Дякуємо за користування Gemini Bank! Бажаємо фінансового процвітання!\n\n" << Color::RESET;
            break;
        }
    }
}

void ConsoleUI::handleLogin() {
    showHeader("АВТОРИЗАЦІЯ КЛІЄНТА ТА АДМІНІСТРАТОРА");
    std::string username = readString("Введіть логін: ");
    std::string password = readPassword("Введіть пароль: ");

    Account* user = bank_.login(username, password);
    if (!user) {
        std::cout << "\n" << Color::RED << "[!] Помилка: Невірний логін або пароль." << Color::RESET << "\n";
        pause();
        return;
    }

    currentUser_ = user;
    if (currentUser_->isAdmin()) {
        runAdminMenu();
    } else {
        runClientMenu();
    }
    currentUser_ = nullptr;
}

void ConsoleUI::handleRegister() {
    showHeader("ВІДКРИТТЯ НОВОГО РАХУНКУ");

    std::cout << "Заповніть інформацію для відкриття банківського рахунку:\n\n";
    std::string fullName = readString("ПІБ клієнта: ");
    std::string username = readString("Бажаний логін: ");
    std::string phone    = readString("Номер телефону (+380...): ");
    std::string email    = readString("Email: ");
    std::string password = readPassword("Встановіть пароль: ");
    std::string confirm  = readPassword("Підтвердіть пароль: ");

    if (password != confirm) {
        std::cout << "\n" << Color::RED << "[!] Помилка: Паролі не співпадають!" << Color::RESET << "\n";
        pause();
        return;
    }

    double initialDeposit = readDouble("Початковий внесок (грн, від 0): ", 0.0, 10000000.0);

    std::string outIBAN, outCard;
    OperationResult res = bank_.registerAccount(username, fullName, password, phone, email, initialDeposit, outIBAN, outCard);

    if (res == OperationResult::SUCCESS) {
        std::cout << "\n" << Color::GREEN << Color::BOLD << "[✓] Рахунок успішно відкрито!" << Color::RESET << "\n";
        std::cout << "─────────────────────────────────────────────────────────────\n";
        std::cout << " Власник:       " << fullName << "\n";
        std::cout << " IBAN рахунку:  " << Color::YELLOW << outIBAN << Color::RESET << "\n";
        std::cout << " Номер картки:  " << Color::YELLOW << outCard << Color::RESET << "\n";
        std::cout << " Баланс:        " << Color::GREEN  << formatMoney(initialDeposit) << Color::RESET << "\n";
        std::cout << "─────────────────────────────────────────────────────────────\n";
        std::cout << "Збережіть ці реквізити для здійснення операцій.\n";
    } else {
        std::cout << "\n" << Color::RED << "[!] " << getResultMessage(res) << Color::RESET << "\n";
    }
    pause();
}

void ConsoleUI::handleAbout() {
    showHeader("ДОВІДКА ТА ТЕСТОВІ АКАУНТИ");
    std::cout << "Система Gemini Bank розроблена на C++20 з повною підтримкою:\n";
    std::cout << " • Збереження стану в JSON форматі (файл: data/bank_data.json)\n";
    std::cout << " • Хешування паролів за стандартом SHA-256\n";
    std::cout << " • Банківські операції: поповнення, зняття, миттєві перекази\n";
    std::cout << " • Експорт офіційних банківських виписок у текстові файли\n\n";

    std::cout << Color::BOLD << "Передвстановлені тестові акаунти:" << Color::RESET << "\n";
    std::cout << " 1. Клієнт 1: логін " << Color::CYAN << "user1" << Color::RESET 
              << ", пароль: " << Color::YELLOW << "1234" << Color::RESET << " (Коваленко О.С.)\n";
    std::cout << " 2. Клієнт 2: логін " << Color::CYAN << "user2" << Color::RESET 
              << ", пароль: " << Color::YELLOW << "1234" << Color::RESET << " (Мельник М.І.)\n";
    std::cout << " 3. Адмін:    логін " << Color::CYAN << "admin" << Color::RESET 
              << ", пароль: " << Color::YELLOW << "admin123" << Color::RESET << "\n";

    pause();
}

// =========================================================================
// КЛІЄНТСЬКА ЧАСТИНА
// =========================================================================

void ConsoleUI::runClientMenu() {
    while (true) {
        std::string title = "ОСОБИСТИЙ КАБІНЕТ КЛІЄНТА | " + currentUser_->getFullName();
        showHeader(title);

        // Render visual plastic bank card
        renderCard(currentUser_->getCardNumber(), 
                   currentUser_->getFullName(), 
                   "12/29", 
                   currentUser_->getBalance(), 
                   currentUser_->getCurrency(), 
                   currentUser_->isFrozen());

        std::cout << "  " << Color::CYAN << "╭─────────────────────────────────────────────────────────────────────────────╮\n"
                  << "  │ " << Color::BRIGHT_WHITE << Color::BOLD << "ПАНЕЛЬ УПРАВЛІННЯ РАХУНКОМ ТА ОПЕРАЦІЯМИ                                  " << Color::CYAN << "│\n"
                  << "  ├──────┬──────────────────────────────────────────────────────────────────────┤\n"
                  << "  │ " << Color::BRIGHT_YELLOW << "[1]" << Color::CYAN << "  │  " << Color::WHITE << "💳  Повні реквізити рахунку (IBAN, картка, договір)                 " << Color::CYAN << "│\n"
                  << "  │ " << Color::BRIGHT_GREEN  << "[2]" << Color::CYAN << "  │  " << Color::BRIGHT_GREEN << "💵  Поповнити картку (каса / термінал)                              " << Color::CYAN << "│\n"
                  << "  │ " << Color::BRIGHT_CYAN   << "[3]" << Color::CYAN << "  │  " << Color::BRIGHT_CYAN  << "🏧  Зняти готівку в банкоматі (ATM)                                 " << Color::CYAN << "│\n"
                  << "  │ " << Color::BRIGHT_MAGENTA<< "[4]" << Color::CYAN << "  │  " << Color::BRIGHT_MAGENTA<< "🔄  Миттєвий переказ коштів (за карткою, IBAN або логіном)         " << Color::CYAN << "│\n"
                  << "  │ " << Color::BRIGHT_WHITE  << "[5]" << Color::CYAN << "  │  " << Color::WHITE << "📜  Історія операцій та квитанції                                   " << Color::CYAN << "│\n"
                  << "  │ " << Color::BRIGHT_BLUE   << "[6]" << Color::CYAN << "  │  " << Color::WHITE << "📄  Завантажити офіційну виписку (.txt)                             " << Color::CYAN << "│\n"
                  << "  │ " << Color::YELLOW        << "[7]" << Color::CYAN << "  │  " << Color::WHITE << "🔐  Змінити пароль акаунта                                          " << Color::CYAN << "│\n"
                  << "  │ " << Color::BRIGHT_RED    << "[8]" << Color::CYAN << "  │  " << Color::BRIGHT_RED << "🚪  Вийти з особистого кабінету                                     " << Color::CYAN << "│\n"
                  << "  ╰──────┴──────────────────────────────────────────────────────────────────────╯" << Color::RESET << "\n\n";

        int choice = readInt("  Оберіть пункт меню (1-8): ", 1, 8);

        if (choice == 1) {
            viewAccountDetails();
        } else if (choice == 2) {
            handleDeposit();
        } else if (choice == 3) {
            handleWithdraw();
        } else if (choice == 4) {
            handleTransfer();
        } else if (choice == 5) {
            viewTransactionHistory();
        } else if (choice == 6) {
            handleExportStatement();
        } else if (choice == 7) {
            handleChangePassword();
        } else if (choice == 8) {
            break;
        }
    }
}

void ConsoleUI::viewAccountDetails() {
    showHeader("РЕКВІЗИТИ РАХУНКУ");
    std::cout << " Власник:           " << currentUser_->getFullName() << "\n";
    std::cout << " Номер картки:      " << Color::YELLOW << currentUser_->getCardNumber() << Color::RESET << "\n";
    std::cout << " IBAN (Рахунок):    " << Color::YELLOW << currentUser_->getAccountNumber() << Color::RESET << "\n";
    std::cout << " Логін у системі:   " << currentUser_->getUsername() << "\n";
    std::cout << " Телефон:           " << currentUser_->getPhone() << "\n";
    std::cout << " Email:             " << currentUser_->getEmail() << "\n";
    std::cout << " Поточний баланс:   " << Color::GREEN << Color::BOLD << formatMoney(currentUser_->getBalance(), currentUser_->getCurrency()) << Color::RESET << "\n";
    std::cout << " Статус рахунку:    " << (currentUser_->isFrozen() ? Color::RED + "ЗАБЛОКОВАНО" : Color::GREEN + "АКТИВНИЙ") << Color::RESET << "\n";
    std::cout << " Дата відкриття:    " << currentUser_->getCreatedAt() << "\n";
    pause();
}

void ConsoleUI::handleDeposit() {
    showHeader("ПОПОВНЕННЯ БАЛАНСУ");
    if (currentUser_->isFrozen()) {
        std::cout << Color::RED << "[!] Ваш рахунок заблоковано. Операції неможливі." << Color::RESET << "\n";
        pause();
        return;
    }

    double amount = readDouble("Введіть суму поповнення (грн): ", 1.0, 500000.0);
    std::string desc = readString("Призначення платежу (або Enter для стандартного): ", true);
    if (desc.empty()) desc = "Поповнення готівкою через термінал";

    OperationResult res = bank_.deposit(currentUser_, amount, desc);
    if (res == OperationResult::SUCCESS) {
        const auto& txs = currentUser_->getTransactions();
        std::string txId = txs.empty() ? "TX-AUTO" : txs.back().getId();
        renderReceipt(txId, "ПОПОВНЕННЯ РАХУНКУ", amount, currentUser_->getBalance(), "Каса / Термінал", desc, getCurrentDateTime());
    } else {
        std::cout << "\n" << Color::RED << "[!] " << getResultMessage(res) << Color::RESET << "\n";
    }
    pause();
}

void ConsoleUI::handleWithdraw() {
    showHeader("ЗНЯТТЯ ГОТІВКИ");
    if (currentUser_->isFrozen()) {
        std::cout << Color::RED << "[!] Ваш рахунок заблоковано. Операції неможливі." << Color::RESET << "\n";
        pause();
        return;
    }

    std::cout << "Доступно до зняття: " << Color::GREEN << formatMoney(currentUser_->getBalance()) << Color::RESET << "\n\n";
    double amount = readDouble("Введіть суму для зняття (грн): ", 1.0, 50000.0);

    if (amount > currentUser_->getBalance()) {
        std::cout << "\n" << Color::RED << "[!] Помилка: Недостатньо коштів на рахунку!" << Color::RESET << "\n";
        pause();
        return;
    }

    OperationResult res = bank_.withdraw(currentUser_, amount, "Зняття в банкоматі Gemini ATM");
    if (res == OperationResult::SUCCESS) {
        const auto& txs = currentUser_->getTransactions();
        std::string txId = txs.empty() ? "TX-AUTO" : txs.back().getId();
        renderReceipt(txId, "ЗНЯТТЯ ГОТІВКИ В ATM", amount, currentUser_->getBalance(), "Банкомат Gemini ATM", "Видача готівки", getCurrentDateTime());
    } else {
        std::cout << "\n" << Color::RED << "[!] " << getResultMessage(res) << Color::RESET << "\n";
    }
    pause();
}

void ConsoleUI::handleTransfer() {
    showHeader("ПЕРЕКАЗ КОШТІВ");
    if (currentUser_->isFrozen()) {
        std::cout << Color::RED << "[!] Ваш рахунок заблоковано. Операції неможливі." << Color::RESET << "\n";
        pause();
        return;
    }

    std::cout << "Доступний баланс: " << Color::GREEN << formatMoney(currentUser_->getBalance()) << Color::RESET << "\n\n";
    std::string recipientQuery = readString("Введіть номер картки, IBAN або логін отримувача: ");
    double amount = readDouble("Сума переказу (грн): ", 1.0, 100000.0);
    std::string comment = readString("Коментар до переказу (або Enter): ", true);
    if (comment.empty()) comment = "Приватний переказ";

    std::cout << "\nПідтверджуєте переказ " << Color::YELLOW << formatMoney(amount) << Color::RESET
              << " для " << Color::CYAN << recipientQuery << Color::RESET << "? (1 - Так, 0 - Скасувати): ";
    int confirm = readInt("", 0, 1);
    if (confirm == 0) {
        std::cout << "\n" << Color::YELLOW << "Переказ скасовано користувачем." << Color::RESET << "\n";
        pause();
        return;
    }

    OperationResult res = bank_.transfer(currentUser_, recipientQuery, amount, comment);
    if (res == OperationResult::SUCCESS) {
        const auto& txs = currentUser_->getTransactions();
        std::string txId = txs.empty() ? "TX-AUTO" : txs.back().getId();
        renderReceipt(txId, "МИТТЄВИЙ ПЕРЕКАЗ", amount, currentUser_->getBalance(), recipientQuery, comment, getCurrentDateTime());
    } else {
        std::cout << "\n" << Color::RED << "[!] " << getResultMessage(res) << Color::RESET << "\n";
    }
    pause();
}

void ConsoleUI::viewTransactionHistory() {
    showHeader("ІСТОРІЯ ОПЕРАЦІЙ");
    const auto& txs = currentUser_->getTransactions();

    if (txs.empty()) {
        std::cout << "У вас поки що немає проведених транзакцій.\n";
        pause();
        return;
    }

    std::cout << "Всього транзакцій: " << txs.size() << "\n";
    std::cout << "──────────────────────────────────────────────────────────────────────────────────────────\n";
    std::cout << std::left 
              << std::setw(20) << "Дата і час"
              << std::setw(16) << "Тип"
              << std::setw(15) << "Сума"
              << std::setw(15) << "Залишок"
              << std::setw(26) << "Деталі" << "\n";
    std::cout << "──────────────────────────────────────────────────────────────────────────────────────────\n";

    for (auto it = txs.rbegin(); it != txs.rend(); ++it) {
        const auto& tx = *it;
        std::string typeLabel;
        std::string colorCode = Color::WHITE;
        std::string sign = "";

        switch (tx.getType()) {
            case TransactionType::DEPOSIT:
                typeLabel = "Поповнення";
                colorCode = Color::GREEN;
                sign = "+";
                break;
            case TransactionType::WITHDRAW:
                typeLabel = "Зняття";
                colorCode = Color::RED;
                sign = "-";
                break;
            case TransactionType::TRANSFER_OUT:
                typeLabel = "Переказ (вих)";
                colorCode = Color::RED;
                sign = "-";
                break;
            case TransactionType::TRANSFER_IN:
                typeLabel = "Переказ (вхід)";
                colorCode = Color::GREEN;
                sign = "+";
                break;
            default:
                typeLabel = "Операція";
                break;
        }

        std::string sumStr = sign + formatMoney(tx.getAmount(), "");
        std::string desc = tx.getCounterparty() + " / " + tx.getDescription();
        if (desc.length() > 25) desc = desc.substr(0, 22) + "...";

        std::cout << std::left
                  << std::setw(20) << tx.getTimestamp()
                  << colorCode << std::setw(16) << typeLabel << Color::RESET
                  << colorCode << std::setw(15) << sumStr << Color::RESET
                  << std::setw(15) << formatMoney(tx.getBalanceAfter(), "")
                  << std::setw(26) << desc << "\n";
    }
    std::cout << "──────────────────────────────────────────────────────────────────────────────────────────\n";
    pause();
}

void ConsoleUI::handleExportStatement() {
    showHeader("ЕКСПОРТ БАНКІВСЬКОЇ ВИПИСКИ");
    std::string defaultName = "statement_" + currentUser_->getUsername() + ".txt";
    std::cout << "Файл буде збережено у поточну папку.\n";
    std::cout << "Ім'я файлу за замовчуванням: " << Color::CYAN << defaultName << Color::RESET << "\n\n";

    std::string fileName = readString("Введіть ім'я файлу (або натисніть Enter): ", true);
    if (fileName.empty()) {
        fileName = defaultName;
    }

    if (bank_.exportStatement(*currentUser_, fileName)) {
        std::cout << "\n" << Color::GREEN << "[✓] Виписку успішно експортовано у файл: " 
                  << fileName << Color::RESET << "\n";
    } else {
        std::cout << "\n" << Color::RED << "[!] Помилка запису файлу виписки." << Color::RESET << "\n";
    }
    pause();
}

void ConsoleUI::handleChangePassword() {
    showHeader("ЗМІНА ПАРОЛЯ");
    std::string oldPass = readPassword("Введіть поточний пароль: ");
    std::string newPass = readPassword("Введіть новий пароль: ");
    std::string confirm = readPassword("Підтвердіть новий пароль: ");

    if (newPass != confirm) {
        std::cout << "\n" << Color::RED << "[!] Новий пароль та підтвердження не співпадають!" << Color::RESET << "\n";
        pause();
        return;
    }

    OperationResult res = bank_.changePassword(currentUser_, oldPass, newPass);
    if (res == OperationResult::SUCCESS) {
        std::cout << "\n" << Color::GREEN << "[✓] Пароль успішно змінено!" << Color::RESET << "\n";
    } else {
        std::cout << "\n" << Color::RED << "[!] " << getResultMessage(res) << Color::RESET << "\n";
    }
    pause();
}

// =========================================================================
// АДМІНІСТРАТИВНА ЧАСТИНА
// =========================================================================

void ConsoleUI::runAdminMenu() {
    while (true) {
        showHeader("ПАНЕЛЬ ГОЛОВНОГО АДМІНІСТРАТОРА БАНКУ");

        std::cout << "  " << Color::CYAN << "╭─────────────────────────────────────────────────────────────────────────────╮\n"
                  << "  │ " << Color::BRIGHT_WHITE << Color::BOLD << "ГОЛОВНЕ УПРАВЛІННЯ ТА ФІНАНСОВИЙ МОНІТОРИНГ                                " << Color::CYAN << "│\n"
                  << "  ├──────┬──────────────────────────────────────────────────────────────────────┤\n"
                  << "  │ " << Color::BRIGHT_GREEN  << "[1]" << Color::CYAN << "  │  " << Color::WHITE << "👥  Список усіх зареєстрованих клієнтів банку                        " << Color::CYAN << "│\n"
                  << "  │ " << Color::BRIGHT_YELLOW << "[2]" << Color::CYAN << "  │  " << Color::WHITE << "💰  Загальний капітал та ліквідність банківського сховища           " << Color::CYAN << "│\n"
                  << "  │ " << Color::BRIGHT_CYAN   << "[3]" << Color::CYAN << "  │  " << Color::WHITE << "🔍  Пошук клієнта за номером картки, IBAN або телефоном             " << Color::CYAN << "│\n"
                  << "  │ " << Color::BRIGHT_RED    << "[4]" << Color::CYAN << "  │  " << Color::WHITE << "❄️   Блокування або розблокування рахунку клієнта                    " << Color::CYAN << "│\n"
                  << "  │ " << Color::BRIGHT_MAGENTA<< "[5]" << Color::CYAN << "  │  " << Color::WHITE << "📊  Глобальний журнал аудиту всіх банківських транзакцій            " << Color::CYAN << "│\n"
                  << "  │ " << Color::BRIGHT_BLUE   << "[6]" << Color::CYAN << "  │  " << Color::WHITE << "➕  Створити новий рахунок через відділення банку                   " << Color::CYAN << "│\n"
                  << "  │ " << Color::RED           << "[7]" << Color::CYAN << "  │  " << Color::BRIGHT_RED << "🚪  Вийти з панелі адміністратора                                    " << Color::CYAN << "│\n"
                  << "  ╰──────┴──────────────────────────────────────────────────────────────────────╯" << Color::RESET << "\n\n";

        int choice = readInt("  Оберіть пункт меню (1-7): ", 1, 7);

        if (choice == 1) {
            viewAllClients();
        } else if (choice == 2) {
            viewBankReserves();
        } else if (choice == 3) {
            handleFindClient();
        } else if (choice == 4) {
            handleToggleFreeze();
        } else if (choice == 5) {
            viewSystemAuditLog();
        } else if (choice == 6) {
            handleRegister();
        } else if (choice == 7) {
            break;
        }
    }
}

void ConsoleUI::viewAllClients() {
    showHeader("СПИСОК КЛІЄНТІВ БАНКУ");
    const auto& accounts = bank_.getAllAccounts();

    std::cout << "Всього облікових записів: " << accounts.size() << "\n\n";
    std::cout << "────────────────────────────────────────────────────────────────────────────────────────────────────────────\n";
    std::cout << std::left
              << std::setw(12) << "Логін"
              << std::setw(28) << "ПІБ"
              << std::setw(22) << "Номер картки"
              << std::setw(18) << "Баланс"
              << std::setw(14) << "Статус"
              << std::setw(10) << "Роль" << "\n";
    std::cout << "────────────────────────────────────────────────────────────────────────────────────────────────────────────\n";

    for (const auto& acc : accounts) {
        std::string status = acc.isFrozen() ? Color::RED + "БЛОК" + Color::RESET : Color::GREEN + "АКТИВ" + Color::RESET;
        std::string role = acc.isAdmin() ? Color::YELLOW + "Admin" + Color::RESET : "Client";

        std::string name = acc.getFullName();
        if (name.length() > 26) name = name.substr(0, 23) + "...";

        std::cout << std::left
                  << std::setw(12) << acc.getUsername()
                  << std::setw(28) << name
                  << std::setw(22) << acc.getCardNumber()
                  << std::setw(18) << formatMoney(acc.getBalance())
                  << std::setw(22) << status
                  << std::setw(18) << role << "\n";
    }
    std::cout << "────────────────────────────────────────────────────────────────────────────────────────────────────────────\n";
    pause();
}

void ConsoleUI::viewBankReserves() {
    showHeader("ФІНАНСОВИЙ ЗВІТ БАНКУ");
    double total = bank_.getTotalReserves();
    const auto& accounts = bank_.getAllAccounts();

    size_t activeCount = 0;
    size_t frozenCount = 0;
    for (const auto& acc : accounts) {
        if (!acc.isAdmin()) {
            if (acc.isFrozen()) frozenCount++;
            else activeCount++;
        }
    }

    std::cout << " Загальний депозитний портфель (Капітал): " 
              << Color::GREEN << Color::BOLD << formatMoney(total) << Color::RESET << "\n\n";
    std::cout << " Кількість активних клієнтів: " << Color::CYAN << activeCount << Color::RESET << "\n";
    std::cout << " Кількість заблокованих рахунків: " << Color::RED << frozenCount << Color::RESET << "\n";
    std::cout << " Файл сховища даних: " << Color::YELLOW << "data/bank_data.json" << Color::RESET << "\n";
    pause();
}

void ConsoleUI::handleFindClient() {
    showHeader("ПОШУК КЛІЄНТА");
    std::string query = readString("Введіть логін, IBAN, номер картки або телефон: ");
    Account* acc = bank_.findAccountByIdentifier(query);

    if (!acc) {
        std::cout << "\n" << Color::RED << "[!] Клієнта за вказаними реквізитами не знайдено." << Color::RESET << "\n";
    } else {
        std::cout << "\n" << Color::GREEN << "[✓] Клієнта знайдено:" << Color::RESET << "\n";
        std::cout << " ПІБ:        " << acc->getFullName() << "\n";
        std::cout << " Логін:      " << acc->getUsername() << "\n";
        std::cout << " Номер картки: " << acc->getCardNumber() << "\n";
        std::cout << " IBAN:       " << acc->getAccountNumber() << "\n";
        std::cout << " Телефон:    " << acc->getPhone() << "\n";
        std::cout << " Email:      " << acc->getEmail() << "\n";
        std::cout << " Баланс:     " << formatMoney(acc->getBalance()) << "\n";
        std::cout << " Статус:     " << (acc->isFrozen() ? Color::RED + "ЗАБЛОКОВАНИЙ" : Color::GREEN + "АКТИВНИЙ") << Color::RESET << "\n";
        std::cout << " Кількість транзакцій: " << acc->getTransactions().size() << "\n";
    }
    pause();
}

void ConsoleUI::handleToggleFreeze() {
    showHeader("БЛОКУВАННЯ / РОЗБЛОКУВАННЯ РАХУНКУ");
    std::string query = readString("Введіть IBAN, логін або номер картки клієнта: ");
    Account* acc = bank_.findAccountByIdentifier(query);

    if (!acc) {
        std::cout << "\n" << Color::RED << "[!] Рахунок не знайдено." << Color::RESET << "\n";
        pause();
        return;
    }

    if (acc->isAdmin()) {
        std::cout << "\n" << Color::RED << "[!] Неможливо заблокувати обліковий запис адміністратора!" << Color::RESET << "\n";
        pause();
        return;
    }

    std::cout << "\nКлієнт: " << acc->getFullName() << " (Поточний стан: " 
              << (acc->isFrozen() ? Color::RED + "ЗАБЛОКОВАНИЙ" : Color::GREEN + "АКТИВНИЙ") << Color::RESET << ")\n";
    std::cout << "Оберіть дію:\n";
    std::cout << " [1] Заблокувати рахунок\n";
    std::cout << " [2] Розблокувати рахунок\n";
    std::cout << " [0] Скасувати\n";

    int action = readInt("Ваш вибір (0-2): ", 0, 2);
    if (action == 1) {
        bank_.setAccountFrozen(acc->getAccountNumber(), true);
        std::cout << "\n" << Color::RED << "[✓] Рахунок успішно заблоковано." << Color::RESET << "\n";
    } else if (action == 2) {
        bank_.setAccountFrozen(acc->getAccountNumber(), false);
        std::cout << "\n" << Color::GREEN << "[✓] Рахунок успішно розблоковано." << Color::RESET << "\n";
    }
    pause();
}

void ConsoleUI::viewSystemAuditLog() {
    showHeader("ЗАГАЛЬНИЙ ЖУРНАЛ АУДИТУ ТРАНЗАКЦІЙ");
    auto allTxs = bank_.getAllSystemTransactions();

    if (allTxs.empty()) {
        std::cout << "Жодної операції ще не зафіксовано.\n";
        pause();
        return;
    }

    std::cout << "Всього операцій у системі: " << allTxs.size() << "\n\n";
    std::cout << "────────────────────────────────────────────────────────────────────────────────────────────────────────────\n";
    std::cout << std::left
              << std::setw(20) << "Дата і час"
              << std::setw(15) << "Тип"
              << std::setw(15) << "Сума"
              << std::setw(30) << "Рахунок власника"
              << std::setw(26) << "Контрагент / Опис" << "\n";
    std::cout << "────────────────────────────────────────────────────────────────────────────────────────────────────────────\n";

    for (const auto& item : allTxs) {
        const auto& ownerStr = item.first;
        const auto& tx = item.second;

        std::string sign = (tx.getType() == TransactionType::DEPOSIT || tx.getType() == TransactionType::TRANSFER_IN) ? "+" : "-";
        std::string oStr = ownerStr;
        if (oStr.length() > 28) oStr = oStr.substr(0, 25) + "...";

        std::string desc = tx.getCounterparty();
        if (desc.length() > 24) desc = desc.substr(0, 21) + "...";

        std::cout << std::left
                  << std::setw(20) << tx.getTimestamp()
                  << std::setw(15) << transactionTypeToString(tx.getType())
                  << std::setw(15) << (sign + formatMoney(tx.getAmount(), ""))
                  << std::setw(30) << oStr
                  << std::setw(26) << desc << "\n";
    }
    std::cout << "────────────────────────────────────────────────────────────────────────────────────────────────────────────\n";
    pause();
}

} // namespace Banking
