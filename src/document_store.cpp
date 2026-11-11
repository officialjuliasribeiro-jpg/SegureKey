#include "document_store.h"
#include "document_crypto.h"

#include <openssl/rand.h>

#include <filesystem>
#include <fstream>
#include <algorithm>
#include <array>
#include <cctype>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace DocumentStore {

namespace {

    const fs::path DIRECTORY = "data/documents";
    const fs::path INDEX_FILE = DIRECTORY / "index.skd";

    bool validId(const std::string& id) {
        if (id.size() != 32) return false;

        return std::all_of(id.begin(), id.end(), [](unsigned char c) {
            return std::isxdigit(c) != 0;
        });
    }

    std::string generateId() {
        std::array<unsigned char, 16> bytes{};

        if (RAND_bytes(bytes.data(), bytes.size()) != 1) {
            return {};
        }

        static const char* hex = "0123456789abcdef";
        std::string id;

        for (unsigned char byte : bytes) {
            id += hex[byte >> 4];
            id += hex[byte & 0x0F];
        }

        return id;
    }

    std::string extensionOf(const std::string& filename) {
        fs::path path(filename);
        std::string ext = path.extension().string();

        std::transform(ext.begin(), ext.end(), ext.begin(),
            [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });

        return ext;
    }

    bool allowedFile(const std::string& filename) {
        const std::string ext = extensionOf(filename);
        return ext == ".pdf" || ext == ".jpg" || ext == ".jpeg";
    }

    bool readBinary(
        const fs::path& path,
        std::vector<unsigned char>& data
    ) {
        std::ifstream file(path, std::ios::binary);

        if (!file) return false;

        file.seekg(0, std::ios::end);
        const auto size = file.tellg();

        if (size <= 0 || size > 16 * 1024 * 1024) {
            return false;
        }

        file.seekg(0, std::ios::beg);
        data.resize(static_cast<size_t>(size));

        return static_cast<bool>(
            file.read(
                reinterpret_cast<char*>(data.data()),
                static_cast<std::streamsize>(data.size())
            )
        );
    }

    bool writeBinary(
        const fs::path& path,
        const std::vector<unsigned char>& data
    ) {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);

        if (!file) return false;

        file.write(
            reinterpret_cast<const char*>(data.data()),
            static_cast<std::streamsize>(data.size())
        );

        return file.good();
    }

    bool loadIndex(
        const std::string& password,
        json& documents,
        std::string& error
    ) {
        documents = json::array();

        if (!fs::exists(INDEX_FILE)) {
            return true;
        }

        std::vector<unsigned char> encrypted;
        std::vector<unsigned char> plain;

        if (!readBinary(INDEX_FILE, encrypted)) {
            error = "Não foi possível ler o índice dos documentos.";
            return false;
        }

        if (!DocumentCrypto::decryptFile(
                password, encrypted, plain, error)) {
            return false;
        }

        try {
            documents = json::parse(plain.begin(), plain.end());

            if (!documents.is_array()) {
                error = "Índice de documentos inválido.";
                return false;
            }
        } catch (...) {
            error = "Não foi possível interpretar o índice.";
            return false;
        }

        return true;
    }

    bool saveIndex(
        const std::string& password,
        const json& documents,
        std::string& error
    ) {
        const std::string serialized = documents.dump();

        std::vector<unsigned char> plain(
            serialized.begin(), serialized.end()
        );

        std::vector<unsigned char> encrypted;

        if (!DocumentCrypto::encryptFile(
                password, plain, encrypted, error)) {
            return false;
        }

        const fs::path temporary = INDEX_FILE.string() + ".tmp";

        if (!writeBinary(temporary, encrypted)) {
            error = "Não foi possível gravar o índice temporário.";
            return false;
        }

        std::error_code ec;
        fs::rename(temporary, INDEX_FILE, ec);

        if (ec) {
            fs::remove(temporary);
            error = "Não foi possível atualizar o índice.";
            return false;
        }

        return true;
    }

    fs::path documentPath(const std::string& id) {
        return DIRECTORY / (id + ".skd");
    }

}

bool upload(
    const std::string& password,
    const std::string& filename,
    const std::vector<unsigned char>& content,
    std::string& id,
    std::string& error
) {
    if (!allowedFile(filename)) {
        error = "Formato não permitido. Use PDF, JPG ou JPEG.";
        return false;
    }

    if (content.empty() || content.size() > 15 * 1024 * 1024) {
        error = "O arquivo deve ter até 15 MB.";
        return false;
    }

    std::error_code ec;
    fs::create_directories(DIRECTORY, ec);

    if (ec) {
        error = "Não foi possível criar a pasta de documentos.";
        return false;
    }

    json documents;

    if (!loadIndex(password, documents, error)) {
        return false;
    }

    id = generateId();

    if (id.empty()) {
        error = "Não foi possível gerar o identificador do documento.";
        return false;
    }

    std::vector<unsigned char> encrypted;

    if (!DocumentCrypto::encryptFile(
            password, content, encrypted, error)) {
        return false;
    }

    if (!writeBinary(documentPath(id), encrypted)) {
        error = "Não foi possível salvar o documento criptografado.";
        return false;
    }

    documents.push_back({
        {"id", id},
        {"nome", fs::path(filename).filename().string()},
        {"tamanho", content.size()},
        {"extensao", extensionOf(filename)}
    });

    if (!saveIndex(password, documents, error)) {
        fs::remove(documentPath(id));
        return false;
    }

    return true;
}

bool list(
    const std::string& password,
    json& documents,
    std::string& error
) {
    if (!fs::exists(INDEX_FILE)) {
        documents = json::array();
        return true;
    }

    return loadIndex(password, documents, error);
}

bool download(
    const std::string& password,
    const std::string& id,
    std::vector<unsigned char>& content,
    std::string& filename,
    std::string& error
) {
    if (!validId(id)) {
        error = "Identificador inválido.";
        return false;
    }

    json documents;

    if (!loadIndex(password, documents, error)) {
        return false;
    }

    auto it = std::find_if(
        documents.begin(), documents.end(),
        [&](const json& item) {
            return item.value("id", "") == id;
        }
    );

    if (it == documents.end()) {
        error = "Documento não encontrado.";
        return false;
    }

    filename = it->value("nome", "documento");

    std::vector<unsigned char> encrypted;

    if (!readBinary(documentPath(id), encrypted)) {
        error = "Não foi possível ler o documento.";
        return false;
    }

    return DocumentCrypto::decryptFile(
        password, encrypted, content, error
    );
}

bool remove(
    const std::string& password,
    const std::string& id,
    std::string& error
) {
    if (!validId(id)) {
        error = "Identificador inválido.";
        return false;
    }

    json documents;

    if (!loadIndex(password, documents, error)) {
        return false;
    }

    const auto oldSize = documents.size();

    documents.erase(
        std::remove_if(
            documents.begin(), documents.end(),
            [&](const json& item) {
                return item.value("id", "") == id;
            }
        ),
        documents.end()
    );

    if (documents.size() == oldSize) {
        error = "Documento não encontrado.";
        return false;
    }

    if (!saveIndex(password, documents, error)) {
        return false;
    }

    std::error_code ec;
    fs::remove(documentPath(id), ec);

    if (ec) {
        error = "O registro foi removido, mas o arquivo não pôde ser excluído.";
        return false;
    }

    return true;
}

}