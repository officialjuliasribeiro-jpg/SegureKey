const categorias = {
    pessoal: {
        titulo: "Organizador financeiro",
        descricao: "Controle suas receitas, despesas e investimentos.",
        campos: [
            ["mes", "Mês de referência (AAAA-MM)", "month", true],
            ["tipo", "Tipo de movimentação (Receita, Despesa ou Investimento)", "text", true],
            ["categoria", "Categoria", "text", true],
            ["titulo", "Descrição da movimentação", "text", true],
            ["planejado", "Valor planejado (R$)", "number", false],
            ["realizado", "Valor realizado (R$)", "number", true],
            ["data", "Data da movimentação", "date", false],
            ["informacao", "Observações", "textarea", false]
        ]
    },

    senhas: {
        titulo: "Gerenciador de senhas",
        campos: [
            ["servico", "Nome do serviço", "text", true],
            ["site", "Endereço do site", "url", false],
            ["usuario", "Nome de usuário ou e-mail", "text", true],
            ["senha", "Senha da conta", "password", true],
            ["observacoes", "Observações", "textarea", false]
        ]
    },

    financeiro: {
        titulo: "Cofre financeiro"
        ,campos: [
            ["instituicao", "Instituição", "text", true],
            ["tipo", "Tipo de registro", "text", true],
            ["identificacao", "Identificação da conta", "text", true],
            ["titular", "Titular", "text", false],
            ["observacoes", "Observações", "textarea", false]
        ]
    },

    documentos: {
        titulo: "Documentos protegidos",
        campos: [
            ["nome", "Nome do documento", "text", true],
            ["tipo", "Categoria", "text", true],
            ["identificacao", "Identificação", "text", false],
            ["descricao", "Descrição", "textarea", true]
        ]
    },

    mensagens: {
        titulo: "Mensagens privadas",
        campos: [
            ["assunto", "Assunto", "text", true],
            ["destinatario", "Destinatário", "text", false],
            ["mensagem", "Conteúdo da mensagem", "textarea", true]
        ]
    }
};

let paginaAtual = "dashboard";
let registrosAtuais = [];
let indiceEdicao = null;
let cofreDesbloqueado = false;


// ELEMENTOS
const senhaMestra = document.getElementById("master-password");
const authMessage = document.getElementById("auth-message");
const recordList = document.getElementById("record-list");
const dynamicFields = document.getElementById("dynamic-fields");


// NAVEGAÇÃO
function navegar(pagina) {

    paginaAtual = pagina;
    indiceEdicao = null;

    const painelCriptografia = document.getElementById("message-crypto-panel");
    if (painelCriptografia) {
        painelCriptografia.classList.toggle("hidden", pagina !== "mensagens");
    }

    document.querySelectorAll(".page").forEach(element => {
        element.classList.remove("active");
    });

    document.querySelectorAll(".nav").forEach(element => {
        element.classList.toggle(
            "active",
            element.dataset.page === pagina
        );
    });

    if (pagina === "dashboard") {

        document.getElementById("dashboard").classList.add("active");
        document.getElementById("page-title").textContent = "Dashboard";

    } else {

        document.getElementById("records-page").classList.add("active");

        document.getElementById("page-title").textContent =
            categorias[pagina].titulo;

        const isDocumentsPage = pagina === "documentos";
        const isOrganizerPage = pagina === "pessoal";
        document.getElementById("documents-panel")?.classList.toggle("hidden", !isDocumentsPage);
        document.getElementById("finance-organizer")?.classList.toggle("hidden", !isOrganizerPage);
        document.getElementById("records-form-card")?.classList.toggle("hidden", isDocumentsPage || isOrganizerPage);
        document.getElementById("records-list-card")?.classList.toggle("hidden", isDocumentsPage || isOrganizerPage);

        if (!isDocumentsPage && !isOrganizerPage) montarFormulario(pagina);

        if (cofreDesbloqueado) {
            if (isDocumentsPage) carregarDocumentos();
            else if (isOrganizerPage) carregarOrganizadorFinanceiro();
            else carregarRegistros();
        } else if (isDocumentsPage) {
            document.getElementById("documents-list").textContent =
                "Desbloqueie o cofre para visualizar os documentos.";
        } else if (isOrganizerPage) {
            document.getElementById("finance-organizer-content").innerHTML =
                '<div class="fo-empty">Desbloqueie o cofre para abrir seu planejamento financeiro.</div>';
        } else {
            recordList.textContent =
                "Desbloqueie o cofre para visualizar os registros.";
        }
    }

    document.getElementById("record-feedback").textContent = "";
}

document.querySelectorAll("[data-page]").forEach(button => {

    button.addEventListener("click", () => {
        navegar(button.dataset.page);
    });

});


// MONTAR FORMULÁRIO DINÂMICO
function montarFormulario(categoria) {

    const configuracao = categorias[categoria];

    document.getElementById("module-heading").textContent =
        configuracao.titulo;

    document.getElementById("module-description").textContent =
        configuracao.descricao;

    dynamicFields.replaceChildren();

    configuracao.campos.forEach(campo => {

        const [nome, titulo, tipo, obrigatorio] = campo;

        const container = document.createElement("div");
        container.className = "field";

        if (tipo === "textarea") {
            container.classList.add("full");
        }

        const label = document.createElement("label");
        label.textContent = titulo;

        const input = document.createElement(
            tipo === "textarea" ? "textarea" : "input"
        );

        input.id = "field-" + nome;
        input.name = nome;

        if (tipo !== "textarea") {
            input.type = tipo;
        }

        input.required = obrigatorio;
        input.maxLength = nome === "senha" ? 128 : 10000;

        if (nome === "senha") {
            const passwordRow = document.createElement("div");
            passwordRow.className = "password-input-row";
            passwordRow.style.display = "flex";
            passwordRow.style.gap = "8px";
            passwordRow.style.alignItems = "center";

            input.style.flex = "1";

            const toggle = document.createElement("button");
            toggle.type = "button";
            toggle.className = "secondary-button";
            toggle.textContent = "Mostrar";
            toggle.setAttribute("aria-label", "Mostrar ou ocultar senha");

            toggle.addEventListener("click", () => {
                const showing = input.type === "text";
                input.type = showing ? "password" : "text";
                toggle.textContent = showing ? "Mostrar" : "Ocultar";
            });

            passwordRow.append(input, toggle);
            container.append(label, passwordRow);
        } else {
            container.append(label, input);
        }

        dynamicFields.appendChild(container);

    });

}


// OBTER DADOS DO FORMULÁRIO
function obterDadosFormulario(categoria) {

    const configuracao = categorias[categoria];
    const dados = {};

    for (const campo of configuracao.campos) {

        const nome = campo[0];

        const input = document.getElementById("field-" + nome);

        if (!input) continue;

        dados[nome] = input.value.trim();

    }

    return dados;
}


// PREENCHER FORMULÁRIO
function preencherFormulario(categoria, dados) {

    montarFormulario(categoria);

    categorias[categoria].campos.forEach(campo => {

        const nome = campo[0];
        const input = document.getElementById("field-" + nome);

        if (input) {
            input.value = dados[nome] || "";
        }

    });

}


// AUTENTICAÇÃO
document.getElementById("unlock-button")
.addEventListener("click", async () => {

    const senha = senhaMestra.value;

    if (senha.length < 12) {
        authMessage.textContent =
            "A senha mestra precisa ter pelo menos 12 caracteres.";
        return;
    }

    authMessage.textContent = "Verificando cofre...";

    try {

        const response = await fetch("/api/vault/list", {
            method: "POST",
            headers: {
                "Content-Type": "application/json"
            },
            body: JSON.stringify({ senha })
        });

        const resultado = await response.json();

        if (!resultado.success) {
            throw new Error(resultado.error);
        }

        cofreDesbloqueado = true;

        authMessage.textContent = "Cofre desbloqueado com sucesso.";

        atualizarContadores(resultado.registros);
        carregarDocumentos();
        if (paginaAtual === "pessoal") carregarOrganizadorFinanceiro();
        else if (paginaAtual !== "dashboard" && paginaAtual !== "documentos") carregarRegistros();

    } catch (error) {

        cofreDesbloqueado = false;

        authMessage.textContent = error.message;

    }

});


// CONTADORES
function atualizarContadores(registros) {

    const contagem = {
        senhas: 0,
        financeiro: 0,
        mensagens: 0
    };

    registros.forEach(registro => {

        if (contagem[registro.categoria] !== undefined) {
            contagem[registro.categoria]++;
        }

    });

    Object.keys(contagem).forEach(categoria => {

        document.getElementById("count-" + categoria).textContent =
            contagem[categoria];

    });

}


// SALVAR REGISTRO
document.getElementById("record-form")
.addEventListener("submit", async event => {

    event.preventDefault();

    if (!cofreDesbloqueado) {
        alert("Desbloqueie o cofre primeiro.");
        return;
    }

    const dados = obterDadosFormulario(paginaAtual);

    const titulo =
        dados.servico ||
        dados.instituicao ||
        dados.nome ||
        dados.assunto ||
        dados.titulo;

    const informacao = JSON.stringify(dados);

    const feedback = document.getElementById("record-feedback");

    feedback.textContent = "Salvando registro...";

    try {

        const editando = indiceEdicao !== null;

        const response = await fetch(
            editando ? "/api/vault/edit" : "/api/vault/add",
            {
                method: "POST",
                headers: {
                    "Content-Type": "application/json"
                },
                body: JSON.stringify(
                    editando
                    ? {
                        senha: senhaMestra.value,
                        indice: indiceEdicao,
                        titulo,
                        informacao
                    }
                    : {
                        senha: senhaMestra.value,
                        categoria: paginaAtual,
                        titulo,
                        informacao
                    }
                )
            }
        );

        const resultado = await response.json();

        if (!resultado.success) {
            throw new Error(resultado.error);
        }

        feedback.textContent = resultado.message;

        indiceEdicao = null;

        document.getElementById("cancel-edit").classList.add("hidden");

        event.target.reset();

        await carregarRegistros();

    } catch (error) {

        feedback.textContent = "Erro: " + error.message;

    }

});


// CARREGAR REGISTROS
async function carregarRegistros() {

    if (!cofreDesbloqueado) return;

    recordList.textContent = "Carregando registros...";

    try {

        const response = await fetch("/api/vault/list", {
            method: "POST",
            headers: {
                "Content-Type": "application/json"
            },
            body: JSON.stringify({
                senha: senhaMestra.value
            })
        });

        const resultado = await response.json();

        if (!resultado.success) {
            throw new Error(resultado.error);
        }

        registrosAtuais = resultado.registros;

        atualizarContadores(registrosAtuais);
        renderizarRegistros();

    } catch (error) {

        recordList.textContent = error.message;

    }

}


// RENDERIZAR REGISTROS
function renderizarRegistros() {

    recordList.replaceChildren();

    const pesquisa = document.getElementById("record-search")
        .value.toLowerCase();

    const filtrados = registrosAtuais
        .map((registro, indice) => ({ registro, indice }))
        .filter(item => {

            const categoria = item.registro.categoria || "pessoal";

            return categoria === paginaAtual &&
                (
                    item.registro.titulo.toLowerCase().includes(pesquisa) ||
                    item.registro.informacao.toLowerCase().includes(pesquisa)
                );

        });

    document.getElementById("record-count").textContent =
        filtrados.length + " registros";

    if (filtrados.length === 0) {

        recordList.textContent = "Nenhum registro encontrado.";
        return;

    }

    filtrados.forEach(item => {

        const registro = item.registro;
        const indice = item.indice;

        const card = document.createElement("article");
        card.className = "record-item";

        const title = document.createElement("h3");
        title.textContent = registro.titulo;

        const content = document.createElement("p");

        let dados;

        try {
            dados = JSON.parse(registro.informacao);
        } catch {
            dados = null;
        }

        if (dados && typeof dados === "object" && !Array.isArray(dados)) {

            Object.entries(dados).forEach(([chave, valor]) => {

                const linha = document.createElement("div");
                linha.textContent = chave + ": " +
    (chave === "senha" ? "••••••••" : valor);
                content.appendChild(linha);

            });

        } else {

            content.textContent = registro.informacao;

        }

        const actions = document.createElement("div");
        actions.className = "record-actions";

        const edit = document.createElement("button");
        edit.textContent = "Editar";

        edit.onclick = () => editarRegistro(indice);

        const remove = document.createElement("button");
        remove.textContent = "Excluir";
        remove.className = "delete";

        remove.onclick = () => excluirRegistro(indice);

        actions.append(edit, remove);
        card.append(title, content, actions);

        recordList.appendChild(card);

    });

}


// EDITAR REGISTRO
function editarRegistro(indice) {

    const registro = registrosAtuais[indice];

    let dados;

    try {
        dados = JSON.parse(registro.informacao);
    } catch {
        dados = {
            titulo: registro.titulo,
            informacao: registro.informacao
        };
    }

    indiceEdicao = indice;

    preencherFormulario(paginaAtual, dados);

    document.getElementById("cancel-edit").classList.remove("hidden");

    document.getElementById("record-feedback").textContent =
        "Editando registro. Salve para confirmar.";

    document.getElementById("record-form").scrollIntoView({
        behavior: "smooth"
    });

}


// CANCELAR EDIÇÃO
document.getElementById("cancel-edit")
.addEventListener("click", () => {

    indiceEdicao = null;

    montarFormulario(paginaAtual);

    document.getElementById("cancel-edit").classList.add("hidden");

    document.getElementById("record-feedback").textContent =
        "Edição cancelada.";

});


// EXCLUIR REGISTRO
async function excluirRegistro(indice) {

    if (!confirm("Deseja realmente excluir este registro?")) {
        return;
    }

    try {

        const response = await fetch("/api/vault/delete", {
            method: "POST",
            headers: {
                "Content-Type": "application/json"
            },
            body: JSON.stringify({
                senha: senhaMestra.value,
                indice
            })
        });

        const resultado = await response.json();

        if (!resultado.success) {
            throw new Error(resultado.error);
        }

        indiceEdicao = null;

        await carregarRegistros();

    } catch (error) {

        alert(error.message);

    }

}


// PESQUISA
document.getElementById("record-search")
.addEventListener("input", renderizarRegistros);


// DOCUMENTOS PROTEGIDOS
async function carregarDocumentos() {
    if (!cofreDesbloqueado) return;

    const lista = document.getElementById("documents-list");
    lista.textContent = "Carregando documentos...";

    try {
        const response = await fetch("/api/documents/list", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ senha: senhaMestra.value })
        });

        const resultado = await response.json();
        if (!response.ok) {
            throw new Error(resultado.erro || resultado.error || "Não foi possível listar os documentos.");
        }

        const documentos = resultado.documentos || [];
        document.getElementById("count-documentos").textContent = documentos.length;
        renderizarDocumentos(documentos);
    } catch (error) {
        lista.textContent = "Erro: " + error.message;
    }
}

function renderizarDocumentos(documentos) {
    const lista = document.getElementById("documents-list");
    lista.replaceChildren();

    if (!documentos.length) {
        lista.textContent = "Nenhum documento guardado ainda.";
        return;
    }

    documentos.forEach(documento => {
        const card = document.createElement("article");
        card.className = "record-item";

        const titulo = document.createElement("h3");
        titulo.textContent = documento.nome || "Documento";

        const detalhe = document.createElement("p");
        const tamanhoMB = (Number(documento.tamanho || 0) / (1024 * 1024)).toFixed(2);
        detalhe.textContent = `${(documento.extensao || "").toUpperCase()} · ${tamanhoMB} MB`;

        const actions = document.createElement("div");
        actions.className = "record-actions";

        const download = document.createElement("button");
        download.type = "button";
        download.textContent = "Baixar / visualizar";
        download.addEventListener("click", () => baixarDocumento(documento.id));

        const remove = document.createElement("button");
        remove.type = "button";
        remove.textContent = "Excluir";
        remove.className = "delete";
        remove.addEventListener("click", () => excluirDocumento(documento.id, documento.nome));

        actions.append(download, remove);
        card.append(titulo, detalhe, actions);
        lista.appendChild(card);
    });
}

document.getElementById("document-upload-form").addEventListener("submit", async event => {
    event.preventDefault();

    if (!cofreDesbloqueado) {
        alert("Desbloqueie o cofre primeiro.");
        return;
    }

    const input = document.getElementById("document-file");
    const feedback = document.getElementById("document-feedback");
    const arquivo = input.files[0];

    if (!arquivo) {
        feedback.textContent = "Escolha um arquivo primeiro.";
        return;
    }

    const extensao = arquivo.name.split(".").pop().toLowerCase();
    if (!["pdf", "jpg", "jpeg"].includes(extensao)) {
        feedback.textContent = "Formato não permitido. Selecione PDF, JPG ou JPEG.";
        return;
    }

    if (arquivo.size === 0 || arquivo.size > 15 * 1024 * 1024) {
        feedback.textContent = "O arquivo deve ter até 15 MB.";
        return;
    }

    const senhaAtual = senhaMestra.value;
    if (!senhaAtual || senhaAtual.length < 12) {
        feedback.textContent = "A senha mestra não foi identificada. Desbloqueie o cofre novamente.";
        cofreDesbloqueado = false;
        return;
    }

    const formData = new FormData();
formData.append("arquivo", arquivo);

    feedback.textContent = "Criptografando e guardando documento...";

    try {
        const response = await fetch("/api/documents/upload", {
    method: "POST",
    headers: {
        "X-Master-Password": senhaAtual
    },
    body: formData
});

        const textoResposta = await response.text();
        let resultado = {};
        try { resultado = textoResposta ? JSON.parse(textoResposta) : {}; }
        catch { resultado = { erro: textoResposta || "Resposta inválida do servidor." }; }
        if (!response.ok || !(resultado.sucesso || resultado.success)) {
            throw new Error(resultado.erro || resultado.error || `Falha ao guardar documento (HTTP ${response.status}).`);
        }

        feedback.textContent = "Documento guardado e criptografado com sucesso.";
        input.value = "";
        await carregarDocumentos();
    } catch (error) {
        feedback.textContent = "Erro: " + error.message;
    }
});

async function baixarDocumento(id) {
    try {
        const response = await fetch("/api/documents/download", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ senha: senhaMestra.value, id })
        });

        if (!response.ok) {
            let mensagem = "Não foi possível recuperar o documento.";
            try {
                const erro = await response.json();
                mensagem = erro.erro || erro.error || mensagem;
            } catch {}
            throw new Error(mensagem);
        }

        const blob = await response.blob();
        const url = URL.createObjectURL(blob);
        const link = document.createElement("a");
        link.href = url;

        const disposition = response.headers.get("Content-Disposition") || "";
        const nomeArquivo = disposition.match(/filename="?([^";]+)"?/i);
        link.download = nomeArquivo ? nomeArquivo[1] : "documento";
        document.body.appendChild(link);
        link.click();
        link.remove();
        URL.revokeObjectURL(url);
    } catch (error) {
        alert("Erro: " + error.message);
    }
}

async function excluirDocumento(id, nome) {
    if (!confirm(`Deseja excluir o documento "${nome}"?`)) return;

    try {
        const response = await fetch("/api/documents/delete", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ senha: senhaMestra.value, id })
        });

        const resultado = await response.json();
        if (!response.ok || !resultado.sucesso) {
            throw new Error(resultado.erro || resultado.error || "Não foi possível excluir.");
        }

        document.getElementById("document-feedback").textContent =
            "Documento excluído.";
        await carregarDocumentos();
    } catch (error) {
        alert("Erro: " + error.message);
    }
}


// CRIPTOGRAFIA INTEGRADA ÀS MENSAGENS PRIVADAS
let ultimoResultadoCrypto = "";

async function executarCriptografiaMensagem(operacao) {
    const campoMensagem = document.getElementById("crypto-message");
    const resultadoArea = document.getElementById("crypto-result");
    const feedback = document.getElementById("crypto-feedback");
    const botaoUsar = document.getElementById("crypto-use-result");
    const mensagem = campoMensagem.value;

    if (!mensagem.trim()) {
        feedback.textContent = "Digite uma mensagem antes de continuar.";
        return;
    }

    feedback.textContent = operacao === "encrypt"
        ? "Criptografando mensagem..."
        : "Recuperando mensagem...";
    resultadoArea.textContent = "Processando...";
    botaoUsar.disabled = true;
    ultimoResultadoCrypto = "";

    try {
        const response = await fetch("/api/hill/" + operacao, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ mensagem })
        });

        const resultado = await response.json();

        if (!response.ok || !resultado.success) {
            throw new Error(resultado.error || "Não foi possível concluir a operação.");
        }

        ultimoResultadoCrypto = operacao === "encrypt"
            ? resultado.criptografada
            : resultado.descriptografada;

        if (typeof ultimoResultadoCrypto !== "string" || !ultimoResultadoCrypto.length) {
            throw new Error("O servidor não retornou um resultado válido.");
        }

        resultadoArea.textContent = ultimoResultadoCrypto;
        feedback.textContent = operacao === "encrypt"
            ? "Mensagem transformada. Você pode usar o resultado no formulário abaixo."
            : "Mensagem recuperada. Você pode usar o resultado no formulário abaixo.";
        botaoUsar.disabled = false;

    } catch (error) {
        resultadoArea.textContent = "Não foi possível processar a mensagem.";
        feedback.textContent = "Erro: " + error.message;
    }
}

document.getElementById("crypto-encrypt")
    .addEventListener("click", () => executarCriptografiaMensagem("encrypt"));

document.getElementById("crypto-decrypt")
    .addEventListener("click", () => executarCriptografiaMensagem("decrypt"));

document.getElementById("crypto-use-result")
    .addEventListener("click", () => {
        const campoFormulario = document.getElementById("field-mensagem");
        const feedback = document.getElementById("record-feedback");

        if (!campoFormulario || !ultimoResultadoCrypto) {
            document.getElementById("crypto-feedback").textContent =
                "Abra Mensagens privadas para usar o resultado no formulário.";
            return;
        }

        campoFormulario.value = ultimoResultadoCrypto;
        campoFormulario.scrollIntoView({ behavior: "smooth", block: "center" });
        feedback.textContent =
            "Resultado da criptografia inserido. Salve para armazená-lo no cofre protegido.";
    });


// STATUS DO SERVIDOR
async function verificarServidor() {

    const status = document.getElementById("server-status");

    try {

        const response = await fetch("/api/status");
        const resultado = await response.json();

        status.textContent =
            resultado.status === "online" ? "Online" : "Indisponível";

    } catch {

        status.textContent = "Offline";

    }

}

verificarServidor();