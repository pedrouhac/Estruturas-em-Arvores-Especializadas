#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <chrono>
#include <random>
#include <cmath>

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

vector<int> gerarChavesInexistentes(const vector<int>& base) {
    vector<int> inexistentes;
    inexistentes.reserve(base.size());
    for (int chave : base) {
        inexistentes.push_back(chave + 9999999); 
    }
    return inexistentes;
}



struct TreapNode {
    int chave;
    int prioridade;
    TreapNode* esquerda;
    TreapNode* direita;
};

TreapNode* criarNo(int chave, int prioridade) {
    TreapNode* no = new TreapNode;
    no->chave = chave;
    no->prioridade = prioridade;
    no->esquerda = no->direita = nullptr;
    return no;
}

// Gerador de prioridades aleatórias. Usamos uma semente FIXA (42) para
// que a demonstração seja reprodutível (mesmas imagens toda vez que
// rodar).
mt19937 geradorAleatorio(42);
uniform_int_distribution<int> distribuicaoPrioridade(1, 1000);
int gerarPrioridade() { return distribuicaoPrioridade(geradorAleatorio); }

// ==========================================================
// ROTAÇÕES BÁSICAS (mesma mecânica da AVL/Splay, mas aqui disparadas
// pela comparação de PRIORIDADE, não por fator de balanceamento nem
// por acesso)
// ==========================================================
TreapNode* rotacionarDireita(TreapNode* y) {
    TreapNode* x = y->esquerda;
    y->esquerda = x->direita;
    x->direita = y;
    return x;
}

TreapNode* rotacionarEsquerda(TreapNode* x) {
    TreapNode* y = x->direita;
    x->direita = y->esquerda;
    y->esquerda = x;
    return y;
}

// ==========================================================
// INSERÇÃO -- O(log n) esperado.
// ==========================================================
TreapNode* inserirComPrioridade(TreapNode* raiz, int chave, int prioridade) {
    if (!raiz) return criarNo(chave, prioridade);

    if (chave < raiz->chave) {
        raiz->esquerda = inserirComPrioridade(raiz->esquerda, chave, prioridade);
        if (raiz->esquerda->prioridade > raiz->prioridade) {
            raiz = rotacionarDireita(raiz);
        }
    } else if (chave > raiz->chave) {
        raiz->direita = inserirComPrioridade(raiz->direita, chave, prioridade);
        if (raiz->direita->prioridade > raiz->prioridade) {
            raiz = rotacionarEsquerda(raiz);
        }
    }
    // chave == raiz->chave: já existe, não duplica.
    return raiz;
}

TreapNode* inserir(TreapNode* raiz, int chave) {
    return inserirComPrioridade(raiz, chave, gerarPrioridade());
}

// ==========================================================
// BUSCA -- O(log n) esperado. Igual a uma BST comum: a prioridade não
// participa da busca, só da manutenção da forma da árvore.
// ==========================================================
bool buscar(TreapNode* raiz, int chave) {
    if (!raiz) return false;
    if (chave == raiz->chave) return true;
    return (chave < raiz->chave) ? buscar(raiz->esquerda, chave) : buscar(raiz->direita, chave);
}

// ==========================================================
// REMOÇÃO -- O(log n) esperado.
// ==========================================================
TreapNode* remover(TreapNode* raiz, int chave) {
    if (!raiz) return nullptr;

    if (chave < raiz->chave) {
        raiz->esquerda = remover(raiz->esquerda, chave);
    } else if (chave > raiz->chave) {
        raiz->direita = remover(raiz->direita, chave);
    } else {
        // Encontrado.
        if (!raiz->esquerda) {
            TreapNode* temp = raiz->direita;
            delete raiz;
            return temp;
        }
        if (!raiz->direita) {
            TreapNode* temp = raiz->esquerda;
            delete raiz;
            return temp;
        }
        // Dois filhos: gira na direção do filho de maior prioridade
        // para empurrar o nó-alvo para baixo, mantendo a propriedade
        // de heap, e continua removendo recursivamente.
        if (raiz->esquerda->prioridade > raiz->direita->prioridade) {
            raiz = rotacionarDireita(raiz);
            raiz->direita = remover(raiz->direita, chave);
        } else {
            raiz = rotacionarEsquerda(raiz);
            raiz->esquerda = remover(raiz->esquerda, chave);
        }
    }
    return raiz;
}

// ==========================================================
// OPERAÇÃO ESPECÍFICA: ATUALIZAÇÃO DE PRIORIDADE
// ==========================================================
TreapNode* atualizarPrioridade(TreapNode* raiz, int chave, int novaPrioridade) {
    if (!buscar(raiz, chave)) return raiz; // chave não existe, nada a fazer
    raiz = remover(raiz, chave);
    raiz = inserirComPrioridade(raiz, chave, novaPrioridade);
    return raiz;
}

void destruirTreap(TreapNode* no) {
    if (!no) return;
    destruirTreap(no->esquerda);
    destruirTreap(no->direita);
    delete no;
}

int altura(TreapNode* no) {
    if (!no) return 0;
    return 1 + max(altura(no->esquerda), altura(no->direita));
}

// ==========================================================
// EXPORTAÇÃO VISUAL (Graphviz / DOT)
// ==========================================================
void gerarDotRecursivo(TreapNode* no, int meuId, int& proximoId, ofstream& arquivo) {
    if (no->esquerda) {
        int idFilho = ++proximoId;
        arquivo << "    node" << idFilho << " [label=\"" << no->esquerda->chave
                << "\\n(p=" << no->esquerda->prioridade << ")\"];\n";
        arquivo << "    node" << meuId << " -> node" << idFilho << " [label=\"E\"];\n";
        gerarDotRecursivo(no->esquerda, idFilho, proximoId, arquivo);
    }
    if (no->direita) {
        int idFilho = ++proximoId;
        arquivo << "    node" << idFilho << " [label=\"" << no->direita->chave
                << "\\n(p=" << no->direita->prioridade << ")\"];\n";
        arquivo << "    node" << meuId << " -> node" << idFilho << " [label=\"D\"];\n";
        gerarDotRecursivo(no->direita, idFilho, proximoId, arquivo);
    }
}

void exportarGraphviz(TreapNode* raiz, const string& nomeArquivo) {
    ofstream arquivo(nomeArquivo);
    if (!arquivo.is_open()) return;
    arquivo << "digraph Treap {\n    node [shape=circle];\n";
    if (raiz) {
        arquivo << "    node0 [label=\"" << raiz->chave << "\\n(p=" << raiz->prioridade
                << ")\", style=filled, fillcolor=lightgrey];\n";
        int id = 0;
        gerarDotRecursivo(raiz, 0, id, arquivo);
    }
    arquivo << "}\n";
    arquivo.close();
}

// ==========================================================
// LEITURA DO DATASET COMPARTILHADO
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

    TreapNode* raiz = nullptr;

    // --- Estado inicial ---
    exportarGraphviz(raiz, "treap_estado_inicial.dot");

    vector<int> chaves = lerDataset(caminhoDataset);
    cout << "--- Lidas " << chaves.size() << " chaves de \"" << caminhoDataset << "\" ---" << endl;

    if (chaves.empty()) {
        cout << "Dataset vazio. Encerrando." << endl;
        return 0;
    }

    auto inicio = high_resolution_clock::now();
    for (int chave : chaves) {
        raiz = inserir(raiz, chave);
    }
    auto fim = high_resolution_clock::now();
    auto duracao = duration_cast<microseconds>(fim - inicio);

    cout << "Insercao de " << chaves.size() << " chaves levou " << duracao.count() << " microsegundos." << endl;
    cout << "Altura da arvore apos insercoes: " << altura(raiz)
         << " (para " << chaves.size() << " chaves, log2(n) ~= "
         << (chaves.empty() ? 0.0 : log2((double)chaves.size())) << ")" << endl;
    cout << "Raiz apos insercoes (chave com a maior prioridade sorteada): "
         << raiz->chave << " (p=" << raiz->prioridade << ")" << endl;

    // --- Estado após inserções ---
    exportarGraphviz(raiz, "treap_apos_insercoes.dot");

    int alvoBusca = chaves[0];
    int chaveInexistente = alvoBusca + 9999999;

    cout << "\n--- Teste de Busca ---" << endl;
    cout << "Buscar " << alvoBusca << ": " << (buscar(raiz, alvoBusca) ? "Encontrado" : "Nao encontrado") << endl;
    cout << "Buscar " << chaveInexistente << ": " << (buscar(raiz, chaveInexistente) ? "Encontrado" : "Nao encontrado") << endl;

    // --- Estado intermediário: alteração de prioridade forçando rotações ---
    cout << "\n--- Teste de Atualizacao de Prioridade (operacao especifica) ---" << endl;
    cout << "Atribuindo prioridade maxima (9999) a chave " << alvoBusca << ", forcando-a a subir..." << endl;
    raiz = atualizarPrioridade(raiz, alvoBusca, 9999);
    cout << "Raiz apos atualizar prioridade de " << alvoBusca << ": " << raiz->chave << " (p=" << raiz->prioridade << ")" << endl;
    exportarGraphviz(raiz, "treap_apos_prioridade.dot");

    // --- Estado após remoção ---
    cout << "\n--- Teste de Remocao ---" << endl;
    cout << "Removendo " << alvoBusca << "..." << endl;
    raiz = remover(raiz, alvoBusca);
    cout << "Buscar " << alvoBusca << " apos remocao: " << (buscar(raiz, alvoBusca) ? "Encontrado" : "Nao encontrado") << endl;
    exportarGraphviz(raiz, "treap_apos_remocao.dot");

    destruirTreap(raiz);

    // ==========================================
    // BENCHMARK
    // ==========================================
    string distribuicao = (argc > 2) ? argv[2] : "padrao";

    TreapNode* raizBench = nullptr;

    long long tempoInsercaoBench = cronometrar([&]() {
        for (int chave : chaves) raizBench = inserir(raizBench, chave);
    });

    long long alturaFinal = altura(raizBench);

    bool encontrouTudo = true;
    long long tempoBuscaExistente = cronometrar([&]() {
        for (int chave : chaves) {
            if (!buscar(raizBench, chave)) encontrouTudo = false;
        }
    });

    vector<int> chavesInexistentes = gerarChavesInexistentes(chaves);
    bool encontrouAlgumaInexistente = false; // deve continuar "false" ao final (nenhuma deveria existir)
    long long tempoBuscaInexistente = cronometrar([&]() {
        for (int chave : chavesInexistentes) {
            if (buscar(raizBench, chave)) encontrouAlgumaInexistente = true;
        }
    });

    long long tempoRemocaoBench = cronometrar([&]() {
        for (int chave : chaves) raizBench = remover(raizBench, chave);
    });

    cout << "\n--- Benchmark ---" << endl;
    cout << "Insercao: " << tempoInsercaoBench << " us | Busca(existente): " << tempoBuscaExistente
         << " us (todas encontradas: " << (encontrouTudo ? "sim" : "nao") << ")"
         << " | Busca(inexistente): " << tempoBuscaInexistente
         << " us (falso positivo: " << (encontrouAlgumaInexistente ? "sim" : "nao") << ")"
         << " us | Remocao: " << tempoRemocaoBench << " us" << endl;

    registrarResultadoCSV("resultados_numericos.csv", "Treap", distribuicao, chaves.size(),
                           tempoInsercaoBench, tempoBuscaExistente, tempoBuscaInexistente,
                           tempoRemocaoBench, alturaFinal);

    destruirTreap(raizBench);

    return 0;
}