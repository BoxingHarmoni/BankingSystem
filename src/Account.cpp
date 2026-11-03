#include "Account.hpp"
#include "Utils.hpp"
#include <sstream>
#include <iomanip>

namespace Banking {

Account::Account(std::string accountNumber,
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
                 std::string createdAt)
    : accountNumber_(std::move(accountNumber)),
      cardNumber_(std::move(cardNumber)),
      username_(std::move(username)),
      fullName_(std::move(fullName)),
      passwordHash_(std::move(passwordHash)),
      phone_(std::move(phone)),
      email_(std::move(email)),
      balance_(initialBalance),
      currency_(std::move(currency)),
      isFrozen_(isFrozen),
      isAdmin_(isAdmin),
      createdAt_(std::move(createdAt)) {}

void Account::setPassword(const std::string& plainPassword) {
    passwordHash_ = Utils::sha256(plainPassword);
}

bool Account::verifyPassword(const std::string& plainPassword) const {
    return passwordHash_ == Utils::sha256(plainPassword);
}

bool Account::deposit(double amount, const std::string& counterparty, const std::string& description) {
    if (amount <= 0.0) return false;
    balance_ += amount;
    Transaction tx(
        Utils::generateTransactionId(),
        TransactionType::DEPOSIT,
        amount,
        balance_,
        Utils::getCurrentDateTime(),
        counterparty,
        description
    );
    transactions_.push_back(tx);
    return true;
}

bool Account::withdraw(double amount, const std::string& counterparty, const std::string& description) {
    if (amount <= 0.0 || balance_ < amount || isFrozen_) return false;
    balance_ -= amount;
    Transaction tx(
        Utils::generateTransactionId(),
        TransactionType::WITHDRAW,
        amount,
        balance_,
        Utils::getCurrentDateTime(),
        counterparty,
        description
    );
    transactions_.push_back(tx);
    return true;
}

void Account::addTransaction(const Transaction& tx) {
    transactions_.push_back(tx);
}

std::string Account::toJSON() const {
    std::ostringstream oss;
    oss << "  {\n"
        << "    \"accountNumber\": \"" << Utils::escapeJSON(accountNumber_) << "\",\n"
        << "    \"cardNumber\": \"" << Utils::escapeJSON(cardNumber_) << "\",\n"
        << "    \"username\": \"" << Utils::escapeJSON(username_) << "\",\n"
        << "    \"fullName\": \"" << Utils::escapeJSON(fullName_) << "\",\n"
        << "    \"passwordHash\": \"" << Utils::escapeJSON(passwordHash_) << "\",\n"
        << "    \"phone\": \"" << Utils::escapeJSON(phone_) << "\",\n"
        << "    \"email\": \"" << Utils::escapeJSON(email_) << "\",\n"
        << "    \"balance\": " << std::fixed << std::setprecision(2) << balance_ << ",\n"
        << "    \"currency\": \"" << Utils::escapeJSON(currency_) << "\",\n"
        << "    \"isFrozen\": " << (isFrozen_ ? "true" : "false") << ",\n"
        << "    \"isAdmin\": " << (isAdmin_ ? "true" : "false") << ",\n"
        << "    \"createdAt\": \"" << Utils::escapeJSON(createdAt_) << "\",\n"
        << "    \"transactions\": [\n";
    for (size_t i = 0; i < transactions_.size(); ++i) {
        oss << "      " << transactions_[i].toJSON();
        if (i + 1 < transactions_.size()) oss << ",";
        oss << "\n";
    }
    oss << "    ]\n"
        << "  }";
    return oss.str();
}

namespace {
    std::string extractString(const std::string& json, const std::string& key) {
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

    double extractDouble(const std::string& json, const std::string& key) {
        std::string pattern = "\"" + key + "\":";
        size_t pos = json.find(pattern);
        if (pos == std::string::npos) return 0.0;
        pos += pattern.length();
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\r' || json[pos] == '\n')) pos++;
        size_t end = pos;
        while (end < json.length() && (isdigit(json[end]) || json[end] == '.' || json[end] == '-')) end++;
        try {
            return std::stod(json.substr(pos, end - pos));
        } catch (...) {
            return 0.0;
        }
    }

    bool extractBool(const std::string& json, const std::string& key) {
        std::string pattern = "\"" + key + "\":";
        size_t pos = json.find(pattern);
        if (pos == std::string::npos) return false;
        pos += pattern.length();
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\r' || json[pos] == '\n')) pos++;
        if (json.substr(pos, 4) == "true") return true;
        return false;
    }
}

Account Account::fromJSON(const std::string& jsonStr) {
    std::string accNum = extractString(jsonStr, "accountNumber");
    std::string cardNum = extractString(jsonStr, "cardNumber");
    std::string user = extractString(jsonStr, "username");
    std::string name = extractString(jsonStr, "fullName");
    std::string passHash = extractString(jsonStr, "passwordHash");
    std::string phone = extractString(jsonStr, "phone");
    std::string email = extractString(jsonStr, "email");
    double balance = extractDouble(jsonStr, "balance");
    std::string currency = extractString(jsonStr, "currency");
    bool isFrozen = extractBool(jsonStr, "isFrozen");
    bool isAdmin = extractBool(jsonStr, "isAdmin");
    std::string createdAt = extractString(jsonStr, "createdAt");

    Account acc(accNum, cardNum, user, name, passHash, phone, email, balance, currency, isFrozen, isAdmin, createdAt);

    // Parse transactions
    size_t txStart = jsonStr.find("\"transactions\":");
    if (txStart != std::string::npos) {
        size_t openBracket = jsonStr.find('[', txStart);
        size_t closeBracket = jsonStr.find(']', openBracket);
        if (openBracket != std::string::npos && closeBracket != std::string::npos) {
            size_t curr = openBracket;
            while (curr < closeBracket) {
                size_t objStart = jsonStr.find('{', curr);
                if (objStart == std::string::npos || objStart >= closeBracket) break;
                size_t objEnd = jsonStr.find('}', objStart);
                if (objEnd == std::string::npos || objEnd > closeBracket) break;

                std::string txJson = jsonStr.substr(objStart, objEnd - objStart + 1);
                acc.addTransaction(Transaction::fromJSON(txJson));
                curr = objEnd + 1;
            }
        }
    }

    return acc;
}

} // namespace Banking
