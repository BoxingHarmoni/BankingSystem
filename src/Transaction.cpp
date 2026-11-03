#include "Transaction.hpp"
#include "Utils.hpp"
#include <sstream>
#include <iomanip>

namespace Banking {

std::string transactionTypeToString(TransactionType type) {
    switch (type) {
        case TransactionType::DEPOSIT:      return "DEPOSIT";
        case TransactionType::WITHDRAW:     return "WITHDRAW";
        case TransactionType::TRANSFER_OUT: return "TRANSFER_OUT";
        case TransactionType::TRANSFER_IN:  return "TRANSFER_IN";
        case TransactionType::SYSTEM:       return "SYSTEM";
    }
    return "UNKNOWN";
}

TransactionType stringToTransactionType(const std::string& str) {
    if (str == "DEPOSIT")      return TransactionType::DEPOSIT;
    if (str == "WITHDRAW")     return TransactionType::WITHDRAW;
    if (str == "TRANSFER_OUT") return TransactionType::TRANSFER_OUT;
    if (str == "TRANSFER_IN")  return TransactionType::TRANSFER_IN;
    return TransactionType::SYSTEM;
}

Transaction::Transaction(std::string id,
                         TransactionType type,
                         double amount,
                         double balanceAfter,
                         std::string timestamp,
                         std::string counterparty,
                         std::string description)
    : id_(std::move(id)),
      type_(type),
      amount_(amount),
      balanceAfter_(balanceAfter),
      timestamp_(std::move(timestamp)),
      counterparty_(std::move(counterparty)),
      description_(std::move(description)) {}

std::string Transaction::toJSON() const {
    std::ostringstream oss;
    oss << "{\n"
        << "        \"id\": \"" << Utils::escapeJSON(id_) << "\",\n"
        << "        \"type\": \"" << transactionTypeToString(type_) << "\",\n"
        << "        \"amount\": " << std::fixed << std::setprecision(2) << amount_ << ",\n"
        << "        \"balanceAfter\": " << std::fixed << std::setprecision(2) << balanceAfter_ << ",\n"
        << "        \"timestamp\": \"" << Utils::escapeJSON(timestamp_) << "\",\n"
        << "        \"counterparty\": \"" << Utils::escapeJSON(counterparty_) << "\",\n"
        << "        \"description\": \"" << Utils::escapeJSON(description_) << "\"\n"
        << "      }";
    return oss.str();
}

namespace {
    std::string extractStringField(const std::string& json, const std::string& key) {
        std::string pattern = "\"" + key + "\":";
        size_t pos = json.find(pattern);
        if (pos == std::string::npos) return "";
        pos = json.find('"', pos + pattern.length());
        if (pos == std::string::npos) return "";
        size_t end = pos + 1;
        while (end < json.length()) {
            if (json[end] == '"' && json[end - 1] != '\\') break;
            end++;
        }
        if (end > json.length()) return "";
        return Utils::unescapeJSON(json.substr(pos + 1, end - pos - 1));
    }

    double extractDoubleField(const std::string& json, const std::string& key) {
        std::string pattern = "\"" + key + "\":";
        size_t pos = json.find(pattern);
        if (pos == std::string::npos) return 0.0;
        pos += pattern.length();
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
        size_t end = pos;
        while (end < json.length() && (isdigit(json[end]) || json[end] == '.' || json[end] == '-')) end++;
        try {
            return std::stod(json.substr(pos, end - pos));
        } catch (...) {
            return 0.0;
        }
    }
}

Transaction Transaction::fromJSON(const std::string& jsonStr) {
    std::string id = extractStringField(jsonStr, "id");
    std::string typeStr = extractStringField(jsonStr, "type");
    double amount = extractDoubleField(jsonStr, "amount");
    double balanceAfter = extractDoubleField(jsonStr, "balanceAfter");
    std::string timestamp = extractStringField(jsonStr, "timestamp");
    std::string counterparty = extractStringField(jsonStr, "counterparty");
    std::string desc = extractStringField(jsonStr, "description");

    return Transaction(id, stringToTransactionType(typeStr), amount, balanceAfter, timestamp, counterparty, desc);
}

} // namespace Banking
