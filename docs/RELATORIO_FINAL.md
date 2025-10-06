# Relatório Final — v2-cli

Status: funcionalidades obrigatórias concluídas e logging integrado.

## Diagrama de Sequência (Cliente ⇄ Servidor)

![Client-Server Sequence](diagrams/client_server_sequence.puml)

Descrição: o `chat_client` estabelece conexão TCP, envia mensagens delimitadas por `\n`, o `chat_server` registra eventos via `libtslog` e realiza broadcast das linhas recebidas para os demais clientes conectados. Encerramento é feito via `SIGINT/SIGTERM` com limpeza de recursos.

## Mapeamento Requisitos → Código

- Logging (libtslog)
  - Fila MPSC + thread dedicada: `src/libtslog/tslog.cpp:19`, `src/libtslog/tslog.cpp:125`, `src/libtslog/tslog.cpp:174`
  - Timestamp ISO-8601 (UTC, ms): `src/libtslog/tslog.cpp:40`
  - Níveis e formatação de linha: `src/libtslog/tslog.cpp:52`, `src/libtslog/tslog.cpp:201`, `src/libtslog/tslog.cpp:212`
  - Rotação opcional por tamanho: `src/libtslog/tslog.cpp:68`
  - Saída simultânea para stdout: `src/libtslog/tslog.cpp:112`
  - Inicialização/encerramento + diretórios: `src/libtslog/tslog.cpp:157`, `src/libtslog/tslog.cpp:167`, `src/libtslog/tslog.cpp:177`

- Servidor de chat (`chat_server`)
  - CLI (porta, log, nível): `src/net/chat_server.cpp:27`, `src/net/chat_server.cpp:40`, `src/net/chat_server.cpp:83`
  - Sinais para shutdown gracioso: `src/net/chat_server.cpp:89`
  - Socket/bind/listen/SO_REUSEADDR: `src/net/chat_server.cpp:92`, `src/net/chat_server.cpp:99`, `src/net/chat_server.cpp:106`
  - Loop principal com `select(2)`: `src/net/chat_server.cpp:120`
  - Aceitar conexões + log: `src/net/chat_server.cpp:141`, `src/net/chat_server.cpp:146`
  - Receber por linhas e broadcast: `src/net/chat_server.cpp:165`, `src/net/chat_server.cpp:171`, `src/net/chat_server.cpp:173`
  - Desconexão/limpeza e encerramento: `src/net/chat_server.cpp:180`, `src/net/chat_server.cpp:186`

- Cliente de chat (`chat_client`)
  - CLI (host, porta, log, nível, batch): `src/net/chat_client.cpp:30`, `src/net/chat_client.cpp:43`
  - Conexão TCP + logs: `src/net/chat_client.cpp:111`, `src/net/chat_client.cpp:116`
  - Thread leitora (stdout do chat): `src/net/chat_client.cpp:76`, `src/net/chat_client.cpp:118`
  - Modo batch com `--count/--delay`: `src/net/chat_client.cpp:120`, `src/net/chat_client.cpp:127`
  - Modo interativo (stdin → socket): `src/net/chat_client.cpp:132`

- Scripts e Build
  - Teste de chat multi-cliente: `scripts/test_chat.sh:1`
  - Stress do logger (v1): `scripts/stress_log.sh:1`
  - Build: `Makefile:1`, `CMakeLists.txt:1`

## Relatório de Análise (IA)

- Correção e confiabilidade
  - Fragmentação de linhas: o servidor processa somente o chunk recebido no `recv(2)` atual e descarta fragmentos de linhas que cruzam fronteiras de leitura. Há um `recv_buf` global (`src/net/chat_server.cpp:118`) não utilizado. Sugestão: manter um buffer por cliente (ex.: `std::unordered_map<int, std::string>`) acumulando até `\n` e só então logar/broadcastar.
  - `send(2)` sem verificação de envio parcial: chamadas podem enviar menos bytes. Sugestão: laço até enviar tudo e tratar `EPIPE/EAGAIN` removendo o cliente conforme necessário.
  - Possível bloqueio em `send`: sockets não são non-blocking e não há `select` para escrita; em clientes lentos, `send` pode bloquear o loop. Sugestão: usar `O_NONBLOCK` e buffers de saída por cliente, monitorando `FD_SET` de escrita.

- Desempenho
  - Logging em stdout habilitado por padrão no servidor (`also_stdout=true`), o que aumenta I/O. Em produção, considerar desabilitar stdout e/ou elevar nível mínimo para `warn`.
  - Rotação de logs existe na lib, mas não está parametrizada no servidor (usa `max_bytes=0`). Adicionar opção CLI para rotação evita crescimento indefinido de arquivo.

- Segurança
  - Sem autenticação/controle de origem: qualquer cliente pode conectar e spammar. Considerar rate limiting por IP e autenticação básica/token em versões futuras.
  - Injeção em logs: mensagens do usuário vão para o log literalmente. Apesar de timestamps/nível fixos, ainda há risco de inserção de caracteres de controle. Sugestão: normalizar mensagens (remover caracteres não-imprimíveis, limitar tamanho por linha).

- Portabilidade e manutenção
  - Código POSIX (Linux/macOS). Para Windows, seria necessário adaptar sockets (`WSA*`) e `select`.
  - Estrutura simples e coesa: `libtslog` isolada e reutilizável, servidores/cliente consomem via headers públicos.

- Próximos passos sugeridos
  - Buffer por cliente (entrada/saída), sockets non-blocking e `poll/epoll`.
  - CLI para rotação de logs (`--log-max-bytes`) e diretório configurável.
  - Testes automatizados de linha dividida, envio parcial e clientes lentos.
  - Documentar contrato de protocolo (delimitação por `\n`, limites de tamanho) em `docs/`.

