
# 🩺 Projeto Final da Fase 2 – Cabine de Triagem Automatizada

Autor: **Danilo Oliverira e Tifany Severo**

Curso: Residência Tecnológica em Sistemas Embarcados

Instituição: EmbarcaTech - HBr

Brasília, julho de 2025

## 🎯 Objetivo

Desenvolver um sistema automatizado capaz de realizar triagem clínica inicial de pacientes em serviços de urgência e emergência, utilizando sensores embarcados e algoritmos de pré-classificação, com foco na redução do tempo de espera, padronização dos dados,  apoio à equipe de enfermagem e melhoria da experiência do paciente.

## ⚠️ Problema

Serviços de emergência no Brasil sofrem com superlotação e triagens manuais lentas. Um estudo apontou que o tempo médio entre a chegada do paciente e o início da triagem pode ultrapassar **20 horas**, sendo que apenas **32% dos pacientes críticos** recebem atendimento no tempo adequado [1].

A automação do processo visa:
- Reduzir o tempo de espera.
- Padronizar a coleta de dados.
- Apoiar a decisão clínica com segurança e agilidade.

Casos de sucesso como Johns Hopkins e Hospital Santa Rita mostraram reduções de **30% a 50%** nos tempos de triagem após a adoção de tecnologias similares [2][3].

---

## ✅ Requisitos Funcionais

### Módulo da Cabine
- **RF-001**: Identificação via CNS ou CPF.
- **RF-002**: Aferição automatizada de temperatura, pressão arterial, SpO2 e frequência cardíaca.
- **RF-003**: Interface touchscreen com anamnese guiada.
- **RF-004**: Consentimento informado conforme LGPD.

### Módulo de Processamento
- **RF-005**: Algoritmo de pré-classificação com base no Protocolo de Manchester.
- **RF-006**: Alerta imediato em casos críticos (ex: SpO2 < 90%).
- **RF-007**: Integração com prontuário eletrônico (HL7/FHIR).

### Módulo de Supervisão
- **RF-008**: Dashboard de triagem para a equipe de enfermagem.
- **RF-009**: Validação final da classificação de risco pelo enfermeiro.

---

## 🔐 Requisitos Não Funcionais

- **RNF-001**: Certificação ANVISA dos sensores.
- **RNF-002**: Conformidade com CFM e COFEN.
- **RNF-003**: LGPD — criptografia em trânsito e em repouso.
- **RNF-004**: Acessibilidade (Norma ABNT NBR 9050) e modo assistido.
- **RNF-005**: Tempo máximo de atendimento: 5 minutos por paciente.
- **RNF-006**: Módulo de autodiagnóstico e materiais higienizáveis.

---

## 🔧 Lista de Materiais (BOM)

| Componente                                  | Qtd. | Função |
|---------------------------------------------|------|--------|
| Placa BitDogLab (RP2040 com Wi-Fi)          | 2    | Processamento |
| Display HMI Nextion (NX8048P070-011C)       | 1    | Interface touchscreen |
| Monitor LCD/HDMI (~7")                      | 2    | Exibição (Cabine + Enfermagem) |
| Sensor Oxímetro/Batimentos (MAX30102)       | 1    | SpO2 e frequência cardíaca |
| Sensor de Temperatura IR (MLX90614)         | 1    | Medição sem contato |
| Interface DVI/HDMI (BitDogLab)              | 2    | Vídeo |
| Placa para SDCARD SPI + MicroSD (8GB+)      | 2    | Armazenamento |
| Breadboard + Jumper Wires                   | 1 kit| Montagem protótipo |

---

## 📚 Referências

[1] BITTENCOURT, R. J.; HORTALE, V. A. *Intervenções para solucionar a superlotação nos serviços de emergência hospitalar*. Cad. Saúde Pública, 2014.  
[2] ARKANGEL AI. *Como a IA otimiza os tempos de espera nos hospitais*. [Link](https://www.arkangel.ai/br/blog-ai/complete-guide-on-how-ai-optimizes-hospital-waiting-times)  
[3] CONCLÍNICA. *Inteligência Artificial reduz tempo de triagem e melhora atendimento hospitalar*. [Link](https://conclinica.com.br/inteligencia-artificial-reduz-tempo-de-triagem-e-melhora-atendimento-hospitalar/)

---



