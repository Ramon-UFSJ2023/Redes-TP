Projeto: Servidor HTTP Básico e Cliente Navegador

Este projeto simula a comunicação básica da web, com um servidor HTTP simples e um cliente de linha de comando feitos em C usando sockets.

Visão Geral

O servidor serve arquivos e lista diretórios.

O cliente faz requisições GET e salva o conteúdo baixado.

Componentes
🖥️ servidor.c (meu_servidor)

Escuta na porta 8080.

Aceita apenas GET.

Serve arquivos com 200 OK e gera listagem se não houver index.html.

Retorna 404 se o arquivo não existir.

Bloqueia caminhos suspeitos (com ..).

🌐 cliente.c (meu_navegador)

Usa uma URL completa (ex: http://localhost:8080/arquivo.txt).

Analisa host, porta e caminho.

Envia um GET, lê a resposta e salva o arquivo (ou index.html se o caminho for /).

Mostra erros se o servidor responder com 404 ou outro código.

⚙️ Makefile

make → compila os dois programas.

make clean → remove os executáveis.

Como Usar

Inicie o servidor:

./meu_servidor diretorio_servidor/


No outro terminal, rode o cliente:

./meu_navegador http://localhost:8080/arquivo.txt


O cliente salva o arquivo baixado no diretório atual.
Pra testar listagem de diretório:

./meu_navegador http://localhost:8080/


Feito pra aprendizado e testes simples de HTTP. Pode ser extendido depois pra suportar mais métodos e funçoes.