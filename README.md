# Projeto de Triagem Automatizada - EmbarcaTech 2025

**Autores:** Danilo Oliveira e Tífany Severo

**Curso:** Residência Tecnológica em Sistemas Embarcados

**Instituição:** EmbarcaTech - HBr

**Brasília, 2025**

-----

## 1\. 📖 Descrição do Projeto

Este projeto apresenta o desenvolvimento de um protótipo de **cabine de triagem automatizada** para serviços de urgência e emergência. A solução utiliza um sistema embarcado baseado na Raspberry Pi Pico W para automatizar a coleta de sinais vitais e sintomas do paciente, otimizando o fluxo de atendimento e empoderando a equipe de enfermagem com dados pré-processados em um dashboard em tempo real.

O sistema visa atacar a ineficiência nos processos de triagem hospitalar, reduzindo o tempo de espera e a sobrecarga dos profissionais de saúde, sem substituí-los, mas sim fornecendo uma ferramenta de suporte à decisão clínica.

-----

## 2\. 🎯 O Problema

Os serviços de urgência e emergência no Brasil enfrentam uma superlotação crônica, resultando em um gargalo crítico na triagem de classificação de risco. Estudos apontam que o tempo médio de espera para o início da triagem pode chegar a **20 horas** em grandes hospitais públicos, e apenas uma minoria dos pacientes de alto risco é atendida no tempo recomendado. Este cenário gera esgotamento profissional, atrasos no tratamento de casos graves e uma experiência negativa para o paciente.

O objetivo deste projeto é desenvolver uma solução tecnológica que otimize este processo, padronizando a coleta de dados iniciais e permitindo que a equipe de saúde foque na avaliação clínica e na tomada de decisão.

-----

## 3\. 💡 A Solução Proposta

A solução é um ecossistema integrado composto por três módulos principais:

1.  **Módulo da Cabine (Interação com o Paciente):** Uma estrutura física onde o paciente, de forma autônoma e guiada, tem seus sinais vitais aferidos por sensores e informa seus principais sintomas através de uma interface touchscreen.
2.  **Módulo de Processamento e Integração:** O cérebro do sistema, que processa os dados coletados, gera uma sugestão de classificação de risco baseada no Protocolo de Manchester e comunica-se de forma segura com a estação de enfermagem e o sistema do hospital (PEP).
3.  **Módulo de Supervisão (Estação de Enfermagem):** Um dashboard em tempo real que exibe a fila de pacientes, seus dados e a classificação de risco sugerida, permitindo que o enfermeiro valide, ajuste e confirme a triagem com agilidade e precisão.

### Arquitetura do Sistema

A interação entre os módulos garante um fluxo de dados contínuo e resiliente, desde a entrada do paciente na cabine até o registro final no prontuário eletrônico.

![Diagrama de Blocos Funcionais](./assets/Diagrama_De_blocos_funcionais.png)
*Figura 1 – Diagrama de Blocos Funcionais do Sistema*

-----

## 4\. 🛠️ Hardware e Tecnologias Utilizadas

### Módulo da Cabine

| Componente | Descrição |
| :--- | :--- |
| **Microcontrolador** | Placa BitDogLab (Raspberry Pi Pico W - RP2040 com Wi-Fi) |
| **Display (HMI)** | Display HMI Touchscreen Nextion |
| **Sensor de Oximetria** | Módulo MAX30102 (Aferição de SpO₂ e Frequência Cardíaca) |
| **Sensor de Temperatura**| Módulo Infravermelho MLX90614 (Aferição sem contato) |
| **Sensor de Distância** | Módulo VL53L0X (Verificação de posicionamento do paciente) |
| **Armazenamento** | Módulo para Cartão SD via SPI (Logs e operação offline) |
| **Módulo de som** | Módulo reprodução de instruções sonoras |
| **Software Embarcado** | C/C++, FreeRTOS, Pico-SDK |

### Estação de Enfermagem

| Componente | Descrição |
| :--- | :--- |
| **Hardware** | Computador / Terminal / Notebook padrão |
| **Software** | Aplicação Web (HTML, CSS, JavaScript) para o Dashboard |
| **Comunicação** | Rede TCP/IP (Wi-Fi), Protocolo HTTP |

-----

## 5\. 📂 Organização do Código

A estrutura do projeto foi pensada para ser modular e escalável, separando as responsabilidades de hardware, aplicação e comunicação.

```
/triagem-automatizada/
|
|-- 📂 src/                      # Código fonte principal do firmware da cabine
|   |-- 📂 drivers/              # Abstrações de hardware para os sensores e periféricos
|   |   |-- max30102.c
|   |   |-- mlx90614.c
|   |   `-- nextion_hmi.c
|   |
|   |-- 📂 tasks/                # Arquivos das tarefas do FreeRTOS
|   |   |-- sensor_task.c
|   |   |-- network_task.c
|   |   `-- state_machine_task.c
|   |
|   |-- main.c                   # Ponto de entrada, inicialização do sistema e das tasks
|   `-- config.h                 # Definições de configuração (pinos, senhas, etc.)
|
|-- 📂 include/                  # Arquivos de cabeçalho (.h)
|
|-- 📂 dashboard/                # Código fonte da aplicação web da Estação de Enfermagem
|   |-- index.html
|   |-- assets/css/style.css
|   `-- assets/js/script.js
|
|-- 📂 doc/                      # Documentação do projeto
|
|-- .gitignore
|-- CMakeLists.txt               # Arquivo de build para o firmware
`-- README.md
```

-----

## 6\. 🚀 Como Compilar e Executar

### Pré-requisitos

  * Raspberry Pi Pico SDK (`PICO_SDK_PATH`) configurado.
  * CMake, GCC para ARM.
  * Um servidor local (ex: Node.js com Express, Python com Flask) para rodar o backend do dashboard.
  * Duas redes Wi-Fi (ou uma com acesso cliente-servidor) para comunicação.

### Passo 1: Configuração do Servidor e Dashboard

1.  Navegue até a pasta `dashboard/`.
2.  Instale as dependências necessárias (ex: `npm install`).
3.  Inicie o servidor (ex: `npm start`).
4.  Anote o endereço IP da máquina que está rodando o servidor.

### Passo 2: Configuração do Firmware da Cabine

1.  Abra o arquivo `src/config.h`.

2.  Ajuste as macros com os dados da sua rede Wi-Fi e do servidor:

    ```c
    // filepath: src/config.h
    #define WIFI_SSID "SUA_REDE_WIFI"
    #define WIFI_PASSWORD "SUA_SENHA_WIFI"
    #define SERVER_IP "IP_DO_SEU_SERVIDOR_DASHBOARD" // Ex: "192.168.1.10"
    #define SERVER_PORT 3000
    ```

### Passo 3: Compilação do Firmware

Execute os seguintes comandos na raiz do projeto:

```bash
mkdir build
cd build
cmake ..
make
```

Após a compilação, o arquivo `triagem.uf2` estará disponível dentro da pasta `build/`.

### Passo 4: Execução

1.  **Dashboard:** Acesse o endereço `http://IP_DO_SEU_SERVIDOR_DASHBOARD:PORTA` em um navegador na Estação de Enfermagem.
2.  **Cabine:**
      * Conecte a Raspberry Pi Pico W ao computador enquanto segura o botão `BOOTSEL`.
      * Arraste e solte o arquivo `triagem.uf2` para o dispositivo de armazenamento que aparecer.
      * A placa reiniciará automaticamente e começará a executar o firmware da cabine, conectando-se à rede e ao servidor.

-----

## 7\. 📸 Imagens e Vídeos

### Protótipo Final

![Protótipo final da cabine de triagem](./assets/prototipo.png)
*Figura 2 – Protótipo final da cabine de triagem*

### Dashboard em Operação

![Protótipo final da cabine de triagem](./assets/dashboard.jpeg)
*Figura 3 – Dashboard exibindo dados do paciente em tempo real*

### Demonstração em Vídeo

[**Clique aqui para assistir ao vídeo do protótipo em funcionamento\!**](https://drive.google.com/drive/folders/1i_84O6hsd5oDjEaaV3iJG4wYRMZh1bOG?usp=sharing)

-----

## 8\. 📜 Licença

Este projeto foi desenvolvido para fins educacionais no âmbito do programa de Residência EmbarcaTech.

Licença: **GNU GPL-3.0**.
