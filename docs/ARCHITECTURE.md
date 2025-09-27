# Arquitetura - v1-logging

## Visão Geral

A biblioteca libtslog implementa um sistema de logging thread-safe usando o padrão MPSC (Multi-Producer Single-Consumer). Este design garante alta performance minimizando a contenção de locks enquanto garante thread safety.

## Design Principal

A biblioteca usa uma **thread dedicada de escrita** que consome mensagens de uma fila compartilhada. Múltiplas threads produtoras podem seguramente enfileirar mensagens usando uma fila protegida por mutex, enquanto uma única thread consumidora lida com todas as operações de I/O de forma assíncrona.

**Por que MPSC?** Este padrão elimina a necessidade de sincronização durante escritas em arquivo, reduz o overhead de syscalls através de batching, e fornece melhor performance sob alta concorrência comparado a handles de arquivo por thread.

## Diagrama de Arquitetura

![Architecture Overview](diagrams/architecture.puml)

O diagrama mostra a implementação atual da libtslog e componentes placeholder para futuro desenvolvimento do sistema JobFleet (Server, Worker, Enqueue).

## Detalhes da Implementação

- **Gerenciamento de Fila**: `std::deque` com `std::mutex` e `std::condition_variable`
- **Escritas em Lote**: Thread de escrita faz flush das mensagens em lotes (a cada ~50ms ou quando buffer está cheio)
- **Rotação de Log**: Rotação baseada no tamanho do arquivo com esquema simples de backup `.1`
- **Tratamento de Erros**: API que não lança exceções; erros logados no stderr sem parar o serviço
- **Timestamps**: UTC com precisão de milissegundos usando `std::chrono`