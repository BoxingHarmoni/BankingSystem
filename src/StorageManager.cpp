#include "StorageManager.hpp"
#include "Utils.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>

namespace Banking {

StorageManager::StorageManager(std::string filePath)
    : filePath_(std::move(filePath)) {}

void StorageManager::seedDefaultData(std::vector<Account>& accounts) {
    accounts.clear();

    // 1. Admin account
    Account admin(
        "UA000000000000000000000000000",
        "0000-0000-0000-0000",
        "admin",
        "Адміністратор Банку",
        Utils::sha256("admin123"),
        "+380800500000",
        "admin@formatail.bank",
        0.0,
        "UAH",
        false,
        true,
        Utils::getCurrentDateTime()
    );
    accounts.push_back(admin);

    // 2. Client 1: Олександр Коваленко
    Account client1(
        Utils::generateIBAN(1001),
        Utils::generateCardNumber(1001),
        "user1",
        "Коваленко Олександр Сергійович",
        Utils::sha256("1234"),
        "+380671112233",
        "kovalenko@example.com",
        25400.00,
        "UAH",
        false,
        false,
        Utils::getCurrentDateTime()
    );
    client1.addTransaction(Transaction(
        Utils::generateTransactionId(),
        TransactionType::DEPOSIT,
        25400.00,
        25400.00,
        Utils::getCurrentDateTime(),
        "Термінал самообслуговування №12",
        "Початкове поповнення рахунку"
    ));
    accounts.push_back(client1);

    // 3. Client 2: Марія Мельник
    Account client2(
        Utils::generateIBAN(1002),
        Utils::generateCardNumber(1002),
        "user2",
        "Мельник Марія Іванівна",
        Utils::sha256("1234"),
        "+380509998877",
        "melnyk@example.com",
        14850.50,
        "UAH",
        false,
        false,
        Utils::getCurrentDateTime()
    );
    client2.addTransaction(Transaction(
        Utils::generateTransactionId(),
        TransactionType::DEPOSIT,
        15000.00,
        15000.00,
        Utils::getCurrentDateTime(),
        "Каса відділення банку",
        "Внесення готівки на депозит"
    ));
    client2.addTransaction(Transaction(
        Utils::generateTransactionId(),
        TransactionType::WITHDRAW,
        149.50,
        14850.50,
        Utils::getCurrentDateTime(),
        "Супермаркет 'Сільпо'",
        "Оплата покупок"
    ));
    accounts.push_back(client2);

    saveAccounts(accounts);
}

bool StorageManager::loadAccounts(std::vector<Account>& accounts) {
    accounts.clear();
    std::ifstream inFile(filePath_);
    if (!inFile.is_open()) {
        // If file doesn't exist, create it with seed data
        seedDefaultData(accounts);
        return true;
    }

    std::stringstream buffer;
    buffer << inFile.rdbuf();
    inFile.close();

    std::string json = buffer.str();
    if (json.empty()) {
        seedDefaultData(accounts);
        return true;
    }

    size_t accountsKey = json.find("\"accounts\":");
    if (accountsKey == std::string::npos) {
        seedDefaultData(accounts);
        return true;
    }

    size_t arrayStart = json.find('[', accountsKey);
    if (arrayStart == std::string::npos) return false;

    int depth = 0;
    size_t objStart = std::string::npos;
    bool inQuotes = false;
    bool escape = false;

    for (size_t i = arrayStart; i < json.length(); ++i) {
        char c = json[i];
        if (escape) {
            escape = false;
            continue;
        }
        if (c == '\\') {
            escape = true;
            continue;
        }
        if (c == '"') {
            inQuotes = !inQuotes;
            continue;
        }
        if (!inQuotes) {
            if (c == '{') {
                if (depth == 0) {
                    objStart = i;
                }
                depth++;
            } else if (c == '}') {
                depth--;
                if (depth == 0 && objStart != std::string::npos) {
                    std::string accJson = json.substr(objStart, i - objStart + 1);
                    accounts.push_back(Account::fromJSON(accJson));
                    objStart = std::string::npos;
                }
            } else if (c == ']' && depth == 0) {
                break;
            }
        }
    }

    if (accounts.empty()) {
        seedDefaultData(accounts);
    }
    return true;
}

bool StorageManager::saveAccounts(const std::vector<Account>& accounts) {
    std::ofstream outFile(filePath_);
    if (!outFile.is_open()) {
        return false;
    }

    outFile << "{\n"
            << "  \"bank\": \"Bank Forma Tail\",\n"
            << "  \"updatedAt\": \"" << Utils::getCurrentDateTime() << "\",\n"
            << "  \"accounts\": [\n";

    for (size_t i = 0; i < accounts.size(); ++i) {
        outFile << accounts[i].toJSON();
        if (i + 1 < accounts.size()) {
            outFile << ",";
        }
        outFile << "\n";
    }

    outFile << "  ]\n}\n";
    outFile.close();
    return true;
}

bool StorageManager::exportStatement(const Account& account, const std::string& outPath) {
    std::ofstream file(outPath);
    if (!file.is_open()) return false;

    file << "========================================================================================\n";
    file << "                                ОФІЦІЙНА ВИПИСКА ПО РАХУНКУ                              \n";
    file << "                                     \"BANK FORMA TAIL\"                                  \n";
    file << "========================================================================================\n\n";

    file << "Власник рахунку:    " << account.getFullName() << "\n";
    file << "IBAN рахунку:       " << account.getAccountNumber() << "\n";
    file << "Номер картки:       " << account.getCardNumber() << "\n";
    file << "Поточний баланс:    " << Utils::formatMoney(account.getBalance(), account.getCurrency()) << "\n";
    file << "Статус рахунку:     " << (account.isFrozen() ? "ЗАБЛОКОВАНО" : "АКТИВНИЙ") << "\n";
    file << "Дата формування:    " << Utils::getCurrentDateTime() << "\n\n";

    file << "----------------------------------------------------------------------------------------\n";
    file << std::left 
         << std::setw(20) << "Дата і час"
         << std::setw(15) << "Тип операції"
         << std::setw(14) << "Сума"
         << std::setw(14) << "Залишок"
         << std::setw(25) << "Контрагент / Опис" << "\n";
    file << "----------------------------------------------------------------------------------------\n";

    const auto& txs = account.getTransactions();
    if (txs.empty()) {
        file << "                                 Транзакцій не знайдено                                 \n";
    } else {
        for (const auto& tx : txs) {
            std::string typeLabel;
            std::string sign = "";
            switch (tx.getType()) {
                case TransactionType::DEPOSIT:
                    typeLabel = "Поповнення";
                    sign = "+";
                    break;
                case TransactionType::WITHDRAW:
                    typeLabel = "Зняття";
                    sign = "-";
                    break;
                case TransactionType::TRANSFER_OUT:
                    typeLabel = "Переказ (вих.)";
                    sign = "-";
                    break;
                case TransactionType::TRANSFER_IN:
                    typeLabel = "Переказ (вхід.)";
                    sign = "+";
                    break;
                default:
                    typeLabel = "Інше";
                    break;
            }

            std::string amountStr = sign + Utils::formatMoney(tx.getAmount(), "");
            std::string desc = tx.getCounterparty() + " (" + tx.getDescription() + ")";
            if (desc.length() > 24) desc = desc.substr(0, 21) + "...";

            file << std::left 
                 << std::setw(20) << tx.getTimestamp()
                 << std::setw(15) << typeLabel
                 << std::setw(14) << amountStr
                 << std::setw(14) << Utils::formatMoney(tx.getBalanceAfter(), "")
                 << std::setw(25) << desc << "\n";
        }
    }

    file << "========================================================================================\n";
    file << "Дякуємо, що користуєтесь Bank Forma Tail! Бажаємо фінансового процвітання.\n";
    file.close();
    return true;
}

} // namespace Banking
