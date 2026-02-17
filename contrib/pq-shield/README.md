# PIP-2030: Escudo Pós-Quântico (Post-Quantum Shield)

## O Problema: Deadline 2030

A criptografia de chave pública atual (ECDSA, RSA) baseia-se na dificuldade de fatorar grandes números ou resolver o problema do logaritmo discreto. Computadores quânticos de grande escala (previstos para serem viáveis por volta de 2030) poderão quebrar esses algoritmos usando o algoritmo de Shor, permitindo a derivação de chaves privadas a partir de chaves públicas.

Isso representa uma ameaça existencial para sistemas como o Bitcoin e infraestruturas de segurança global.

## A Solução: Criptografia Baseada em Lattices (Dilithium)

Este repositório implementa o **PIP-2030**, uma ferramenta de referência (`pq-shield`) que utiliza **CRYSTALS-Dilithium (Mode 3)**, um dos algoritmos finalistas do NIST para padronização de criptografia pós-quântica.

Dilithium é baseado em problemas de lattices (reticulados) que são considerados difíceis mesmo para computadores quânticos.

## Ferramenta: `pq-shield`

O `pq-shield` é uma ferramenta de linha de comando (CLI) desenvolvida em Go para geração de chaves, assinatura e verificação segura.

### Instalação

Requer Go 1.20+.

```bash
go build -o pq-shield ./cmd/pq-shield
```

### Uso

#### 1. Geração de Chaves

Gera um par de chaves Dilithium (Mode 3).

```bash
./pq-shield keygen -pub public.key -priv private.key
```
*   `public.key`: Chave pública (segura para compartilhar).
*   `private.key`: Chave privada (DEVE ser mantida em segredo). Permissões de arquivo são restritas (0600) automaticamente.

#### 2. Assinar Mensagem

Cria uma assinatura digital para um arquivo.

```bash
./pq-shield sign -priv private.key -msg dados.txt -out dados.sig
```

#### 3. Verificar Assinatura

Verifica se a assinatura é válida para o arquivo e chave pública fornecidos.

```bash
./pq-shield verify -pub public.key -msg dados.txt -sig dados.sig
```

Se a verificação for bem-sucedida, o programa retorna `exit code 0`. Caso contrário, retorna `exit code 1` e mensagem de erro.

## Detalhes Técnicos

*   **Algoritmo**: CRYSTALS-Dilithium Mode 3 (NIST Round 3).
*   **Biblioteca**: `github.com/cloudflare/circl`.
*   **Segurança**: Operações de chave privada são protegidas por permissões de sistema de arquivos. Nenhuma chave é exposta em logs.
*   **Kernel/Sistema**: A implementação utiliza `/dev/urandom` (via `crypto/rand`) para entropia de alta qualidade.
