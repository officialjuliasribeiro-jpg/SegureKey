#ifndef DOCUMENT_STORE_H
#define DOCUMENT_STORE_H

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace DocumentStore {

    bool upload(
        const std::string& password,
        const std::string& filename,
        const std::vector<unsigned char>& content,
        std::string& id,
        std::string& error
    );

    bool list(
        const std::string& password,
        nlohmann::json& documents,
        std::string& error
    );

    bool download(
        const std::string& password,
        const std::string& id,
        std::vector<unsigned char>& content,
        std::string& filename,
        std::string& error
    );

    bool remove(
        const std::string& password,
        const std::string& id,
        std::string& error
    );
}

#endif