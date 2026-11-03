#pragma once

#include <string>
#include <vector>
#include "Transaction.hpp"

namespace Banking {

class Account {
private:
    std::string accountNumber_; // IBAN
    std::string cardNumber_;    // 16-digit card number
    std::string username_;      // Unique login
    std::string fullName_;      // Client full name
    std::string passwordHash_;  // SHA-256 hash
    std::string phone_;         // Phone number
    std::string email_;         // Email address
    double balance_{0.0};       // Account balance
    std::string currency_{"UAH"};
    bool isFrozen_{false};
    bool isAdmin_{false};
    std::string createdAt_;
    std::vector<Transaction> transactions_;

public:
    Account() = default;
    Account(std::string accountNumber,
            std::string cardNumber,
            std::string username,
            std::string fullName,
            std::string passwordHash,
            std::string phone,
            std::string email,
            double initialBalance,
            std::string currency,
            bool isFrozen,
            bool isAdmin,
            std::string createdAt);

    // Getters
    const std::string& getAccountNumber() const { return accountNumber_; }
    const std::string& getCardNumber() const { return cardNumber_; }
    const std::string& getUsername() const { return username_; }
    const std::string& getFullName() const { return fullName_; }
    const std::string& getPasswordHash() const { return passwordHash_; }
    const std::string& getPhone() const { return phone_; }
    const std::string& getEmail() const { return email_; }
    double getBalance() const { return balance_; }
    const std::string& getCurrency() const { return currency_; }
    bool isFrozen() const { return isFrozen_; }
    bool isAdmin() const { return isAdmin_; }
    const std::string& getCreatedAt() const { return createdAt_; }
    const std::vector<Transaction>& getTransactions() const { return transactions_; }

    // Mutators & Security
    void setFullName(const std::string& name) { fullName_ = name; }
    void setPhone(const std::string& phone) { phone_ = phone; }
    void setEmail(const std::string& email) { email_ = email; }
    void setFrozen(bool frozen) { isFrozen_ = frozen; }
    void setPassword(const std::string& plainPassword);
    bool verifyPassword(const std::string& plainPassword) const;

    // Banking Operations
    bool deposit(double amount, const std::string& counterparty, const std::string& description);
    bool withdraw(double amount, const std::string& counterparty, const std::string& description);
    void addTransaction(const Transaction& tx);

    // Serialization
    std::string toJSON() const;
    static Account fromJSON(const std::string& jsonStr);
};

} // namespace Banking
