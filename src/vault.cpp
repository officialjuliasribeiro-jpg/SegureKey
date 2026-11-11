#include "vault.h"

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>

#include <filesystem>
#include <fstream>
#include <vector>
#include <array>
#include <cstdint>
#include <algorithm>
#include <iterator>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace {

const std::string FILE_PATH = "data/vault.enc";

constexpr size_t SALT_SIZE = 16;
constexpr size_t NONCE_SIZE = 12;
constexpr size_t TAG_SIZE = 16;
constexpr size_t KEY_SIZE = 32;

using Salt = std::array<unsigned char, SALT_SIZE>;
using Nonce = std::array<unsigned char, NONCE_SIZE>;
using Key = std::array<unsigned char, KEY_SIZE>;
using Tag = std::array<unsigned char, TAG_SIZE>;

bool deriveKey(
    const std::string& password,
    const Salt& salt,
    Key& key
) {
    return EVP_PBE_scrypt(
        password.data(),
        password.size(),
        salt.data(),
        salt.size(),
        32768,
        8,
        1,
        64 * 1024 * 1024,
        key.data(),
        key.size()
    ) == 1;
}

bool encryptData(
    const std::string& plaintext,
    const Key& key,
    const Nonce& nonce,
    std::vector<unsigned char>& ciphertext,
    Tag& tag
) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();

    if (!ctx) return false;

    ciphertext.resize(plaintext.size() + 16);

    int length = 0;
    int total = 0;

    bool success =
        EVP_EncryptInit_ex(
            ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr
        ) == 1 &&
        EVP_CIPHER_CTX_ctrl(
            ctx, EVP_CTRL_GCM_SET_IVLEN, NONCE_SIZE, nullptr
        ) == 1 &&
        EVP_EncryptInit_ex(
            ctx, nullptr, nullptr, key.data(), nonce.data()
        ) == 1 &&
        EVP_EncryptUpdate(
            ctx,
            ciphertext.data(),
            &length,
            reinterpret_cast<const unsigned char*>(plaintext.data()),
            static_cast<int>(plaintext.size())
        ) == 1;

    if (success) {
        total = length;

        success = EVP_EncryptFinal_ex(
            ctx, ciphertext.data() + total, &length
        ) == 1;

        total += length;
    }

    if (success) {
        success = EVP_CIPHER_CTX_ctrl(
            ctx, EVP_CTRL_GCM_GET_TAG, TAG_SIZE, tag.data()
        ) == 1;
    }

    ciphertext.resize(total);
    EVP_CIPHER_CTX_free(ctx);

    return success;
}

bool decryptData(
    const std::vector<unsigned char>& ciphertext,
    const Key& key,
    const Nonce& nonce,
    const Tag& tag,
    std::string& plaintext
) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();

    if (!ctx) return false;

    std::vector<unsigned char> output(ciphertext.size() + 16);

    int length = 0;
    int total = 0;

    bool success =
        EVP_DecryptInit_ex(
            ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr
        ) == 1 &&
        EVP_CIPHER_CTX_ctrl(
            ctx, EVP_CTRL_GCM_SET_IVLEN, NONCE_SIZE, nullptr
        ) == 1 &&
        EVP_DecryptInit_ex(
            ctx, nullptr, nullptr, key.data(), nonce.data()
        ) == 1;

    if (success && !ciphertext.empty()) {
        success = EVP_DecryptUpdate(
            ctx,
            output.data(),
            &length,
            ciphertext.data(),
            static_cast<int>(ciphertext.size())
        ) == 1;

        total = length;
    }

    if (success) {
        success = EVP_CIPHER_CTX_ctrl(
            ctx, EVP_CTRL_GCM_SET_TAG, TAG_SIZE,
            const_cast<unsigned char*>(tag.data())
        ) == 1;
    }

    if (success) {
        success = EVP_DecryptFinal_ex(
            ctx, output.data() + total, &length
        ) == 1;

        total += length;
    }

    if (success) {
        plaintext.assign(
            reinterpret_cast<const char*>(output.data()),
            total
        );
    }

    EVP_CIPHER_CTX_free(ctx);

    return success;
}

bool saveVault(
    const json& records,
    const std::string& password,
    const Salt* existingSalt,
    std::string& error
) {
    Salt salt;

    if (existingSalt) {
        salt = *existingSalt;
    } else if (RAND_bytes(salt.data(), SALT_SIZE) != 1) {
        error = "Falha ao gerar salt.";
        return false;
    }

    Nonce nonce;

    if (RAND_bytes(nonce.data(), NONCE_SIZE) != 1) {
        error = "Falha ao gerar nonce.";
        return false;
    }

    Key key{};

    if (!deriveKey(password, salt, key)) {
        error = "Falha na derivacao da chave.";
        return false;
    }

    std::string plaintext = records.dump();

    std::vector<unsigned char> ciphertext;
    Tag tag{};

    bool encrypted = encryptData(
        plaintext, key, nonce, ciphertext, tag
    );

    OPENSSL_cleanse(key.data(), key.size());

    if (!encrypted) {
        error = "Falha na criptografia.";
        return false;
    }

    std::vector<unsigned char> fileData;

    const char magic[] = {'S', 'K', 'V', '1'};

    fileData.insert(fileData.end(), magic, magic + 4);
    fileData.insert(fileData.end(), salt.begin(), salt.end());
    fileData.insert(fileData.end(), nonce.begin(), nonce.end());
    fileData.insert(fileData.end(), tag.begin(), tag.end());
    fileData.insert(
        fileData.end(), ciphertext.begin(), ciphertext.end()
    );

    fs::create_directories("data");

    const std::string temporaryPath = FILE_PATH + ".tmp";

    {
        std::ofstream file(
            temporaryPath,
            std::ios::binary | std::ios::trunc
        );

        if (!file) {
            error = "Nao foi possivel criar o arquivo.";
            return false;
        }

        file.write(
            reinterpret_cast<const char*>(fileData.data()),
            static_cast<std::streamsize>(fileData.size())
        );

        if (!file) {
            error = "Erro ao gravar os dados.";
            file.close();
            fs::remove(temporaryPath);
            return false;
        }
    }

    std::error_code ec;
    fs::rename(temporaryPath, FILE_PATH, ec);

    if (ec) {
        fs::remove(temporaryPath);
        error = "Erro ao finalizar o armazenamento.";
        return false;
    }

    return true;
}

bool loadVault(
    const std::string& password,
    json& records,
    Salt& salt,
    std::string& error
) {
    std::ifstream file(FILE_PATH, std::ios::binary);

    if (!file) {
        error = "Cofre ainda nao criado. Cadastre o primeiro registro.";
        return false;
    }

    std::vector<unsigned char> data(
        std::istreambuf_iterator<char>(file),
        {}
    );

    constexpr size_t HEADER_SIZE =
        4 + SALT_SIZE + NONCE_SIZE + TAG_SIZE;

    if (data.size() < HEADER_SIZE ||
        data.size() > 10 * 1024 * 1024) {
        error = "Arquivo do cofre invalido.";
        return false;
    }

    if (data[0] != 'S' || data[1] != 'K' ||
        data[2] != 'V' || data[3] != '1') {
        error = "Formato do cofre desconhecido.";
        return false;
    }

    size_t pos = 4;

    std::copy_n(data.begin() + pos, SALT_SIZE, salt.begin());
    pos += SALT_SIZE;

    Nonce nonce;
    std::copy_n(data.begin() + pos, NONCE_SIZE, nonce.begin());
    pos += NONCE_SIZE;

    Tag tag;
    std::copy_n(data.begin() + pos, TAG_SIZE, tag.begin());
    pos += TAG_SIZE;

    std::vector<unsigned char> ciphertext(
        data.begin() + pos, data.end()
    );

    Key key{};

    if (!deriveKey(password, salt, key)) {
        error = "Falha na derivacao da chave.";
        return false;
    }

    std::string plaintext;

    bool decrypted = decryptData(
        ciphertext, key, nonce, tag, plaintext
    );

    OPENSSL_cleanse(key.data(), key.size());

    if (!decrypted) {
        error = "Senha incorreta ou arquivo adulterado.";
        return false;
    }

    try {
        records = json::parse(plaintext);

        if (!records.is_array()) {
            error = "Conteudo do cofre invalido.";
            return false;
        }
    } catch (...) {
        error = "Nao foi possivel interpretar os registros.";
        return false;
    }

    return true;
}

}

namespace Vault {

bool addRecord(
    const std::string& password,
    const std::string& title,
    const std::string& information,
    std::string& error
) {
    if (password.size() < 12) {
        error = "A senha mestra deve ter pelo menos 12 caracteres.";
        return false;
    }

    if (title.empty() || information.empty() ||
        title.size() > 200 || information.size() > 10000) {
        error = "Titulo ou informacao invalida.";
        return false;
    }

    json records = json::array();
    Salt salt{};

    if (fs::exists(FILE_PATH)) {
        if (!loadVault(password, records, salt, error)) {
            return false;
        }

        if (records.size() >= 1000) {
            error = "Limite de registros atingido.";
            return false;
        }

        records.push_back({
            {"titulo", title},
            {"informacao", information}
        });

        return saveVault(records, password, &salt, error);
    }

    records.push_back({
        {"titulo", title},
        {"informacao", information}
    });

    return saveVault(records, password, nullptr, error);
}
// EDITAR REGISTRO
bool editRecord(
    const std::string& password,
    size_t index,
    const std::string& title,
    const std::string& information,
    std::string& error
) {
    if (password.size() < 12) {
        error = "Senha mestra invalida.";
        return false;
    }

    if (title.empty() || information.empty() ||
        title.size() > 200 || information.size() > 10000) {
        error = "Titulo ou informacao invalida.";
        return false;
    }

    json records = json::array();
    Salt salt{};

    if (!loadVault(password, records, salt, error)) {
        return false;
    }

    if (index >= records.size()) {
        error = "Registro nao encontrado.";
        return false;
    }

    records[index]["titulo"] = title;
    records[index]["informacao"] = information;

    return saveVault(records, password, &salt, error);
}


// EXCLUIR REGISTRO
bool deleteRecord(
    const std::string& password,
    size_t index,
    std::string& error
) {
    if (password.size() < 12) {
        error = "Senha mestra invalida.";
        return false;
    }

    json records = json::array();
    Salt salt{};

    if (!loadVault(password, records, salt, error)) {
        return false;
    }

    if (index >= records.size()) {
        error = "Registro nao encontrado.";
        return false;
    }

    records.erase(records.begin() + index);

    return saveVault(records, password, &salt, error);
}
bool addCategorizedRecord(
    const std::string& password,
    const std::string& category,
    const std::string& title,
    const std::string& information,
    std::string& error
) {
    const std::vector<std::string> categories = {
        "pessoal",
        "senhas",
        "financeiro",
        "documentos",
        "mensagens"
    };

    if (std::find(
        categories.begin(),
        categories.end(),
        category
    ) == categories.end()) {
        error = "Categoria invalida.";
        return false;
    }

    if (password.size() < 12) {
        error = "Senha mestra invalida.";
        return false;
    }

    if (title.empty() || information.empty() ||
        title.size() > 200 || information.size() > 10000) {
        error = "Dados invalidos.";
        return false;
    }

    json records = json::array();
    Salt salt{};

    if (fs::exists(FILE_PATH)) {
        if (!loadVault(password, records, salt, error)) {
            return false;
        }

        if (records.size() >= 1000) {
            error = "Limite de registros atingido.";
            return false;
        }
    }

    records.push_back({
        {"categoria", category},
        {"titulo", title},
        {"informacao", information}
    });

    if (fs::exists(FILE_PATH)) {
        return saveVault(records, password, &salt, error);
    }

    return saveVault(records, password, nullptr, error);
}
bool listRecords(
    const std::string& password,
    json& records,
    std::string& error
) {
    if (password.size() < 12) {
        error = "Senha mestra invalida.";
        return false;
    }

    Salt salt{};

    return loadVault(password, records, salt, error);
}

}