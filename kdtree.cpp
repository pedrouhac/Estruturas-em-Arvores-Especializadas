#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <chrono>
#include <cmath>
#include <limits>

using namespace std;
using namespace std::chrono;

// ==========================================================
// KD-TREE (k-dimensional Tree), especializada para k=2 (pontos (x, y))
//
// Ideia central: em vez de comparar por uma única chave (como
// BST/AVL/Splay/Treap), a KD-Tree PARTICIONA O ESPAÇO. A cada nível da
// árvore, a dimensão usada para comparar alterna (aqui: nível par
// compara por x, nível ímpar compara por y). Cada nó representa um
// HIPERPLANO (aqui, uma reta) que corta o espaço em duas metades.
// ==========================================================

struct Ponto {
    double x, y;
};

struct KDNode {
    Ponto ponto;
    KDNode* esquerda;
    KDNode* direita;
};

KDNode* criarNo(Ponto p) {
    KDNode* no = new KDNode;
    no->ponto = p;
    no->esquerda = no->direita = nullptr;
    return no;
}

bool pontosIguais(const Ponto& a, const Ponto& b) {
    return a.x == b.x && a.y == b.y;
}

// Retorna a coordenada de p correspondente ao eixo usado naquele nível
// (0 = x, 1 = y).
double coordenada(const Ponto& p, int eixo) {
    return (eixo == 0) ? p.x : p.y;
}

// ==========================================================
// INSERÇÃO -- O(log n) médio (O(n) no pior caso, se a árvore
// degenerar por entradas já ordenadas/adversariais).
// A cada nível, decide ir para esquerda/direita comparando a
// coordenada do eixo correspondente à profundidade atual.
// ==========================================================
KDNode* inserir(KDNode* raiz, Ponto p, int profundidade = 0) {
    if (!raiz) return criarNo(p);

    int eixo = profundidade % 2;
    if (coordenada(p, eixo) < coordenada(raiz->ponto, eixo)) {
        raiz->esquerda = inserir(raiz->esquerda, p, profundidade + 1);
    } else {
        raiz->direita = inserir(raiz->direita, p, profundidade + 1);
    }
    return raiz;
}

// ==========================================================
// BUSCA EXATA -- O(log n) médio.
// ==========================================================
bool buscar(KDNode* raiz, Ponto p, int profundidade = 0) {
    if (!raiz) return false;
    if (pontosIguais(raiz->ponto, p)) return true;

    int eixo = profundidade % 2;
    if (coordenada(p, eixo) < coordenada(raiz->ponto, eixo)) {
        return buscar(raiz->esquerda, p, profundidade + 1);
    } else {
        return buscar(raiz->direita, p, profundidade + 1);
    }
}

// ==========================================================
// REMOÇÃO -- algoritmo clássico de Bentley (1975).
// Não é possível simplesmente "pendurar" as subárvores como numa BST
// comum, porque cada nível respeita um eixo de comparação diferente.
// A técnica é: substituir o nó removido pelo elemento de MENOR valor,
// NO MESMO EIXO do nó removido, encontrado na subárvore DIREITA (ou,
// na ausência dela, na subárvore esquerda -- caso em que ela também
// precisa ser realocada para o lado direito).
// ==========================================================
KDNode* encontrarMinimo(KDNode* raiz, int eixoAlvo, int profundidade) {
    if (!raiz) return nullptr;
    int eixoAtual = profundidade % 2;

    if (eixoAtual == eixoAlvo) {
        // Neste nível, apenas a subárvore esquerda pode conter um
        // valor menor no eixo alvo (propriedade da KD-Tree).
        if (!raiz->esquerda) return raiz;
        return encontrarMinimo(raiz->esquerda, eixoAlvo, profundidade + 1);
    }

    // Eixo diferente: o mínimo pode estar em qualquer subárvore, então
    // é preciso comparar as três possibilidades.
    KDNode* menor = raiz;
    KDNode* candEsquerda = encontrarMinimo(raiz->esquerda, eixoAlvo, profundidade + 1);
    KDNode* candDireita = encontrarMinimo(raiz->direita, eixoAlvo, profundidade + 1);
    if (candEsquerda && coordenada(candEsquerda->ponto, eixoAlvo) < coordenada(menor->ponto, eixoAlvo)) {
        menor = candEsquerda;
    }
    if (candDireita && coordenada(candDireita->ponto, eixoAlvo) < coordenada(menor->ponto, eixoAlvo)) {
        menor = candDireita;
    }
    return menor;
}

KDNode* remover(KDNode* raiz, Ponto p, int profundidade = 0) {
    if (!raiz) return nullptr;

    int eixo = profundidade % 2;

    if (pontosIguais(raiz->ponto, p)) {
        if (raiz->direita) {
            KDNode* minimo = encontrarMinimo(raiz->direita, eixo, profundidade + 1);
            raiz->ponto = minimo->ponto;
            raiz->direita = remover(raiz->direita, minimo->ponto, profundidade + 1);
        } else if (raiz->esquerda) {
            // Truque clássico do algoritmo de Bentley: como não há
            // subárvore direita, a esquerda inteira precisa "virar" a
            // nova subárvore direita (a KD-Tree não usa subárvore
            // esquerda para armazenar valores >= no eixo de divisão).
            KDNode* minimo = encontrarMinimo(raiz->esquerda, eixo, profundidade + 1);
            raiz->ponto = minimo->ponto;
            raiz->direita = remover(raiz->esquerda, minimo->ponto, profundidade + 1);
            raiz->esquerda = nullptr;
        } else {
            delete raiz;
            return nullptr;
        }
        return raiz;
    }

    if (coordenada(p, eixo) < coordenada(raiz->ponto, eixo)) {
        raiz->esquerda = remover(raiz->esquerda, p, profundidade + 1);
    } else {
        raiz->direita = remover(raiz->direita, p, profundidade + 1);
    }
    return raiz;
}

// ==========================================================
// OPERAÇÃO ESPECÍFICA: VIZINHO MAIS PRÓXIMO (Nearest Neighbor)
// A operação que justifica a existência da KD-Tree: encontrar, em
// O(log n) médio, o ponto mais próximo de um ponto de consulta -- sem
// precisar comparar com todos os n pontos (o que seria O(n)).
// ==========================================================
double distanciaQuadrada(const Ponto& a, const Ponto& b) {
    double dx = a.x - b.x;
    double dy = a.y - b.y;
    return dx * dx + dy * dy;
}

void vizinhoMaisProximoRec(KDNode* raiz, const Ponto& alvo, int profundidade,
                            KDNode*& melhor, double& melhorDist) {
    if (!raiz) return;

    double d = distanciaQuadrada(raiz->ponto, alvo);
    if (!melhor || d < melhorDist) {
        melhor = raiz;
        melhorDist = d;
    }

    int eixo = profundidade % 2;
    double diferenca = coordenada(alvo, eixo) - coordenada(raiz->ponto, eixo);

    KDNode* primeiroLado = (diferenca < 0) ? raiz->esquerda : raiz->direita;
    KDNode* segundoLado = (diferenca < 0) ? raiz->direita : raiz->esquerda;

    vizinhoMaisProximoRec(primeiroLado, alvo, profundidade + 1, melhor, melhorDist);

    // Só vale a pena explorar o outro lado do hiperplano se a
    // distância até o próprio hiperplano for menor que a melhor
    // distância já encontrada -- essa poda é o que torna a busca
    // O(log n) em vez de O(n).
    if (diferenca * diferenca < melhorDist) {
        vizinhoMaisProximoRec(segundoLado, alvo, profundidade + 1, melhor, melhorDist);
    }
}

Ponto vizinhoMaisProximo(KDNode* raiz, const Ponto& alvo) {
    KDNode* melhor = nullptr;
    double melhorDist = numeric_limits<double>::max();
    vizinhoMaisProximoRec(raiz, alvo, 0, melhor, melhorDist);
    return melhor ? melhor->ponto : Ponto{0, 0};
}

void destruirKDTree(KDNode* raiz) {
    if (!raiz) return;
    destruirKDTree(raiz->esquerda);
    destruirKDTree(raiz->direita);
    delete raiz;
}

int altura(KDNode* raiz) {
    if (!raiz) return 0;
    return 1 + max(altura(raiz->esquerda), altura(raiz->direita));
}

// ==========================================================
// EXPORTAÇÃO VISUAL 1: estrutura da árvore (Graphviz / DOT)
// ==========================================================
void gerarDotRecursivo(KDNode* no, int meuId, int& proximoId, ofstream& arquivo) {
    if (no->esquerda) {
        int idFilho = ++proximoId;
        arquivo << "    node" << idFilho << " [label=\"(" << no->esquerda->ponto.x
                << "," << no->esquerda->ponto.y << ")\"];\n";
        arquivo << "    node" << meuId << " -> node" << idFilho << " [label=\"E\"];\n";
        gerarDotRecursivo(no->esquerda, idFilho, proximoId, arquivo);
    }
    if (no->direita) {
        int idFilho = ++proximoId;
        arquivo << "    node" << idFilho << " [label=\"(" << no->direita->ponto.x
                << "," << no->direita->ponto.y << ")\"];\n";
        arquivo << "    node" << meuId << " -> node" << idFilho << " [label=\"D\"];\n";
        gerarDotRecursivo(no->direita, idFilho, proximoId, arquivo);
    }
}

void exportarGraphviz(KDNode* raiz, const string& nomeArquivo) {
    ofstream arquivo(nomeArquivo);
    if (!arquivo.is_open()) return;
    arquivo << "digraph KDTree {\n    node [shape=box];\n";
    if (raiz) {
        arquivo << "    node0 [label=\"(" << raiz->ponto.x << "," << raiz->ponto.y
                << ")\", style=filled, fillcolor=lightgrey];\n";
        int id = 0;
        gerarDotRecursivo(raiz, 0, id, arquivo);
    }
    arquivo << "}\n";
    arquivo.close();
}

// ==========================================================
// EXPORTAÇÃO VISUAL 2: PARTICIONAMENTO ESPACIAL (SVG)
// Esta é a representação mais importante para a KD-Tree, pois mostra
// literalmente como o espaço 2D vai sendo cortado por retas verticais
// (divisão por x) e horizontais (divisão por y), com a região de cada
// corte limitada pela região herdada do nó pai.
// ==========================================================
void gerarSVGRecursivo(KDNode* no, double xMin, double xMax, double yMin, double yMax,
                       int profundidade, ofstream& arquivo) {
    if (!no) return;
    int eixo = profundidade % 2;

    if (eixo == 0) {
        // Divisão vertical em x = no->ponto.x
        arquivo << "<line x1=\"" << no->ponto.x << "\" y1=\"" << yMin
                << "\" x2=\"" << no->ponto.x << "\" y2=\"" << yMax
                << "\" stroke=\"blue\" stroke-width=\"0.6\"/>\n";
        gerarSVGRecursivo(no->esquerda, xMin, no->ponto.x, yMin, yMax, profundidade + 1, arquivo);
        gerarSVGRecursivo(no->direita, no->ponto.x, xMax, yMin, yMax, profundidade + 1, arquivo);
    } else {
        // Divisão horizontal em y = no->ponto.y
        arquivo << "<line x1=\"" << xMin << "\" y1=\"" << no->ponto.y
                << "\" x2=\"" << xMax << "\" y2=\"" << no->ponto.y
                << "\" stroke=\"red\" stroke-width=\"0.6\"/>\n";
        gerarSVGRecursivo(no->esquerda, xMin, xMax, yMin, no->ponto.y, profundidade + 1, arquivo);
        gerarSVGRecursivo(no->direita, xMin, xMax, no->ponto.y, yMax, profundidade + 1, arquivo);
    }

    arquivo << "<circle cx=\"" << no->ponto.x << "\" cy=\"" << no->ponto.y
            << "\" r=\"1.5\" fill=\"black\"/>\n";
    arquivo << "<text x=\"" << (no->ponto.x + 2) << "\" y=\"" << (no->ponto.y - 2)
            << "\" font-size=\"3\">(" << no->ponto.x << "," << no->ponto.y << ")</text>\n";
}

void exportarSVG(KDNode* raiz, double xMin, double xMax, double yMin, double yMax,
                  const string& nomeArquivo) {
    ofstream arquivo(nomeArquivo);
    if (!arquivo.is_open()) return;
    arquivo << "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\""
            << xMin << " " << yMin << " " << (xMax - xMin) << " " << (yMax - yMin) << "\">\n";
    arquivo << "<rect x=\"" << xMin << "\" y=\"" << yMin << "\" width=\"" << (xMax - xMin)
            << "\" height=\"" << (yMax - yMin) << "\" fill=\"white\" stroke=\"black\" stroke-width=\"0.5\"/>\n";
    gerarSVGRecursivo(raiz, xMin, xMax, yMin, yMax, 0, arquivo);
    arquivo << "</svg>\n";
    arquivo.close();
}

// ==========================================================
// LEITURA DO DATASET DE PONTOS ("x y" por linha)
// ==========================================================
vector<Ponto> lerPontos(const string& caminho) {
    vector<Ponto> pontos;
    ifstream arquivo(caminho);
    if (!arquivo.is_open()) {
        cerr << "[ERRO] Nao foi possivel abrir o arquivo: " << caminho << endl;
        return pontos;
    }
    string linha;
    while (getline(arquivo, linha)) {
        if (linha.empty()) continue;
        istringstream iss(linha);
        double x, y;
        if (iss >> x >> y) {
            pontos.push_back({x, y});
        } else {
            cerr << "[AVISO] Linha ignorada (formato invalido, esperado \"x y\"): \"" << linha << "\"" << endl;
        }
    }
    return pontos;
}

// ==========================================================
// PROGRAMA PRINCIPAL
// ==========================================================
int main(int argc, char* argv[]) {
    string caminhoPontos = (argc > 1) ? argv[1] : "pontos.txt";
    const double LIMITE = 100.0; // domínio espacial usado nos exemplos: [0,100] x [0,100]

    KDNode* raiz = nullptr;

    // --- Estado inicial (Seção 3, item 1) ---
    exportarSVG(raiz, 0, LIMITE, 0, LIMITE, "kdtree_estado_inicial.svg");

    vector<Ponto> pontos = lerPontos(caminhoPontos);
    cout << "--- Lidos " << pontos.size() << " pontos de \"" << caminhoPontos << "\" ---" << endl;

    auto inicio = high_resolution_clock::now();
    for (const Ponto& p : pontos) {
        raiz = inserir(raiz, p);
    }
    auto fim = high_resolution_clock::now();
    auto duracao = duration_cast<microseconds>(fim - inicio);

    cout << "Insercao de " << pontos.size() << " pontos levou " << duracao.count() << " microsegundos." << endl;
    cout << "Altura da arvore apos insercoes: " << altura(raiz) << endl;

    // --- Estado após inserções, evidenciando o particionamento espacial (Seção 3, item 1/2) ---
    exportarSVG(raiz, 0, LIMITE, 0, LIMITE, "kdtree_apos_insercoes.svg");
    exportarGraphviz(raiz, "kdtree_apos_insercoes.dot");

    cout << "\n--- Teste de Busca ---" << endl;
    Ponto alvoBusca = {50, 30};
    cout << "Buscar (50,30): " << (buscar(raiz, alvoBusca) ? "Encontrado" : "Nao encontrado") << endl;
    Ponto alvoInexistente = {99, 99};
    cout << "Buscar (99,99): " << (buscar(raiz, alvoInexistente) ? "Encontrado" : "Nao encontrado") << endl;

    // --- Operação específica: vizinho mais próximo ---
    cout << "\n--- Teste de Vizinho Mais Proximo (operacao especifica) ---" << endl;
    Ponto consulta = {40, 35};
    Ponto vizinho = vizinhoMaisProximo(raiz, consulta);
    cout << "Vizinho mais proximo de (" << consulta.x << "," << consulta.y << "): ("
         << vizinho.x << "," << vizinho.y << ")" << endl;

    // --- Estado após remoção (Seção 3, item 3) ---
    cout << "\n--- Teste de Remocao ---" << endl;
    Ponto alvoRemocao = {30, 40}; // ponto raiz original
    cout << "Removendo (30,40) [ponto raiz original]..." << endl;
    raiz = remover(raiz, alvoRemocao);
    cout << "Buscar (30,40) apos remocao: " << (buscar(raiz, alvoRemocao) ? "Encontrado" : "Nao encontrado") << endl;
    exportarSVG(raiz, 0, LIMITE, 0, LIMITE, "kdtree_apos_remocao.svg");
    exportarGraphviz(raiz, "kdtree_apos_remocao.dot");

    destruirKDTree(raiz);
    return 0;
}
