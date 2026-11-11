#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <exception>

#include <httplib.h>
#include <nlohmann/json.hpp>

#include "vault.h"
#include "hill.h"
#include "document_store.h"

using json = nlohmann::json;

int main() {

    httplib::Server servidor;

    servidor.set_payload_max_length(16 * 1024 * 1024);
    servidor.set_mount_point("/", "./resources");

    // ==========================================
    // STATUS DO SERVIDOR
    // ==========================================

    servidor.Get("/api/status",
        [](const httplib::Request&, httplib::Response& res) {

            json resposta = {
                {"sistema", "SecureKey"},
                {"status", "online"},
                {"backend", "C++"}
            };

            res.set_content(resposta.dump(4), "application/json");
        }
    );

    // ==========================================
    // ADICIONAR REGISTRO AO COFRE
    // ==========================================

    servidor.Post("/api/vault/add",
        [](const httplib::Request& req, httplib::Response& res) {

            try {
                json dados = json::parse(req.body);

                std::string senha = dados.value("senha", "");
                std::string titulo = dados.value("titulo", "");
                std::string informacao = dados.value("informacao", "");
                std::string categoria = dados.value("categoria", "pessoal");

                std::string erro;

                bool sucesso = Vault::addCategorizedRecord(
                    senha,
                    categoria,
                    titulo,
                    informacao,
                    erro
                );

                json resposta;
                resposta["success"] = sucesso;

                if (sucesso) {
                    resposta["message"] = "Registro criptografado e salvo!";
                } else {
                    resposta["error"] = erro;
                }

                res.set_content(resposta.dump(), "application/json");

            } catch (...) {
                res.status = 400;
                res.set_content(
                    R"({"success":false,"error":"Dados invalidos."})",
                    "application/json"
                );
            }
        }
    );

    // ==========================================
    // LISTAR REGISTROS DO COFRE
    // ==========================================

    servidor.Post("/api/vault/list",
        [](const httplib::Request& req, httplib::Response& res) {

            try {
                json dados = json::parse(req.body);

                std::string senha = dados.value("senha", "");
                std::string erro;

                json registros;

                bool sucesso = Vault::listRecords(
                    senha,
                    registros,
                    erro
                );

                json resposta;
                resposta["success"] = sucesso;

                if (sucesso) {
                    resposta["registros"] = registros;
                } else {
                    resposta["error"] = erro;
                }

                res.set_content(resposta.dump(), "application/json");

            } catch (...) {
                res.status = 400;
                res.set_content(
                    R"({"success":false,"error":"Dados invalidos."})",
                    "application/json"
                );
            }
        }
    );

    // ==========================================
    // EDITAR REGISTRO
    // ==========================================

    servidor.Post("/api/vault/edit",
        [](const httplib::Request& req, httplib::Response& res) {

            try {
                json dados = json::parse(req.body);

                if (!dados.contains("indice") ||
                    !dados["indice"].is_number_integer() ||
                    dados["indice"].get<long long>() < 0) {

                    res.status = 400;
                    res.set_content(
                        R"({"success":false,"error":"Indice invalido."})",
                        "application/json"
                    );
                    return;
                }

                std::string senha = dados.value("senha", "");
                std::string titulo = dados.value("titulo", "");
                std::string informacao = dados.value("informacao", "");

                size_t indice = static_cast<size_t>(
                    dados["indice"].get<long long>()
                );

                std::string erro;

                bool sucesso = Vault::editRecord(
                    senha,
                    indice,
                    titulo,
                    informacao,
                    erro
                );

                json resposta = {
                    {"success", sucesso}
                };

                if (sucesso) {
                    resposta["message"] = "Registro atualizado!";
                } else {
                    resposta["error"] = erro;
                }

                res.set_content(resposta.dump(), "application/json");

            } catch (...) {
                res.status = 400;
                res.set_content(
                    R"({"success":false,"error":"Dados invalidos."})",
                    "application/json"
                );
            }
        }
    );

    // ==========================================
    // EXCLUIR REGISTRO
    // ==========================================

    servidor.Post("/api/vault/delete",
        [](const httplib::Request& req, httplib::Response& res) {

            try {
                json dados = json::parse(req.body);

                if (!dados.contains("indice") ||
                    !dados["indice"].is_number_integer() ||
                    dados["indice"].get<long long>() < 0) {

                    res.status = 400;
                    res.set_content(
                        R"({"success":false,"error":"Indice invalido."})",
                        "application/json"
                    );
                    return;
                }

                std::string senha = dados.value("senha", "");

                size_t indice = static_cast<size_t>(
                    dados["indice"].get<long long>()
                );

                std::string erro;

                bool sucesso = Vault::deleteRecord(
                    senha,
                    indice,
                    erro
                );

                json resposta = {
                    {"success", sucesso}
                };

                if (sucesso) {
                    resposta["message"] = "Registro excluido!";
                } else {
                    resposta["error"] = erro;
                }

                res.set_content(resposta.dump(), "application/json");

            } catch (...) {
                res.status = 400;
                res.set_content(
                    R"({"success":false,"error":"Dados invalidos."})",
                    "application/json"
                );
            }
        }
    );

    // ==========================================
    // CIFRA DE HILL - CRIPTOGRAFAR
    // ==========================================

    servidor.Post("/api/hill/encrypt",
        [](const httplib::Request& req, httplib::Response& res) {

            try {
                json dados = json::parse(req.body);

                std::string mensagem = dados.value("mensagem", "");

                std::string criptografada;
                std::string erro;

                bool sucesso = Hill::encrypt(
                    mensagem,
                    criptografada,
                    erro
                );

                json resposta = {
                    {"success", sucesso}
                };

                if (sucesso) {
                    resposta["criptografada"] = criptografada;
                    resposta["matriz"] = {
                        {3, 3},
                        {2, 5}
                    };
                    resposta["determinante"] = 9;
                    resposta["modulo"] = 26;
                } else {
                    resposta["error"] = erro;
                }

                res.set_content(resposta.dump(), "application/json");

            } catch (...) {
                res.status = 400;
                res.set_content(
                    R"({"success":false,"error":"Dados invalidos."})",
                    "application/json"
                );
            }
        }
    );

    // ==========================================
    // CIFRA DE HILL - DESCRIPTOGRAFAR
    // ==========================================

    servidor.Post("/api/hill/decrypt",
        [](const httplib::Request& req, httplib::Response& res) {

            try {
                json dados = json::parse(req.body);

                std::string mensagem = dados.value("mensagem", "");

                std::string descriptografada;
                std::string erro;

                bool sucesso = Hill::decrypt(
                    mensagem,
                    descriptografada,
                    erro
                );

                json resposta = {
                    {"success", sucesso}
                };

                if (sucesso) {
                    resposta["descriptografada"] = descriptografada;

                    resposta["matriz_inversa"] = {
                        {15, 17},
                        {20, 9}
                    };

                    resposta["modulo"] = 26;
                } else {
                    resposta["error"] = erro;
                }

                res.set_content(resposta.dump(), "application/json");

            } catch (...) {
                res.status = 400;
                res.set_content(
                    R"({"success":false,"error":"Dados invalidos."})",
                    "application/json"
                );
            }
        }
    );

    // ==========================================
    // ENVIAR DOCUMENTO CRIPTOGRAFADO
    // ==========================================

    servidor.Post("/api/documents/upload",
        [](const httplib::Request& req, httplib::Response& res) {

            try {
                if (!req.has_file("arquivo")) {
                    res.status = 400;
                    res.set_content(
                        R"({"erro":"Nenhum arquivo foi enviado."})",
                        "application/json"
                    );
                    return;
                }

                // Senha mestra recebida pelo cabeçalho HTTP
                const std::string senha =
                    req.get_header_value("X-Master-Password");

                if (senha.empty()) {
                    res.status = 400;
                    res.set_content(
                        R"({"erro":"Senha mestra nao recebida pelo servidor."})",
                        "application/json"
                    );
                    return;
                }

                const auto& arquivo = req.get_file_value("arquivo");

                std::vector<unsigned char> conteudo(
                    arquivo.content.begin(),
                    arquivo.content.end()
                );

                std::string id;
                std::string erro;

                bool sucesso = DocumentStore::upload(
                    senha,
                    arquivo.filename,
                    conteudo,
                    id,
                    erro
                );

                if (!sucesso) {
                    res.status = 400;
                    res.set_content(
                        json{{"sucesso", false}, {"erro", erro}}.dump(),
                        "application/json"
                    );
                    return;
                }

                res.set_content(
                    json{
                        {"sucesso", true},
                        {"id", id},
                        {"mensagem", "Documento criptografado e salvo!"}
                    }.dump(),
                    "application/json"
                );

            } catch (const std::exception& e) {
                res.status = 500;
                res.set_content(
                    json{{"sucesso", false}, {"erro", e.what()}}.dump(),
                    "application/json"
                );
            }
        }
    );

    // ==========================================
    // LISTAR DOCUMENTOS
    // ==========================================

    servidor.Post("/api/documents/list",
        [](const httplib::Request& req, httplib::Response& res) {

            try {
                json dados = json::parse(req.body);

                std::string senha = dados.value("senha", "");

                if (senha.empty()) {
                    res.status = 400;
                    res.set_content(
                        R"({"sucesso":false,"erro":"Senha mestra nao informada."})",
                        "application/json"
                    );
                    return;
                }

                json documentos;
                std::string erro;

                bool sucesso = DocumentStore::list(
                    senha,
                    documentos,
                    erro
                );

                if (!sucesso) {
                    res.status = 400;
                    res.set_content(
                        json{
                            {"sucesso", false},
                            {"erro", erro}
                        }.dump(),
                        "application/json"
                    );
                    return;
                }

                res.set_content(
                    json{
                        {"sucesso", true},
                        {"documentos", documentos}
                    }.dump(),
                    "application/json"
                );

            } catch (const std::exception& e) {
                res.status = 400;
                res.set_content(
                    json{
                        {"sucesso", false},
                        {"erro", e.what()}
                    }.dump(),
                    "application/json"
                );
            }
        }
    );

    // ==========================================
    // BAIXAR DOCUMENTO DESCRIPTOGRAFADO
    // ==========================================

    servidor.Post("/api/documents/download",
        [](const httplib::Request& req, httplib::Response& res) {

            try {
                json dados = json::parse(req.body);

                std::string senha = dados.value("senha", "");
                std::string id = dados.value("id", "");

                std::vector<unsigned char> conteudo;
                std::string nome;
                std::string erro;

                bool sucesso = DocumentStore::download(
                    senha,
                    id,
                    conteudo,
                    nome,
                    erro
                );

                if (!sucesso) {
                    res.status = 400;
                    res.set_content(
                        json{{"erro", erro}}.dump(),
                        "application/json"
                    );
                    return;
                }

                std::string extensao =
                    std::filesystem::path(nome).extension().string();

                std::string tipo =
                    (extensao == ".pdf" || extensao == ".PDF")
                    ? "application/pdf"
                    : "image/jpeg";

                const std::string arquivo(
                    conteudo.begin(),
                    conteudo.end()
                );

                res.set_header(
                    "Content-Disposition",
                    "attachment; filename=\"documento" + extensao + "\""
                );

                res.set_content(arquivo, tipo);

            } catch (...) {
                res.status = 400;
                res.set_content(
                    R"({"erro":"Requisicao invalida."})",
                    "application/json"
                );
            }
        }
    );

    // ==========================================
    // EXCLUIR DOCUMENTO
    // ==========================================

    servidor.Post("/api/documents/delete",
        [](const httplib::Request& req, httplib::Response& res) {

            try {
                json dados = json::parse(req.body);

                std::string senha = dados.value("senha", "");
                std::string id = dados.value("id", "");

                std::string erro;

                bool sucesso = DocumentStore::remove(
                    senha,
                    id,
                    erro
                );

                if (!sucesso) {
                    res.status = 400;
                    res.set_content(
                        json{
                            {"sucesso", false},
                            {"erro", erro}
                        }.dump(),
                        "application/json"
                    );
                    return;
                }

                res.set_content(
                    R"({"sucesso":true,"mensagem":"Documento excluido!"})",
                    "application/json"
                );

            } catch (...) {
                res.status = 400;
                res.set_content(
                    R"({"sucesso":false,"erro":"Requisicao invalida."})",
                    "application/json"
                );
            }
        }
    );

    // ==========================================
    // INICIAR SERVIDOR
    // ==========================================
    
    std::cout << "\n";
    std::cout << "==============================\n";
    std::cout << "       SECUREKEY SERVER       \n";
    std::cout << "==============================\n";
    std::cout << "Servidor iniciado.\n";
    std::cout << "Cofre criptografado habilitado.\n";
    std::cout << "Documentos protegidos habilitados.\n\n";

    const char* portaAmbiente = std::getenv("PORT");

    int porta = 8080;

    if (portaAmbiente != nullptr) {
        try {
            porta = std::stoi(portaAmbiente);
        } catch (...) {
            std::cerr << "Porta invalida!\n";
            return 1;
        }
    }

    std::cout << "SecureKey iniciado na porta "
              << porta << std::endl;

    if (!servidor.listen("0.0.0.0", porta)) {
        std::cerr << "Erro ao iniciar servidor.\n";
        return 1;
    }

    return 0;
}
