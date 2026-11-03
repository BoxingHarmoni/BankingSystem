#pragma once

#include <string>

namespace Banking {

enum class TransactionType {
    DEPOSIT,
    WITHDRAW,
    TRANSFER_OUT,
    TRANSFER_IN,
    SYSTEM
};

std::string transactionTypeToString(TransactionType type);
TransactionType stringToTransactionType(const std::string& str);

class Transaction {
private:
    std::string id_;
    TransactionType type_;
    double amount_;
    double balanceAfter_;
    std::string timestamp_;
    std::string counterparty_;
    std::string description_;

public:
    Transaction() = default;
    Transaction(std::string id,
                TransactionType type,
                double amount,
                double balanceAfter,
                std::string timestamp,
                std::string counterparty,
                std::string description);

    // Getters
    std::string getId() const { return id_; }
    TransactionType getType() const { return type_; }
    double getAmount() const { return amount_; }
    double getBalanceAfter() const { return balanceAfter_; }
    std::string getTimestamp() const { return timestamp_; }
    std::string getCounterparty() const { return counterparty_; }
    std::string getDescription() const { return description_; }

    // JSON Serialization
    std::string toJSON() const;
    static Transaction fromJSON(const std::string& jsonStr);
};

} // namespace Banking
