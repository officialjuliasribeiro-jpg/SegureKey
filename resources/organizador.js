(() => {
    const content = document.getElementById("finance-organizer-content");
    const monthInput = document.getElementById("fo-month");
    if (!content || !monthInput) return;

    const groups = {
        essenciais: { label: "Despesas essenciais", categories: ["Moradia", "Condomínio", "Aluguel", "Transporte", "Alimentação", "Supermercado", "Luz", "Água", "Internet", "Celular", "Saúde", "Plano de saúde", "Educação", "Cartão de crédito", "Empréstimo", "Outros essenciais"] },
        lazer: { label: "Lazer", categories: ["Alimentação extra", "Assinaturas", "Festas e bares", "Hobbies", "Confraternização", "Outros lazer"] },
        educacao: { label: "Educação", categories: ["Cursos", "Faculdade", "Livros e materiais", "Outros educação"] },
        liberdade: { label: "Liberdade financeira", categories: ["Investimentos", "Objetivos", "Outros investimentos"] },
        reserva: { label: "Reserva de emergência", categories: ["Reserva de emergência"] }
    };
    const incomeCategories = ["Salário", "Renda extra", "Benefícios", "Outras receitas"];
    let vaultRecords = [];
    let activeTab = "acompanhamento";
    let editingIndex = null;

    const money = value => (Number(value) || 0).toLocaleString("pt-BR", { style: "currency", currency: "BRL" });
    const currentMonth = () => {
        const now = new Date();
        return `${now.getFullYear()}-${String(now.getMonth()+1).padStart(2,"0")}`;
    };
    monthInput.value = currentMonth();

    function getEntries() {
        return vaultRecords.map((record, index) => {
            let data;
            try { data = JSON.parse(record.informacao || "{}"); } catch { data = {}; }
            if (!data || data.modelo !== "securekey-financeiro-v1") return null;
            return { ...data, index, titulo: record.titulo };
        }).filter(Boolean).filter(item => item.mes === monthInput.value);
    }

    function totals(entries) {
        const receitas = entries.filter(x => x.grupo === "receitas").reduce((s,x)=>s+Number(x.realizado||0),0);
        const essenciais = entries.filter(x => x.grupo === "essenciais").reduce((s,x)=>s+Number(x.realizado||0),0);
        const lazer = entries.filter(x => x.grupo === "lazer").reduce((s,x)=>s+Number(x.realizado||0),0);
        const investimentos = entries.filter(x => ["educacao","liberdade","reserva"].includes(x.grupo)).reduce((s,x)=>s+Number(x.realizado||0),0);
        const saidas = entries.filter(x => x.grupo !== "receitas");
        const planejado = saidas.reduce((s,x)=>s+Number(x.planejado||0),0);
        const realizado = saidas.reduce((s,x)=>s+Number(x.realizado||0),0);
        return { receitas, essenciais, lazer, investimentos, despesas: essenciais+lazer, saldo: receitas-essenciais-lazer-investimentos, planejado, realizado };
    }

    function card(label, value, hint, cls="") {
        return `<article class="fo-stat ${cls}"><span>${label}</span><strong>${money(value)}</strong><small>${hint}</small></article>`;
    }

    function entryRows(entries, groupKey) {
        const selected = entries.filter(x => x.grupo === groupKey);
        if (!selected.length) return `<tr><td colspan="5" class="fo-table-empty">Nenhum lançamento neste grupo neste mês.</td></tr>`;
        return selected.map(item => `<tr>
            <td><strong>${escapeHtml(item.categoria)}</strong><small>${escapeHtml(item.descricao || "Sem observações")}</small></td>
            <td class="fo-number">${money(item.planejado)}</td>
            <td class="fo-number">${money(item.realizado)}</td>
            <td class="fo-number ${Number(item.planejado)-Number(item.realizado)<0?'negative':'positive'}">${money(Number(item.planejado)-Number(item.realizado))}</td>
            <td class="fo-row-actions"><button type="button" data-fo-edit="${item.index}">Editar</button><button type="button" class="danger" data-fo-delete="${item.index}">Excluir</button></td>
        </tr>`).join("");
    }

    function sectionTable(title, groupKey, entries) {
        const rows = entryRows(entries, groupKey);
        const selected = entries.filter(x => x.grupo === groupKey);
        const planned = selected.reduce((s,x)=>s+Number(x.planejado||0),0);
        const actual = selected.reduce((s,x)=>s+Number(x.realizado||0),0);
        return `<section class="fo-section"><div class="fo-section-head"><div><h3>${title}</h3><p>Planejado, realizado e saldo disponível</p></div><span>${money(actual)} realizado</span></div>
            <div class="fo-table-scroll"><table class="fo-table"><thead><tr><th>Categoria</th><th>Planejado</th><th>Realizado</th><th>Disponível</th><th>Ações</th></tr></thead><tbody>${rows}</tbody><tfoot><tr><th>Total</th><th>${money(planned)}</th><th>${money(actual)}</th><th class="${planned-actual<0?'negative':'positive'}">${money(planned-actual)}</th><th></th></tr></tfoot></table></div></section>`;
    }

    function renderForm() {
        const isIncome = document.getElementById("fo-group")?.value === "receitas";
        const group = document.getElementById("fo-group")?.value || "essenciais";
        const select = document.getElementById("fo-category");
        if (!select) return;
        const choices = isIncome ? incomeCategories : (groups[group]?.categories || []);
        const previous = select.value;
        select.innerHTML = choices.map(x=>`<option value="${escapeAttr(x)}">${escapeHtml(x)}</option>`).join("");
        if (choices.includes(previous)) select.value = previous;
    }

    function formMarkup() {
        return `<section class="fo-section fo-entry-section"><div class="fo-section-head"><div><h3 id="fo-form-title">Adicionar movimentação</h3><p>Registre receitas, despesas e valores que deseja reservar.</p></div><span class="fo-pill">NOVO LANÇAMENTO</span></div>
            <form id="fo-entry-form" class="fo-form">
                <div class="fo-field"><label for="fo-group">Tipo de movimentação</label><select id="fo-group"><option value="receitas">Receita</option><option value="essenciais">Despesa essencial</option><option value="lazer">Lazer</option><option value="educacao">Educação</option><option value="liberdade">Liberdade financeira</option><option value="reserva">Reserva de emergência</option></select></div>
                <div class="fo-field"><label for="fo-category">Categoria</label><select id="fo-category"></select></div>
                <div class="fo-field"><label for="fo-planned">Valor planejado (R$)</label><input id="fo-planned" type="number" min="0" step="0.01" placeholder="0,00"></div>
                <div class="fo-field"><label for="fo-actual">Valor realizado (R$)</label><input id="fo-actual" type="number" min="0" step="0.01" placeholder="0,00" required></div>
                <div class="fo-field fo-wide"><label for="fo-description">Observação (opcional)</label><input id="fo-description" type="text" maxlength="140" placeholder="Ex.: conta de energia, salário, mercado..."></div>
                <div class="fo-form-actions"><button class="fo-primary" type="submit" id="fo-save">Salvar movimentação</button><button class="fo-secondary hidden" type="button" id="fo-cancel">Cancelar edição</button><span id="fo-feedback" role="status"></span></div>
            </form></section>`;
    }

    function render() {
        const entries = getEntries();
        const t = totals(entries);
        const header = `<div class="fo-summary-grid">${card("Receitas realizadas",t.receitas,"Dinheiro que entrou","income")}${card("Despesas essenciais",t.essenciais,"Contas e necessidades","expense")}${card("Lazer",t.lazer,"Gastos não essenciais","leisure")}${card("Investimentos e reservas",t.investimentos,"Valores destinados ao futuro","saving")}${card("Saldo do mês",t.saldo,"Receitas menos despesas e reservas",t.saldo<0?"negative-card":"balance")}</div>`;
        let body = "";
        if (activeTab === "acompanhamento") {
            body = `<div class="fo-intro"><div><h3>Acompanhamento mensal</h3><p>Veja cada grupo e acompanhe o que foi previsto e o que realmente aconteceu.</p></div><span class="fo-month-chip">${monthInput.value}</span></div>
                ${sectionTable("Receitas", "receitas", entries)}
                ${sectionTable("Despesas essenciais", "essenciais", entries)}
                ${sectionTable("Lazer", "lazer", entries)}
                ${sectionTable("Educação", "educacao", entries)}
                ${sectionTable("Liberdade financeira", "liberdade", entries)}
                ${sectionTable("Reserva de emergência", "reserva", entries)}`;
        } else {
            const groupsForControl = ["essenciais","lazer","educacao","liberdade","reserva"];
            const rows = groupsForControl.map(key => {
                const subset = entries.filter(x=>x.grupo===key);
                const planned=subset.reduce((s,x)=>s+Number(x.planejado||0),0);
                const actual=subset.reduce((s,x)=>s+Number(x.realizado||0),0);
                return `<tr><td><strong>${groups[key].label}</strong></td><td>${money(planned)}</td><td>${money(actual)}</td><td class="${planned-actual<0?'negative':'positive'}">${money(planned-actual)}</td></tr>`;
            }).join("");
            body = `<div class="fo-intro"><div><h3>Controle do orçamento</h3><p>Compare o limite planejado com os gastos realizados em cada área.</p></div><span class="fo-month-chip">${monthInput.value}</span></div>
                <section class="fo-section"><div class="fo-table-scroll"><table class="fo-table fo-control-table"><thead><tr><th>Tipo de fluxo</th><th>Planejado</th><th>Gasto atual</th><th>Disponível</th></tr></thead><tbody>${rows}</tbody><tfoot><tr><th>TOTAL</th><th>${money(t.planejado)}</th><th>${money(t.realizado)}</th><th class="${t.planejado-t.realizado<0?'negative':'positive'}">${money(t.planejado-t.realizado)}</th></tr></tfoot></table></div></section>
                <div class="fo-control-note"><strong>Como interpretar:</strong> disponível positivo significa que ainda existe valor dentro do planejamento; negativo indica que o realizado ultrapassou o planejado. Registre também receitas na aba Acompanhamento para visualizar o saldo mensal.</div>`;
        }
        content.innerHTML = header + `<div class="fo-view">${body}</div>` + formMarkup();
        renderForm();
        bindForm();
        content.querySelectorAll("[data-fo-edit]").forEach(button=>button.addEventListener("click",()=>editEntry(Number(button.dataset.foEdit))));
        content.querySelectorAll("[data-fo-delete]").forEach(button=>button.addEventListener("click",()=>deleteEntry(Number(button.dataset.foDelete))));
    }

    function escapeHtml(value) { return String(value ?? "").replace(/[&<>"']/g, ch=>({"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;","'":"&#39;"}[ch])); }
    function escapeAttr(value) { return escapeHtml(value); }

    function bindForm() {
        const form = document.getElementById("fo-entry-form");
        document.getElementById("fo-group").addEventListener("change",renderForm);
        form.addEventListener("submit",saveEntry);
        document.getElementById("fo-cancel").addEventListener("click",()=>{editingIndex=null;render();});
    }

    async function saveEntry(event) {
        event.preventDefault();
        const group = document.getElementById("fo-group").value;
        const category = document.getElementById("fo-category").value;
        const planned = Number(document.getElementById("fo-planned").value || 0);
        const actual = Number(document.getElementById("fo-actual").value || 0);
        const description = document.getElementById("fo-description").value.trim();
        if (!Number.isFinite(planned) || !Number.isFinite(actual) || planned < 0 || actual < 0) {
            document.getElementById("fo-feedback").textContent="Informe valores válidos e não negativos."; return;
        }
        const data = { modelo:"securekey-financeiro-v1", mes:monthInput.value, grupo, categoria, planejado:planned, realizado:actual, descricao:description };
        const title = `${monthInput.value} · ${category}`;
        const feedback = document.getElementById("fo-feedback");
        feedback.textContent = "Salvando...";
        try {
            const edit = editingIndex !== null;
            const response = await fetch(edit?"/api/vault/edit":"/api/vault/add",{
                method:"POST",headers:{"Content-Type":"application/json"},
                body:JSON.stringify(edit?{senha:document.getElementById("master-password").value,indice:editingIndex,titulo:title,informacao:JSON.stringify(data)}:{senha:document.getElementById("master-password").value,categoria:"pessoal",titulo:title,informacao:JSON.stringify(data)})
            });
            const result=await response.json();
            if(!result.success) throw new Error(result.error||"Não foi possível salvar.");
            editingIndex=null;
            await window.carregarOrganizadorFinanceiro();
        } catch(error) { feedback.textContent="Erro: "+error.message; }
    }

    function editEntry(index) {
        const record=vaultRecords[index]; if(!record) return;
        let data={}; try{data=JSON.parse(record.informacao||"{}");}catch{}
        if(data.modelo!=="securekey-financeiro-v1") return;
        editingIndex=index;
        document.getElementById("fo-group").value=data.grupo;
        renderForm();
        document.getElementById("fo-category").value=data.categoria;
        document.getElementById("fo-planned").value=data.planejado ?? 0;
        document.getElementById("fo-actual").value=data.realizado ?? 0;
        document.getElementById("fo-description").value=data.descricao||"";
        document.getElementById("fo-form-title").textContent="Editar movimentação";
        document.getElementById("fo-save").textContent="Salvar alterações";
        document.getElementById("fo-cancel").classList.remove("hidden");
        document.getElementById("fo-entry-form").scrollIntoView({behavior:"smooth",block:"center"});
    }

    async function deleteEntry(index) {
        if(!confirm("Excluir esta movimentação do planejamento?")) return;
        try {
            const response=await fetch("/api/vault/delete",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({senha:document.getElementById("master-password").value,indice:index})});
            const result=await response.json(); if(!result.success) throw new Error(result.error||"Não foi possível excluir.");
            await window.carregarOrganizadorFinanceiro();
        } catch(error) { alert("Erro: "+error.message); }
    }

    window.carregarOrganizadorFinanceiro = async function() {
        content.innerHTML='<div class="fo-empty">Carregando seu planejamento...</div>';
        try {
            const response=await fetch("/api/vault/list",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({senha:document.getElementById("master-password").value})});
            const result=await response.json(); if(!result.success) throw new Error(result.error||"Não foi possível carregar os dados.");
            vaultRecords=result.registros||[]; organizerLoaded=true; render();
        } catch(error) { content.innerHTML=`<div class="fo-empty">Não foi possível carregar o organizador: ${escapeHtml(error.message)}</div>`; }
    };

    let organizerLoaded = false;
    monthInput.addEventListener("change",()=>{ if(organizerLoaded) render(); });
    document.querySelectorAll("[data-fo-tab]").forEach(button=>button.addEventListener("click",()=>{
        activeTab=button.dataset.foTab;
        document.querySelectorAll("[data-fo-tab]").forEach(b=>b.classList.toggle("active",b===button));
        if(vaultRecords.length) render();
    }));
})();