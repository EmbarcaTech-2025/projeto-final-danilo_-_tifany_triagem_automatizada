# Projeto Final da Fase 2: Etapa 2 – Arquitetura e Modelagem
 

**Autores:** Danilo Oliveira e Tifany Severo

**Curso:** Residência Tecnológica em Sistemas Embarcados

**Instituição:** EmbarcaTech - HBr

Brasília, julho de 2025

---

## Arquitetura Geral do Sistema

O sistema proposto é composto por três módulos principais, interligados e integrados ao ambiente hospitalar:

### Módulo da Cabine (Interação com o Paciente)
- Estrutura física com sensores para aferição de sinais vitais.
- Interface touchscreen para coleta de dados e anamnese guiada.
- Unidade de processamento embarcada (BitDogLab RP2040) responsável pela comunicação com sensores, processamento preliminar e envio dos dados.

### Módulo de Processamento e Integração
- Algoritmo de apoio à decisão para pré-classificação de risco, baseado no Protocolo de Manchester.
- Detecção de sinais críticos e geração de alertas imediatos.
- Módulo de integração com o Prontuário Eletrônico do Paciente (PEP) via protocolos HL7 ou FHIR.
- Protocolo de comunicação para envio de dados para a estação de enfermagem.

### Módulo de Supervisão (Estação de Enfermagem)
- Dashboard em tempo real para visualização da fila de pacientes, dados coletados e pré-classificação sugerida.
- Interface para validação final da classificação de risco pelo enfermeiro e emissão de pulseira.
- Registro de auditoria com identificação do profissional responsável e timestamp.

---

## Diagrama de Blocos Funcionais

Fluxo principal:  
Paciente interage com a Cabine → dados coletados → pré-classificação → envio ao Dashboard → validação pelo enfermeiro → emissão de pulseira.

**Obs:** Em casos críticos, a cabine gera alerta sonoro/visual direto na estação.

### Fluxo de Dados
1. Entrada: sinais vitais e respostas ao questionário.
2. Processamento Local: filtragem, cálculo de médias, detecção de valores críticos.
3. Pré-Classificação.
4. Transmissão: dados e classificação preliminar enviados via rede segura.
5. Validação Profissional: ajustes e confirmação no dashboard.
6. Armazenamento: registro definitivo no PEP.
7. Saídas: emissão de pulseira e relatórios de telemetria.

### Considerações de Comunicação
- **Cabine → Processamento:** comunicação interna via I²C, SPI e UART para sensores e HMI.
- **Processamento → Estação:** comunicação de dados via rede TCP/IP segura.
- **Integração com PEP:** protocolo HL7 ou FHIR sobre HTTPS, com autenticação de API.

---

## Arquitetura de Hardware

### Módulo Cabine
- **Sensores médicos:**
  - Termômetro IR MLX90614 (I²C) – temperatura sem contato.
  - Oxímetro MAX30102 (I²C) – SpO₂ e frequência cardíaca.
  - Sensor de distância VL53L0X (I²C).
- **Interface homem-máquina:** Display Nextion (UART) para LGPD, identificação e anamnese.
- **Unidade de processamento:** BitDogLab RP2040 (dual-core, 133 MHz) executando drivers dos sensores, FSM de triagem, filtro/validação de medidas e empacotamento dos dados.
- **Armazenamento local:** SD Card (SPI) para logs e operação offline (cache de pacotes FHIR a sincronizar).

### Módulo Estação de Enfermagem
- Terminal/PC com Dashboard (web/app) para fila, visualização, ajuste e confirmação da classificação.
- Monitor dedicado para exibição do painel.
- Rede segura (TCP/IP) conectando a cabine ao ambiente hospitalar e ao HIS/PEP.

### Integração Hospitalar (externo ao protótipo)
- HIS/PEP recebe dados clínicos, consentimento e registros de auditoria via HL7/FHIR sobre HTTPS.

---

## Interfaces e Protocolos

| Origem → Destino      | Interface            | Função               | Observações |
|----------------------|---------------------|----------------------|-------------|
| MLX90614 → RP2040    | I²C @100–400 kHz    | Temperatura          | Pull-ups de 4.7 kΩ; end. I²C 0x5A |
| MAX30102 → RP2040    | I²C @400 kHz        | SpO₂ / FC            | Amostragem 50–100 Hz; ajuste LED |
| VL53L0X ↔ RP2040     | I²C                 | Distância            | — |
| Nextion ↔ RP2040     | UART 3.3 V          | Interação usuário    | 115200 bps; checksum/ACK |
| SD Card ↔ RP2040     | SPI @10–20 MHz      | Logs/Offline         | ≥8 GB; FAT32 |
| RP2040 ↔ Rede        | Ethernet/Wi-Fi      | Telemetria/PEP       | TLS 1.2+ |
| Cabine/Estação ↔ HIS/PEP | HTTPS + HL7/FHIR | Integração clínica   | OAuth2; timestamp |

**Sincronização Offline:** Pacotes contendo dados, sinais vitais, anamnese, pré-classificação e consentimento são armazenados no SD e reenviados assim que a rede for restaurada.

---

## Fluxograma de Software

### Visão Geral
O sistema é composto por dois fluxos principais:

- **Cabine:** execução autônoma de triagem e coleta de dados, com operação online ou offline, comunicação segura com o PEP/HIS e emissão de ticket.
- **Estação de Enfermagem:** supervisão, revisão e confirmação da classificação, priorização de casos críticos e integração final ao PEP/HIS.

Esses fluxos garantem:
- Resiliência (modo offline, repetição em caso de falhas);
- Privacidade (consentimento LGPD, criptografia);
- Usabilidade (HMI guiada);
- Segurança clínica (alerta crítico);
- Integração (HL7/FHIR via HTTPS).

### Elementos Técnicos Relevantes
- Operação offline com cache no SD e reenvio assíncrono.
- Repetição automática em caso de falha de sensor ou leitura fora da faixa de confiança.
- Controle de acesso no dashboard.
- Integração PEP/HIS via HL7 FHIR.
- Auditoria detalhada de alterações na classificação.

---