#include <iostream>
#include <string>
#include <vector>
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

// Para inteiros, geramos chaves inexistentes somando um valor alto
vector<int> gerarChavesInexistentes(const vector<int>& base) {
    vector<int> inexistentes;
    inexistentes.reserve(base.size());
    for (int chave : base) {
        inexistentes.push_back(chave + 9999999); 
    }
    return inexistentes;
}

struct SplayNode {
    int chave;
    SplayNode* esquerda;
    SplayNode* direita;
    SplayNode* pai;
};

SplayNode* criarNo(int chave) {
    SplayNode* no = new SplayNode;
    no->chave = chave;
    no->esquerda = no->direita = no->pai = nullptr;
    return no;
}

// ==========================================================
// ROTAÇÕES BÁSICAS (mesmas rotações da AVL, mas aqui usadas para
// REORGANIZAR por acesso, e não para corrigir um fator de balanceamento)
// ==========================================================
void rotacionarDireita(SplayNode*& raiz, SplayNode* x) {
    SplayNode* y = x->esquerda;
    x->esquerda = y->direita;
    if (y->direita) y->direita->pai = x;
    y->pai = x->pai;
    if (!x->pai) raiz = y;
    else if (x == x->pai->direita) x->pai->direita = y;
    else x->pai->esquerda = y;
    y->direita = x;
    x->pai = y;
}

void rotacionarEsquerda(SplayNode*& raiz, SplayNode* x) {
    SplayNode* y = x->direita;
    x->direita = y->esquerda;
    if (y->esquerda) y->esquerda->pai = x;
    y->pai = x->pai;
    if (!x->pai) raiz = y;
    else if (x == x->pai->esquerda) x->pai->esquerda = y;
    else x->pai->direita = y;
    y->esquerda = x;
    x->pai = y;
}

// ==========================================================
// SPLAYING 
// ==========================================================
void splay(SplayNode*& raiz, SplayNode* x) {
    while (x->pai != nullptr) {
        SplayNode* pai = x->pai;
        SplayNode* avo = pai->pai;

        if (!avo) {
            // Caso 1: Zig (terminal -- só existem 2 níveis até a raiz)
            if (x == pai->esquerda) rotacionarDireita(raiz, pai);
            else rotacionarEsquerda(raiz, pai);
        } else if (x == pai->esquerda && pai == avo->esquerda) {
            // Caso 2: Zig-Zig (esquerda-esquerda)
            rotacionarDireita(raiz, avo);
            rotacionarDireita(raiz, pai);
        } else if (x == pai->direita && pai == avo->direita) {
            // Caso 2: Zig-Zig (direita-direita)
            rotacionarEsquerda(raiz, avo);
            rotacionarEsquerda(raiz, pai);
        } else if (x == pai->direita && pai == avo->esquerda) {
            // Caso 3: Zig-Zag (esquerda-direita, "boomerang")
            rotacionarEsquerda(raiz, pai);
            rotacionarDireita(raiz, avo);
        } else {
            // Caso 3: Zig-Zag (direita-esquerda, "boomerang")
            rotacionarDireita(raiz, pai);
            rotacionarEsquerda(raiz, avo);
        }
    }
}

// ==========================================================
// BUSCA -- O(log n) amortizado.
// ==========================================================
bool buscar(SplayNode*& raiz, int chave) {
    SplayNode* atual = raiz;
    SplayNode* ultimoVisitado = nullptr;

    while (atual) {
        ultimoVisitado = atual;
        if (chave == atual->chave) {
            splay(raiz, atual);
            return true;
        }
        atual = (chave < atual->chave) ? atual->esquerda : atual->direita;
    }

    if (ultimoVisitado) splay(raiz, ultimoVisitado);
    return false;
}

// ==========================================================
// INSERÇÃO -- O(log n) amortizado.
// ==========================================================
SplayNode* inserir(SplayNode*& raiz, int chave) {
    SplayNode* atual = raiz;
    SplayNode* pai = nullptr;

    while (atual) {
        pai = atual;
        if (chave == atual->chave) {
            splay(raiz, atual); // já existe: tratamos como um acesso
            return atual;
        }
        atual = (chave < atual->chave) ? atual->esquerda : atual->direita;
    }

    SplayNode* novo = criarNo(chave);
    novo->pai = pai;
    if (!pai) raiz = novo;
    else if (chave < pai->chave) pai->esquerda = novo;
    else pai->direita = novo;

    splay(raiz, novo);
    return novo;
}

// ==========================================================
// REMOÇÃO -- O(log n) amortizado.
// ==========================================================
void remover(SplayNode*& raiz, int chave) {
    if (!raiz) return;
    if (!buscar(raiz, chave)) return; // não encontrado (mas já fez o splay do último nó visitado)

    SplayNode* alvo = raiz; // buscar já garantiu que a chave está na raiz
    SplayNode* esquerda = alvo->esquerda;
    SplayNode* direita = alvo->direita;
    if (esquerda) esquerda->pai = nullptr;
    if (direita) direita->pai = nullptr;
    delete alvo;

    if (!esquerda) { raiz = direita; return; }
    if (!direita) { raiz = esquerda; return; }

    SplayNode* maiorEsquerda = esquerda;
    while (maiorEsquerda->direita) maiorEsquerda = maiorEsquerda->direita;
    splay(esquerda, maiorEsquerda); // maiorEsquerda vira raiz da subárvore esquerda (sem filho direito)
    maiorEsquerda->direita = direita;
    direita->pai = maiorEsquerda;
    raiz = maiorEsquerda;
}

void destruirSplay(SplayNode* no) {
    if (!no) return;
    destruirSplay(no->esquerda);
    destruirSplay(no->direita);
    delete no;
}

// ==========================================================
// PERCURSO EM ORDEM 
// ==========================================================
void emOrdem(SplayNode* no, vector<int>& saida) {
    if (!no) return;
    emOrdem(no->esquerda, saida);
    saida.push_back(no->chave);
    emOrdem(no->direita, saida);
}

int altura(SplayNode* no) {
    if (!no) return 0;
    int esq = altura(no->esquerda);
    int dir = altura(no->direita);
    return 1 + max(esq, dir);
}

// ==========================================================
// EXPORTAÇÃO VISUAL (Graphviz / DOT)
// ==========================================================
void gerarDotRecursivo(SplayNode* no, int meuId, int& proximoId, ofstream& arquivo) {
    if (no->esquerda) {
        int idFilho = ++proximoId;
        arquivo << "    node" << meuId << " -> node" << idFilho << " [label=\"E\"];\n";
        arquivo << "    node" << idFilho << " [label=\"" << no->esquerda->chave << "\"];\n";
        gerarDotRecursivo(no->esquerda, idFilho, proximoId, arquivo);
    }
    if (no->direita) {
        int idFilho = ++proximoId;
        arquivo << "    node" << meuId << " -> node" << idFilho << " [label=\"D\"];\n";
        arquivo << "    node" << idFilho << " [label=\"" << no->direita->chave << "\"];\n";
        gerarDotRecursivo(no->direita, idFilho, proximoId, arquivo);
    }
}

void exportarGraphviz(SplayNode* raiz, const string& nomeArquivo) {
    ofstream arquivo(nomeArquivo);
    if (!arquivo.is_open()) return;
    arquivo << "digraph Splay {\n    node [shape=circle];\n";
    if (raiz) {
        arquivo << "    node0 [label=\"" << raiz->chave << "\", style=filled, fillcolor=lightgrey];\n";
        int id = 0;
        gerarDotRecursivo(raiz, 0, id, arquivo);
    }
    arquivo << "}\n";
    arquivo.close();
}

// ==========================================================
// LEITURA DO DATASET DE INTEIROS
// ==========================================================
vector<int> lerDataset(const string& caminho) {
    vector<int> chaves;
    ifstream arquivo(caminho);
    if (!arquivo.is_open()) {
        cerr << "[ERRO] Nao foi possivel abrir o arquivo: " << caminho << endl;
        return chaves;
    }
    string linha;
    while (getline(arquivo, linha)) {
        if (linha.empty()) continue;
        try {
            chaves.push_back(stoi(linha));
        } catch (...) {
            cerr << "[AVISO] Linha ignorada (nao e um inteiro valido): \"" << linha << "\"" << endl;
        }
    }
    return chaves;
}

// ==========================================================
// PROGRAMA PRINCIPAL
// ==========================================================
int main(int argc, char* argv[]) {
    string caminhoDataset = (argc > 1) ? argv[1] : "dataset_numerico.txt";

    SplayNode* raiz = nullptr;

    // --- Estado inicial ---
    exportarGraphviz(raiz, "splay_estado_inicial.dot");

    vector<int> chaves = lerDataset(caminhoDataset);
    cout << "--- Lidas " << chaves.size() << " chaves de \"" << caminhoDataset << "\" ---" << endl;

    if (chaves.empty()) {
        cout << "Dataset vazio. Encerrando." << endl;
        return 0;
    }

    auto inicio = high_resolution_clock::now();
    for (int chave : chaves) {
        inserir(raiz, chave);
    }
    auto fim = high_resolution_clock::now();
    auto duracao = duration_cast<microseconds>(fim - inicio);

    cout << "Insercao de " << chaves.size() << " chaves levou " << duracao.count() << " microsegundos." << endl;
    cout << "Altura da arvore apos insercoes: " << altura(raiz) << endl;
    cout << "Raiz apos insercoes (deve ser a ULTIMA chave inserida, pela regra de splay-on-insert): "
         << raiz->chave << endl;

    // --- Estado após inserções ---
    exportarGraphviz(raiz, "splay_apos_insercoes.dot");

    // --- Demonstração do mecanismo de reorganização ---
    int alvoBusca = chaves[0]; 
    
    cout << "\n--- Teste de Busca (evidenciando reorganizacao) ---" << endl;
    cout << "Buscando " << alvoBusca << " (deve subir ate a raiz apos o acesso)..." << endl;
    buscar(raiz, alvoBusca);
    cout << "Raiz apos buscar " << alvoBusca << ": " << raiz->chave << endl;
    exportarGraphviz(raiz, "splay_apos_busca.dot");

    int chaveInexistente = alvoBusca + 9999999;
    cout << "\nBuscando " << chaveInexistente << " (chave inexistente -- deve fazer splay do ultimo no visitado)..." << endl;
    bool achou = buscar(raiz, chaveInexistente);
    cout << "Encontrado: " << (achou ? "sim" : "nao") << " | Raiz apos a tentativa: " << raiz->chave << endl;

    // --- Estado após remoção/reorganização ---
    cout << "\n--- Teste de Remocao ---" << endl;
    cout << "Removendo " << alvoBusca << "..." << endl;
    remover(raiz, alvoBusca);
    cout << "Raiz apos remover " << alvoBusca << ": " << (raiz ? to_string(raiz->chave) : "arvore vazia") << endl;
    exportarGraphviz(raiz, "splay_apos_remocao.dot");

    vector<int> ordenado;
    emOrdem(raiz, ordenado);
    cout << "\nPercurso em ordem (deve continuar ordenado, confirmando a propriedade de BST): ";
    // Imprime apenas os 10 primeiros para não poluir o terminal caso o dataset seja gigante
    for (size_t i = 0; i < min(ordenado.size(), (size_t)10); i++) {
        cout << ordenado[i] << " ";
    }
    if (ordenado.size() > 10) cout << "...";
    cout << endl;

    destruirSplay(raiz);

    // ==========================================
    // BENCHMARK
    // ==========================================
    string distribuicao = (argc > 2) ? argv[2] : "padrao";

    SplayNode* raizBench = nullptr;

    long long tempoInsercaoBench = cronometrar([&]() {
        for (int chave : chaves) inserir(raizBench, chave);
    });

    long long alturaFinal = altura(raizBench);

    bool encontrouTudo = true;
    long long tempoBuscaExistente = cronometrar([&]() {
        for (int chave : chaves) {
            if (!buscar(raizBench, chave)) encontrouTudo = false;
        }
    });

    vector<int> chavesInexistentes = gerarChavesInexistentes(chaves);
    long long tempoBuscaInexistente = cronometrar([&]() {
        for (int chave : chavesInexistentes) buscar(raizBench, chave);
    });

    long long tempoRemocaoBench = cronometrar([&]() {
        for (int chave : chaves) remover(raizBench, chave);
    });

    cout << "\n--- Benchmark ---" << endl;
    cout << "Insercao: " << tempoInsercaoBench << " us | Busca(existente): " << tempoBuscaExistente
         << " us | Busca(inexistente): " << tempoBuscaInexistente
         << " us | Remocao: " << tempoRemocaoBench << " us" << endl;

    registrarResultadoCSV("resultados_numericos.csv", "Splay", distribuicao, chaves.size(),
                           tempoInsercaoBench, tempoBuscaExistente, tempoBuscaInexistente,
                           tempoRemocaoBench, alturaFinal);

    destruirSplay(raizBench);

    return 0;
}