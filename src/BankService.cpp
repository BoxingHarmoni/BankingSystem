#include "BankService.hpp"
#include "Utils.hpp"
#include <algorithm>
#include <iostream>

namespace Banking {

std::string getResultMessage(OperationResult result) {
    switch (result) {
        case OperationResult::SUCCESS:
            return "Операція виконана успішно.";
        case OperationResult::ACCOUNT_NOT_FOUND:
            return "Рахунок/користувача не знайдено.";
        case OperationResult::INVALID_CREDENTIALS:
            return "Невірний логін або пароль.";
        case OperationResult::INSUFFICIENT_FUNDS:
            return "Недостатньо коштів на рахунку.";
        case OperationResult::ACCOUNT_FROZEN:
            return "Рахунок заблокований банком.";
        case OperationResult::RECIPIENT_FROZEN:
            return "Рахунок отримувача заблокований.";
        case OperationResult::SAME_ACCOUNT:
            return "Неможливо здійснити переказ на власний рахунок.";
        case OperationResult::INVALID_AMOUNT:
            return "Некоректна сума (повинна бути більше 0.00).";
        case OperationResult::USERNAME_TAKEN:
            return "Користувач із таким логіном вже існує.";
        case OperationResult::STORAGE_ERROR:
            return "Помилка запису даних у сховище.";
    }
    return "Невідомий статус операції.";
}

BankService::BankService(std::string storagePath)
    : storage_(std::move(storagePath)) {}

void BankService::updateNextSequence() {
    uint64_t maxSeq = 1000;
    for (const auto& acc : accounts_) {
        try {
            std::string iban = acc.getAccountNumber();
            if (iban.length() > 8) {
                uint64_t seq = std::stoull(iban.substr(iban.length() - 4));
                if (seq > maxSeq) maxSeq = seq;
            }
        } catch (...) {}
    }
    nextAccountSeq_ = maxSeq + 1;
}

bool BankService::initialize() {
    bool ok = storage_.loadAccounts(accounts_);
    if (ok) {
        updateNextSequence();
    }
    return ok;
}

bool BankService::save() {
    return storage_.saveAccounts(accounts_);
}

Account* BankService::login(const std::string& username, const std::string& password) {
    Account* acc = findAccountByUsername(username);
    if (!acc) return nullptr;
    if (acc->verifyPassword(password)) {
        return acc;
    }
    return nullptr;
}

OperationResult BankService::registerAccount(const std::string& username,
                                             const std::string& fullName,
                                             const std::string& password,
                                             const std::string& phone,
                                             const std::string& email,
                                             double initialDeposit,
                                             std::string& outIBAN,
                                             std::string& outCard) {
    if (findAccountByUsername(username) != nullptr) {
        return OperationResult::USERNAME_TAKEN;
    }

    if (initialDeposit < 0.0) {
        return OperationResult::INVALID_AMOUNT;
    }

    uint64_t seq = nextAccountSeq_++;
    outIBAN = Utils::generateIBAN(seq);
    outCard = Utils::generateCardNumber(seq);

    Account newAccount(
        outIBAN,
        outCard,
        username,
        fullName,
        Utils::sha256(password),
        phone,
        email,
        0.0,
        "UAH",
        false,
        false,
        Utils::getCurrentDateTime()
    );

    if (initialDeposit > 0.0) {
        newAccount.deposit(initialDeposit, "Каса банку", "Первинне зарахування коштів");
    }

    accounts_.push_back(newAccount);

    if (!save()) {
        return OperationResult::STORAGE_ERROR;
    }

    return OperationResult::SUCCESS;
}

Account* BankService::findAccountByUsername(const std::string& username) {
    for (auto& acc : accounts_) {
        if (acc.getUsername() == username) {
            return &acc;
        }
    }
    return nullptr;
}

Account* BankService::findAccountByNumber(const std::string& accNumber) {
    for (auto& acc : accounts_) {
        if (acc.getAccountNumber() == accNumber) {
            return &acc;
        }
    }
    return nullptr;
}

Account* BankService::findAccountByCard(const std::string& cardNumber) {
    for (auto& acc : accounts_) {
        if (acc.getCardNumber() == cardNumber) {
            return &acc;
        }
    }
    return nullptr;
}

Account* BankService::findAccountByIdentifier(const std::string& query) {
    std::string cleanQuery = Utils::trim(query);
    // Strip hyphens and spaces if searching for card number
    std::string noDash = cleanQuery;
    noDash.erase(std::remove(noDash.begin(), noDash.end(), '-'), noDash.end());
    noDash.erase(std::remove(noDash.begin(), noDash.end(), ' '), noDash.end());

    for (auto& acc : accounts_) {
        if (acc.getUsername() == cleanQuery ||
            acc.getAccountNumber() == cleanQuery ||
            acc.getCardNumber() == cleanQuery ||
            acc.getPhone() == cleanQuery) {
            return &acc;
        }

        std::string cardClean = acc.getCardNumber();
        cardClean.erase(std::remove(cardClean.begin(), cardClean.end(), '-'), cardClean.end());
        if (cardClean == noDash) {
            return &acc;
        }
    }
    return nullptr;
}

OperationResult BankService::deposit(Account* account, double amount, const std::string& description) {
    if (!account) return OperationResult::ACCOUNT_NOT_FOUND;
    if (account->isFrozen()) return OperationResult::ACCOUNT_FROZEN;
    if (amount <= 0.0) return OperationResult::INVALID_AMOUNT;

    if (!account->deposit(amount, "Каса / Термінал", description)) {
        return OperationResult::STORAGE_ERROR;
    }

    save();
    return OperationResult::SUCCESS;
}

OperationResult BankService::withdraw(Account* account, double amount, const std::string& description) {
    if (!account) return OperationResult::ACCOUNT_NOT_FOUND;
    if (account->isFrozen()) return OperationResult::ACCOUNT_FROZEN;
    if (amount <= 0.0) return OperationResult::INVALID_AMOUNT;
    if (account->getBalance() < amount) return OperationResult::INSUFFICIENT_FUNDS;

    if (!account->withdraw(amount, "Банкомат ATM Forma Tail", description)) {
        return OperationResult::STORAGE_ERROR;
    }

    save();
    return OperationResult::SUCCESS;
}

OperationResult BankService::transfer(Account* sender,
                                     const std::string& recipientIdentifier,
                                     double amount,
                                     const std::string& description) {
    if (!sender) return OperationResult::ACCOUNT_NOT_FOUND;
    if (sender->isFrozen()) return OperationResult::ACCOUNT_FROZEN;
    if (amount <= 0.0) return OperationResult::INVALID_AMOUNT;
    if (sender->getBalance() < amount) return OperationResult::INSUFFICIENT_FUNDS;

    Account* recipient = findAccountByIdentifier(recipientIdentifier);
    if (!recipient) return OperationResult::ACCOUNT_NOT_FOUND;
    if (recipient->getAccountNumber() == sender->getAccountNumber()) return OperationResult::SAME_ACCOUNT;
    if (recipient->isFrozen()) return OperationResult::RECIPIENT_FROZEN;

    // Withdraw from sender
    bool wOk = sender->withdraw(amount, recipient->getFullName() + " (" + recipient->getCardNumber() + ")", description);
    if (!wOk) return OperationResult::INSUFFICIENT_FUNDS;

    // Deposit to recipient
    recipient->deposit(amount, sender->getFullName() + " (" + sender->getCardNumber() + ")", description);

    save();
    return OperationResult::SUCCESS;
}

OperationResult BankService::changePassword(Account* account, const std::string& oldPassword, const std::string& newPassword) {
    if (!account) return OperationResult::ACCOUNT_NOT_FOUND;
    if (!account->verifyPassword(oldPassword)) {
        return OperationResult::INVALID_CREDENTIALS;
    }
    account->setPassword(newPassword);
    save();
    return OperationResult::SUCCESS;
}

OperationResult BankService::setAccountFrozen(const std::string& accNumber, bool frozen) {
    Account* acc = findAccountByNumber(accNumber);
    if (!acc) return OperationResult::ACCOUNT_NOT_FOUND;
    acc->setFrozen(frozen);
    save();
    return OperationResult::SUCCESS;
}

double BankService::getTotalReserves() const {
    double total = 0.0;
    for (const auto& acc : accounts_) {
        total += acc.getBalance();
    }
    return total;
}

std::vector<std::pair<std::string, Transaction>> BankService::getAllSystemTransactions() const {
    std::vector<std::pair<std::string, Transaction>> all;
    for (const auto& acc : accounts_) {
        for (const auto& tx : acc.getTransactions()) {
            all.emplace_back(acc.getAccountNumber() + " (" + acc.getFullName() + ")", tx);
        }
    }
    return all;
}

bool BankService::exportStatement(const Account& account, const std::string& outPath) {
    return storage_.exportStatement(account, outPath);
}

} // namespace Banking
