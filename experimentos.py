#!/usr/bin/env python3
"""
experimentos.py -- Orquestra os experimentos.

O QUE ESSE SCRIPT FAZ:
  1. Compila os 5 arquivos .cpp (trie, patricia, splay, treap, kdtree).
  2. Gera datasets de tamanhos crescentes:
       - dicionario_N.txt   (palavras aleatorias, para Trie/Patricia)
       - numeros_N.txt      (inteiros aleatorios, para Splay/Treap)
       - numeros_N_ordenado.txt (os MESMOS inteiros, mas ordenados --
         para evidenciar o pior caso de uma BST/Splay sem balanceamento
         determinístico)
       - pontos_N.txt       (pontos (x,y) aleatorios, para KD-Tree)
  3. Roda cada executavel varias vezes por tamanho (REPETICOES), cada
     vez com um dataset ALEATORIO DIFERENTE (mesma semente do gerador
     de dados muda a cada repeticao) -- isso importa especialmente
     para a Treap, que usa uma seed FIXA para as prioridades: rodar o
     MESMO dataset repetidas vezes daria sempre a mesma arvore, sem
     variancia nenhuma. Usar datasets diferentes por repeticao resolve
     isso sem precisar mexer no treap.cpp.
  4. Cada programa já grava sua propria linha de CSV (resultados.csv,
     resultados_numericos.csv, resultados_kdtree.csv), como implementado
     nos .cpp. Este script so orquestra as execucoes.
  5. Ao final, consolida os 3 CSVs num unico "resultados_consolidado.csv".

"""

import csv
import random
import string
import subprocess
import sys
from pathlib import Path

# ==========================================================
# CONFIGURACAO
# ==========================================================
TAMANHOS = [100, 1000, 10000, 100000]   # tamanhos de entrada a testar
REPETICOES = 5                  # repeticoes por tamanho (para reduzir ruido)
DIRETORIO_BASE = Path(__file__).resolve().parent

ESTRUTURAS_CPP = {
    "trie": "trie.cpp",
    "patricia": "patricia.cpp",
    "splay": "splay.cpp",
    "treap": "treap.cpp",
    "kdtree": "kdtree.cpp",
}

CSVS_GERADOS_PELOS_PROGRAMAS = [
    "resultados.csv",             # Trie + Patricia
    "resultados_numericos.csv",   # Splay + Treap
    "resultados_kdtree.csv",      # KD-Tree
]


def compilar_tudo():
    print("=== Compilando as 5 estruturas ===")
    for nome, arquivo in ESTRUTURAS_CPP.items():
        origem = DIRETORIO_BASE / arquivo
        destino = DIRETORIO_BASE / nome
        if not origem.exists():
            print(f"[ERRO] Nao encontrei {origem}")
            sys.exit(1)
        cmd = ["g++", "-std=c++17", "-O2", "-o", str(destino), str(origem)]
        resultado = subprocess.run(cmd, capture_output=True, text=True)
        if resultado.returncode != 0:
            print(f"[ERRO] Falha ao compilar {arquivo}:\n{resultado.stderr}")
            sys.exit(1)
        print(f"  OK: {nome}")
    print()


# ==========================================================
# GERACAO DE DATASETS
# ==========================================================
def gerar_palavra_aleatoria(rng, tam_min=3, tam_max=10):
    tamanho = rng.randint(tam_min, tam_max)
    return "".join(rng.choice(string.ascii_lowercase) for _ in range(tamanho))


def gerar_dicionario(n, seed):
    rng = random.Random(seed)
    palavras = [gerar_palavra_aleatoria(rng) for _ in range(n)]
    return palavras


def gerar_numeros(n, seed, faixa_multiplicador=10):
    rng = random.Random(seed)
    # amostragem sem reposicao de um universo maior que n, para reduzir
    # duplicatas (que nao quebram as estruturas, mas simplificam a analise)
    universo = n * faixa_multiplicador
    if universo < n:
        universo = n
    return rng.sample(range(1, universo + 1), n)


def gerar_pontos(n, seed, limite=100.0):
    rng = random.Random(seed)
    pontos = []
    for _ in range(n):
        x = round(rng.uniform(0, limite), 2)
        y = round(rng.uniform(0, limite), 2)
        pontos.append((x, y))
    return pontos


def escrever_linhas(caminho, linhas):
    with open(caminho, "w") as f:
        for linha in linhas:
            f.write(f"{linha}\n")


# ==========================================================
# EXECUCAO DOS BENCHMARKS
# ==========================================================
def rodar(executavel, caminho_dataset, distribuicao):
    cmd = [str(DIRETORIO_BASE / executavel), str(caminho_dataset), distribuicao]
    resultado = subprocess.run(cmd, capture_output=True, text=True, cwd=DIRETORIO_BASE)
    if resultado.returncode != 0:
        print(f"[AVISO] {executavel} retornou codigo {resultado.returncode} "
              f"(dataset={caminho_dataset}, distribuicao={distribuicao})")


def rodar_experimentos():
    print("=== Gerando datasets e rodando benchmarks ===")
    datasets_dir = DIRETORIO_BASE / "datasets_benchmark"
    datasets_dir.mkdir(exist_ok=True)

    for n in TAMANHOS:
        print(f"\n--- n = {n} ---")
        for rep in range(1, REPETICOES + 1):
            seed = hash((n, rep)) & 0xFFFFFFFF  # semente diferente por (tamanho, repeticao)

            # ---- Trie / Patricia (palavras) ----
            palavras = gerar_dicionario(n, seed)
            caminho_dic = datasets_dir / f"dicionario_{n}_{rep}.txt"
            escrever_linhas(caminho_dic, palavras)
            rodar("trie", caminho_dic, "aleatorio")
            rodar("patricia", caminho_dic, "aleatorio")

            # ---- Splay / Treap (numeros aleatorios) ----
            numeros = gerar_numeros(n, seed)
            caminho_num = datasets_dir / f"numeros_{n}_{rep}.txt"
            escrever_linhas(caminho_num, numeros)
            rodar("splay", caminho_num, "aleatorio")
            rodar("treap", caminho_num, "aleatorio")

            # ---- KD-Tree (pontos aleatorios) ----
            pontos = gerar_pontos(n, seed)
            caminho_pontos = datasets_dir / f"pontos_{n}_{rep}.txt"
            escrever_linhas(caminho_pontos, [f"{x} {y}" for x, y in pontos])
            rodar("kdtree", caminho_pontos, "aleatorio")

            print(f"  repeticao {rep}/{REPETICOES} concluida")

        # ---- Distribuicao "ordenado" (pior caso para Splay/Treap) ----
        # Roda so 1 vez por tamanho -- o objetivo aqui nao eh reduzir
        # ruido (o resultado eh deterministico), e sim mostrar o
        # comportamento em um cenario adversarial.
        seed_ordenado = hash((n, "ordenado")) & 0xFFFFFFFF
        numeros_ordenados = sorted(gerar_numeros(n, seed_ordenado))
        caminho_num_ord = datasets_dir / f"numeros_{n}_ordenado.txt"
        escrever_linhas(caminho_num_ord, numeros_ordenados)
        rodar("splay", caminho_num_ord, "ordenado")
        rodar("treap", caminho_num_ord, "ordenado")
        print(f"  distribuicao 'ordenado' concluida")

    print("\n=== Benchmarks concluidos ===\n")


# ==========================================================
# CONSOLIDACAO DOS CSVS
# ==========================================================
def consolidar_csvs():
    print("=== Consolidando resultados ===")
    linhas_consolidadas = []
    cabecalho = None

    for nome_csv in CSVS_GERADOS_PELOS_PROGRAMAS:
        caminho = DIRETORIO_BASE / nome_csv
        if not caminho.exists():
            print(f"[AVISO] {nome_csv} nao encontrado, pulando.")
            continue
        with open(caminho, newline="") as f:
            leitor = csv.reader(f)
            linhas = list(leitor)
        if not linhas:
            continue
        if cabecalho is None:
            cabecalho = linhas[0]
        linhas_consolidadas.extend(linhas[1:])  # pula o cabecalho de cada arquivo

    if cabecalho is None:
        print("[ERRO] Nenhum CSV de resultado encontrado. Rode os benchmarks primeiro.")
        return

    caminho_saida = DIRETORIO_BASE / "resultados_consolidado.csv"
    with open(caminho_saida, "w", newline="") as f:
        escritor = csv.writer(f)
        escritor.writerow(cabecalho)
        escritor.writerows(linhas_consolidadas)

    print(f"OK: {caminho_saida} ({len(linhas_consolidadas)} linhas de dados)")


if __name__ == "__main__":
    compilar_tudo()
    rodar_experimentos()
    consolidar_csvs()
