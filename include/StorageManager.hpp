#pragma once

#include <string>
#include <vector>
#include "Account.hpp"

namespace Banking {

class StorageManager {
private:
    std::string filePath_;

    void seedDefaultData(std::vector<Account>& accounts);

public:
    explicit StorageManager(std::string filePath = "data/bank_data.json");

    bool loadAccounts(std::vector<Account>& accounts);
    bool saveAccounts(const std::vector<Account>& accounts);
    bool exportStatement(const Account& account, const std::string& outPath);

    const std::string& getFilePath() const { return filePath_; }
};

} // namespace Banking
