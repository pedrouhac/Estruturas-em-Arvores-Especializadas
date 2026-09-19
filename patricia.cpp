#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <fstream>
#include <chrono>

using namespace std;
using namespace std::chrono;

// ==========================================================
// UTILITÁRIO DE BENCHMARK E CSV
// ==========================================================
template<typename Func>
long long cronometrar(Func operacao) {
    auto inicio = high_resolution_clock::now();
    operacao();
    auto fim = high_resolution_clock::now();
    return duration_cast<microseconds>(fim - inicio).count();
}

void registrarResultadoCSV(const string& nomeArquivo, const string& estrutura,
                            const string& distribuicao, size_t n,
                            long long tempoInsercaoUs, long long tempoBuscaExistenteUs,
                            long long tempoBuscaInexistenteUs, long long tempoRemocaoUs,
                            long long metricaEstrutural) {
    ifstream teste(nomeArquivo);
    bool arquivoJaExiste = teste.good();
    teste.close();

    ofstream csv(nomeArquivo, ios::app);
    if (!csv.is_open()) return;

    if (!arquivoJaExiste) {
        csv << "estrutura,distribuicao,n,tempo_insercao_us,tempo_busca_existente_us,"
               "tempo_busca_inexistente_us,tempo_remocao_us,metrica_estrutural\n";
    }
    csv << estrutura << "," << distribuicao << "," << n << ","
        << tempoInsercaoUs << "," << tempoBuscaExistenteUs << ","
        << tempoBuscaInexistenteUs << "," << tempoRemocaoUs << ","
        << metricaEstrutural << "\n";
    csv.close();
}

vector<string> gerarChavesInexistentes(const vector<string>& base) {
    vector<string> inexistentes;
    inexistentes.reserve(base.size());
    for (const string& palavra : base) {
        inexistentes.push_back(palavra + "zzq"); 
    }
    return inexistentes;
}

struct PatriciaNode {
    string rotulo;              // fragmento de chave armazenado neste nó
    bool fimDePalavra;
    vector<PatriciaNode*> filhos;

    PatriciaNode(const string& r, bool fim) : rotulo(r), fimDePalavra(fim) {}
};

PatriciaNode* criarNo(const string& rotulo, bool fim) {
    return new PatriciaNode(rotulo, fim);
}

// ==========================================================
// NORMALIZAÇÃO 
// ==========================================================
pair<string, bool> normalizar(const string& palavra) {
    string normalizada = "";
    for (char c : palavra) {
        char letraMin = tolower(static_cast<unsigned char>(c));
        if (letraMin < 'a' || letraMin > 'z') {
            return {palavra, false};
        }
        normalizada += letraMin;
    }
    return {normalizada, true};
}

// Tamanho do maior prefixo comum entre duas strings (mecânica de "Mismatch")
int prefixoComum(const string& a, const string& b) {
    int i = 0;
    int limite = min(a.size(), b.size());
    while (i < limite && a[i] == b[i]) i++;
    return i;
}

// ==========================================================
// INSERÇÃO -- O(k)
// ==========================================================
void inserirRec(PatriciaNode* no, const string& chave) {
    for (PatriciaNode*& filho : no->filhos) {
        int lcp = prefixoComum(filho->rotulo, chave);
        if (lcp == 0) continue; // este filho não compartilha nenhuma letra com a chave

        if (lcp == (int)filho->rotulo.size()) {
            // Rótulo do filho foi totalmente consumido: ou terminamos
            // aqui, ou continuamos descendo por esse filho.
            string resto = chave.substr(lcp);
            if (resto.empty()) {
                filho->fimDePalavra = true;
            } else {
                inserirRec(filho, resto);
            }
            return;
        }

        // Ponto de divergência (mismatch) no MEIO do rótulo do filho:
        // é necessário QUEBRAR o nó (split).
        string comum = filho->rotulo.substr(0, lcp);
        string restoAntigo = filho->rotulo.substr(lcp);
        string restoNovo = chave.substr(lcp);

        PatriciaNode* intermediario = criarNo(comum, restoNovo.empty());
        filho->rotulo = restoAntigo;              // filho antigo encolhe para o sufixo
        intermediario->filhos.push_back(filho);    // e vira filho do novo nó intermediário

        if (!restoNovo.empty()) {
            // a nova palavra diverge no meio -> vira um segundo filho
            intermediario->filhos.push_back(criarNo(restoNovo, true));
        }

        filho = intermediario; // substitui o ponteiro na lista de filhos do pai
        return;
    }

    // Nenhum filho existente compartilha prefixo com a chave: novo ramo folha.
    no->filhos.push_back(criarNo(chave, true));
}

void inserir(PatriciaNode* raiz, const string& palavraNormalizada) {
    if (palavraNormalizada.empty()) return;
    inserirRec(raiz, palavraNormalizada);
}

// ==========================================================
// BUSCA -- O(k)
// ==========================================================
bool buscarRec(PatriciaNode* no, const string& chave) {
    for (PatriciaNode* filho : no->filhos) {
        const string& rot = filho->rotulo;
        if (chave.size() >= rot.size() && chave.compare(0, rot.size(), rot) == 0) {
            string resto = chave.substr(rot.size());
            if (resto.empty()) return filho->fimDePalavra;
            return buscarRec(filho, resto);
        }
    }
    return false;
}

bool buscar(PatriciaNode* raiz, const string& palavraNormalizada) {
    if (palavraNormalizada.empty()) return false;
    return buscarRec(raiz, palavraNormalizada);
}

// ==========================================================
// REMOÇÃO -- O(k)
// ==========================================================
void removerRec(PatriciaNode* no, const string& chave) {
    for (size_t i = 0; i < no->filhos.size(); i++) {
        PatriciaNode* filho = no->filhos[i];
        const string& rot = filho->rotulo;
        if (chave.size() < rot.size() || chave.compare(0, rot.size(), rot) != 0) continue;

        string resto = chave.substr(rot.size());
        if (resto.empty()) {
            filho->fimDePalavra = false;
        } else {
            removerRec(filho, resto);
        }

        // Pós-processamento (bottom-up): o nó "filho" precisa ser
        // removido (se virou folha morta) ou mesclado com seu único
        // filho remanescente (para manter a compressão da Patricia).
        if (filho->filhos.empty() && !filho->fimDePalavra) {
            delete filho;
            no->filhos.erase(no->filhos.begin() + i);
        } else if (filho->filhos.size() == 1 && !filho->fimDePalavra) {
            PatriciaNode* neto = filho->filhos[0];
            neto->rotulo = filho->rotulo + neto->rotulo;
            no->filhos[i] = neto;
            delete filho;
        }
        return;
    }
    // chave não encontrada: nenhuma alteração (operação segura/no-op)
}

void remover(PatriciaNode* raiz, const string& palavraNormalizada) {
    if (palavraNormalizada.empty()) return;
    removerRec(raiz, palavraNormalizada);
}

void destruirPatricia(PatriciaNode* no) {
    if (!no) return;
    for (PatriciaNode* filho : no->filhos) destruirPatricia(filho);
    delete no;
}

// ==========================================================
// CONTAGEM DE NÓS
// ==========================================================
int contarNos(PatriciaNode* no) {
    if (!no) return 0;
    int total = 1;
    for (PatriciaNode* filho : no->filhos) total += contarNos(filho);
    return total;
}

// ==========================================================
// EXPORTAÇÃO VISUAL (Graphviz / DOT)
// ==========================================================
void gerarDotRecursivo(PatriciaNode* no, int meuId, int& proximoId, ofstream& arquivo) {
    for (PatriciaNode* filho : no->filhos) {
        int idFilho = ++proximoId;
        string estilo = filho->fimDePalavra ? ", style=\"rounded,filled\", fillcolor=lightgrey" : "";
        arquivo << "    node" << idFilho << " [label=\"" << filho->rotulo << "\"" << estilo << "];\n";
        arquivo << "    node" << meuId << " -> node" << idFilho << ";\n";
        gerarDotRecursivo(filho, idFilho, proximoId, arquivo);
    }
}

void exportarGraphviz(PatriciaNode* raiz, const string& nomeArquivo) {
    ofstream arquivo(nomeArquivo);
    if (!arquivo.is_open()) return;
    arquivo << "digraph Patricia {\n"
            << "    graph [ranksep=0.6, nodesep=0.4];\n"
            << "    node [shape=box, style=rounded, fontsize=18, margin=\"0.15,0.08\"];\n"
            << "    edge [arrowsize=0.7];\n"
            << "    node0 [label=\"\"];\n";
    int id = 0;
    gerarDotRecursivo(raiz, id, id, arquivo);
    arquivo << "}\n";
    arquivo.close();
}

// ==========================================================
// LEITURA DO DICIONÁRIO COMPARTILHADO (idêntica à da Trie)
// ==========================================================
vector<string> lerDicionario(const string& caminho) {
    vector<string> palavras;
    ifstream arquivo(caminho);
    if (!arquivo.is_open()) {
        cerr << "[ERRO] Nao foi possivel abrir o arquivo: " << caminho << endl;
        return palavras;
    }
    string linha;
    while (getline(arquivo, linha)) {
        if (linha.empty()) continue;
        pair<string, bool> resultado = normalizar(linha);
        if (!resultado.second) {
            cerr << "[AVISO] Palavra ignorada (caractere invalido): \"" << linha << "\"" << endl;
            continue;
        }
        palavras.push_back(resultado.first);
    }
    return palavras;
}

// ==========================================================
// PROGRAMA PRINCIPAL
// ==========================================================
int main(int argc, char* argv[]) {
    string caminhoDicionario = (argc > 1) ? argv[1] : "dicionario.txt";

    PatriciaNode* raiz = criarNo("", false); // raiz é um nó "vazio" que só agrupa os ramos

    // --- Estado inicial ---
    exportarGraphviz(raiz, "patricia_estado_inicial.dot");

    vector<string> palavras = lerDicionario(caminhoDicionario);
    cout << "--- Lidas " << palavras.size() << " palavras de \"" << caminhoDicionario << "\" ---" << endl;

    auto inicio = high_resolution_clock::now();
    for (const string& palavra : palavras) {
        inserir(raiz, palavra);
    }
    auto fim = high_resolution_clock::now();
    auto duracao = duration_cast<microseconds>(fim - inicio);

    cout << "Insercao de " << palavras.size() << " palavras levou " << duracao.count() << " microsegundos." << endl;
    cout << "Numero de nos instanciados na Patricia: " << contarNos(raiz) << endl;

    // --- Estado após inserções, evidenciando bifurcações/splits ---
    exportarGraphviz(raiz, "patricia_apos_insercoes.dot");

    cout << "\n--- Teste de Busca ---" << endl;
    cout << "Buscar 'carro': " << (buscar(raiz, "carro") ? "Encontrado" : "Nao encontrado") << endl;
    cout << "Buscar 'car': " << (buscar(raiz, "car") ? "Encontrado" : "Nao encontrado (eh apenas prefixo)") << endl;

    cout << "\n--- Teste de Remocao (evidenciando mesclagem) ---" << endl;
    cout << "Removendo 'carro'..." << endl;
    remover(raiz, "carro");
    cout << "Buscar 'carro' apos remocao: " << (buscar(raiz, "carro") ? "Encontrado" : "Nao encontrado") << endl;
    cout << "Buscar 'carreta' apos remocao de 'carro': " << (buscar(raiz, "carreta") ? "Encontrado" : "Nao encontrado") << endl;

    // --- Estado após remoção/reorganização ---
    exportarGraphviz(raiz, "patricia_apos_remocao.dot");

    destruirPatricia(raiz);

    // ==========================================
    // BENCHMARK
    // ==========================================
    string distribuicao = (argc > 2) ? argv[2] : "padrao";

    PatriciaNode* raizBench = criarNo("", false);

    long long tempoInsercaoBench = cronometrar([&]() {
        for (const string& palavra : palavras) inserir(raizBench, palavra);
    });

    long long numNos = contarNos(raizBench);

    bool encontrouTudo = true;
    long long tempoBuscaExistente = cronometrar([&]() {
        for (const string& palavra : palavras) {
            if (!buscar(raizBench, palavra)) encontrouTudo = false;
        }
    });

    vector<string> chavesInexistentes = gerarChavesInexistentes(palavras);
    long long tempoBuscaInexistente = cronometrar([&]() {
        for (const string& chave : chavesInexistentes) buscar(raizBench, chave);
    });

    long long tempoRemocaoBench = cronometrar([&]() {
        for (const string& palavra : palavras) remover(raizBench, palavra);
    });

    cout << "\n--- Benchmark ---" << endl;
    cout << "Insercao: " << tempoInsercaoBench << " us | Busca(existente): " << tempoBuscaExistente
         << " us | Busca(inexistente): " << tempoBuscaInexistente
         << " us | Remocao: " << tempoRemocaoBench << " us" << endl;

    registrarResultadoCSV("resultados.csv", "Patricia", distribuicao, palavras.size(),
                           tempoInsercaoBench, tempoBuscaExistente, tempoBuscaInexistente,
                           tempoRemocaoBench, numNos);

    destruirPatricia(raizBench);

    return 0;
}