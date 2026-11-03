#pragma once

#include <vector>
#include <string>
#include <optional>
#include <cstdint>
#include "Account.hpp"
#include "StorageManager.hpp"

namespace Banking {

enum class OperationResult {
    SUCCESS,
    ACCOUNT_NOT_FOUND,
    INVALID_CREDENTIALS,
    INSUFFICIENT_FUNDS,
    ACCOUNT_FROZEN,
    RECIPIENT_FROZEN,
    SAME_ACCOUNT,
    INVALID_AMOUNT,
    USERNAME_TAKEN,
    STORAGE_ERROR
};

std::string getResultMessage(OperationResult result);

class BankService {
private:
    std::vector<Account> accounts_;
    StorageManager storage_;
    uint64_t nextAccountSeq_{1003};

    void updateNextSequence();

public:
    explicit BankService(std::string storagePath = "data/bank_data.json");

    bool initialize();
    bool save();

    // Authentication & Registration
    Account* login(const std::string& username, const std::string& password);
    OperationResult registerAccount(const std::string& username,
                                    const std::string& fullName,
                                    const std::string& password,
                                    const std::string& phone,
                                    const std::string& email,
                                    double initialDeposit,
                                    std::string& outIBAN,
                                    std::string& outCard);

    // Queries
    Account* findAccountByUsername(const std::string& username);
    Account* findAccountByNumber(const std::string& accNumber);
    Account* findAccountByCard(const std::string& cardNumber);
    Account* findAccountByIdentifier(const std::string& query); // username, IBAN or card number
    const std::vector<Account>& getAllAccounts() const { return accounts_; }

    // Banking Operations
    OperationResult deposit(Account* account, double amount, const std::string& description = "Поповнення через касу/термінал");
    OperationResult withdraw(Account* account, double amount, const std::string& description = "Зняття готівки");
    OperationResult transfer(Account* sender,
                             const std::string& recipientIdentifier,
                             double amount,
                             const std::string& description = "Приватний переказ");

    OperationResult changePassword(Account* account, const std::string& oldPassword, const std::string& newPassword);
    OperationResult setAccountFrozen(const std::string& accNumber, bool frozen);

    // Analytics / Admin
    double getTotalReserves() const;
    std::vector<std::pair<std::string, Transaction>> getAllSystemTransactions() const;
    bool exportStatement(const Account& account, const std::string& outPath);
};

} // namespace Banking
