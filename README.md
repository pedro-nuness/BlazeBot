# BlazeBot
BlazeBot é um sistema automatizado para apostas em um jogo específico. Ele usa métodos de previsão e gerenciamento de apostas para otimizar as chances de sucesso do jogador.

![Screenshot_1](https://github.com/pedro-nuness/BlazeBot/assets/93084039/ed0e2a43-8ee8-447e-9cdf-4f8f24677c99)

## Mecanismo de Previsão
O mecanismo de previsão é responsável por analisar os padrões do jogo e fornecer uma estimativa da próxima cor que será sorteada. Ele inclui diferentes métodos de previsão, como padrões de histórico, matemática, inteligência artificial, sequência e streak (sequência de cores).

**1. Previsão por IA**
- O Double Predictor utiliza um modelo de inteligência artificial treinado com base em um histórico de jogadas para prever a próxima cor no jogo. O modelo é atualizado continuamente à medida que novos dados são coletados, permitindo uma melhoria contínua na precisão das previsões.

**2. Análise de padrões**
- Além da previsão por IA, o Double Predictor também realiza uma análise dos padrões presentes no histórico de jogadas. Ele procura por sequências recorrentes de cores ou outros padrões que possam indicar tendências nas jogadas.

**3. Detecção de sequências**
- O software identifica sequências de cores no histórico de jogadas e utiliza essa informação para fazer previsões mais precisas. Ele analisa a frequência e a ordem das cores nas sequências para determinar a próxima cor mais provável.

**4. Avaliação de certeza**
- Para cada previsão feita, o Double Predictor calcula a certeza da previsão com base em diversos fatores, como a confiabilidade dos dados de entrada e a precisão do modelo de IA. Essa avaliação de certeza é utilizada para tomar decisões mais assertivas durante o jogo.

**5. Gerenciador de Apostas**
- O gerenciador de apostas utiliza as previsões fornecidas pelo mecanismo de previsão para decidir quanto apostar e em que cor. Ele inclui uma lógica de apostas automática que ajusta as apostas com base no histórico e nas previsões atuais.

## Estrutura do Projeto
O projeto é dividido em várias partes:

### BetManager
O BetManager é o núcleo do sistema de gestão de apostas. Ele controla todas as transações de apostas e mantém o registro de apostas, histórico de resultados e saldo do jogador. Suas responsabilidades incluem:

- Calcular o valor das apostas com base em estratégias definidas, como multiplicação de apostas, proteção de lucros e gestão de capital.
- Manter o histórico de apostas e resultados para análise posterior.
- Monitorar o saldo do jogador e calcular o lucro ou prejuízo acumulado.
- Implementar lógica para proteger o capital do jogador e evitar perdas excessivas.
- Fornecer informações sobre o estado atual das apostas e resultados para outros componentes do sistema.

### DoublePredictor
O DoublePredictor é responsável por prever os resultados futuros com base em análises estatísticas e padrões observados nos resultados anteriores. Suas funcionalidades incluem:

- Analisar o histórico de resultados para identificar padrões e tendências.
- Utilizar métodos estatísticos e algoritmos de aprendizado de máquina para prever os resultados futuros.
- Oferecer diferentes estratégias de previsão, como padrões de repetição, análise estatística simples e métodos de inteligência artificial.
- Ajustar as previsões com base em eventos recentes e feedback do usuário.
- Fornecer informações sobre as previsões atuais e a confiabilidade das mesmas.

### Interface Gráfica do Usuário
- A interface gráfica do usuário é construída usando a biblioteca ImGui. Ela oferece uma maneira intuitiva e interativa para os usuários interagirem com o sistema de apostas. Suas principais características incluem:
- Exibir informações sobre o estado atual das apostas, resultados e saldo do jogador.
- Permitir ao usuário configurar diferentes estratégias de apostas e métodos de previsão.
- Apresentar gráficos e visualizações dos resultados e histórico de apostas.
- Possibilitar a interação do usuário através de controles como botões, caixas de seleção e sliders.


## Funcionalidades
- ```Apostas Automatizadas:``` O sistema pode fazer apostas automaticamente com base em diferentes métodos de previsão.
- ```Análise de Histórico:``` Exibe o histórico de resultados do jogo em uma janela separada.
- ```Gráficos de Saldo:``` Mostra um gráfico do histórico de saldo do jogador.
- ```Opções de Apostas Personalizáveis:``` Permite ajustar diversos parâmetros das apostas, como multiplicadores, limites de apostas, entre outros.
- ```Previsões Personalizáveis:``` Oferece opções para ajustar os parâmetros de previsão, como tamanho mínimo de padrões, porcentagem mínima e máxima de chance, etc.
   
## Pré-requisitos
- Windows
- DirectX 9
  
## Instalação
1. Clone o repositório para o seu ambiente local.
2. Abra o projeto no Visual Studio.
3. Compile o projeto para gerar o executável.
4. Como Usar
5. Execute o aplicativo após compilar.
6. Configure suas preferências de apostas e previsões nas janelas correspondentes.
7. Inicie o sistema de apostas automático.
8. Acompanhe os resultados e ajuste as configurações conforme necessário.

# DiscordBot 

![image](https://github.com/pedro-nuness/BlazeBot/assets/93084039/22133035-b158-4c4a-ba3e-49ddd569a435)

## Funcionalidades Principais:
- ```Predição Automática:``` O bot pode iniciar automaticamente a predição em canais específicos do Discord. Isso permite que os usuários iniciem e interrompam as predições conforme necessário.

- ```Monitoramento de Resultados:``` O BlazeBot monitora continuamente os resultados das apostas. Ele processa arquivos JSON para extrair informações sobre o saldo do jogador, lucros, cores apostadas e precisão das previsões.

- ```Envio de Mensagens Automáticas:``` Com base nos resultados das apostas, o bot gera mensagens formatadas com informações relevantes, como saldo final, lucro, cores apostadas e precisão das previsões. Essas mensagens são enviadas para os canais designados no Discord.

- ```Geração de Gráficos:``` Além das mensagens formatadas, o bot para discord também gera gráficos que mostram a evolução do saldo do jogador ao longo do tempo. Esses gráficos são anexados às mensagens enviadas para o Discord.

- ```Comandos de Controle:``` O bot oferece comandos de controle, como "clear" para limpar o chat, "shutdown" para desligar o computador e "exit" para encerrar o bot. Isso permite que os administradores controlem remotamente o comportamento das apostas remotamente.

### Referências
<a href="https://github.com/ocornut/imgui">ImGui</a> <br />
<a href="https://pytorch.org/">PyTorch</a> <br />
<a href="https://github.com/brainboxdotcc/DPP">DPP </a> <br />
<a href="https://github.com/SFML/SFML">SFML</a>
