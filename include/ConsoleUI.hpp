#pragma once

#include "BankService.hpp"

namespace Banking {

class ConsoleUI {
private:
    BankService& bank_;
    Account* currentUser_{nullptr};

    void showHeader(const std::string& title);
    void handleLogin();
    void handleRegister();
    void handleAbout();

    // Client Views
    void runClientMenu();
    void viewAccountDetails();
    void handleDeposit();
    void handleWithdraw();
    void handleTransfer();
    void viewTransactionHistory();
    void handleExportStatement();
    void handleChangePassword();

    // Admin Views
    void runAdminMenu();
    void viewAllClients();
    void viewBankReserves();
    void handleFindClient();
    void handleToggleFreeze();
    void viewSystemAuditLog();

public:
    explicit ConsoleUI(BankService& bank);
    void run();
};

} // namespace Banking
