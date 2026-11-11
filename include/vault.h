#ifndef VAULT_H
#define VAULT_H

#include <string>
#include <nlohmann/json.hpp>
#include <cstddef>

namespace Vault {

    bool addRecord(
        const std::string& masterPassword,
        const std::string& title,
        const std::string& information,
        std::string& error
    );

    bool listRecords(
        const std::string& masterPassword,
        nlohmann::json& records,
        std::string& error
    );
bool editRecord(
    const std::string& masterPassword,
    size_t index,
    const std::string& title,
    const std::string& information,
    std::string& error
);

bool deleteRecord(
    const std::string& masterPassword,
    size_t index,
    std::string& error
);
bool addCategorizedRecord(
    const std::string& masterPassword,
    const std::string& category,
    const std::string& title,
    const std::string& information,
    std::string& error
);
}

#endif