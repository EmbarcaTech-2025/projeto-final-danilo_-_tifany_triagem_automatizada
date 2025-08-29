# Projeto Final da Fase 2: Etapa 3 – Arquitetura e Modelagem
 

**Autores:** Danilo Oliveira e Tifany Severo

**Curso:** Residência Tecnológica em Sistemas Embarcados

**Instituição:** EmbarcaTech - HBr

Brasília, julho de 2025

---

## Desafios Encontrados Durante o Desenvolvimento

Durante o desenvolvimento do protótipo da triagem automatizada, que abrange desde o cadastro inicial até o envio das medições para o dashboard em tempo real, diversos desafios foram enfrentados:

- **Integração de múltiplos sensores no barramento I²C**: A operação simultânea do MLX90614, MAX30102 e VL53L0X em frequências distintas exigiu extensivos testes até encontrar uma configuração estável. Apesar dos avanços, ocasionalmente ainda ocorrem travamentos no barramento.

- **Implementação do FreeRTOS com comunicação Wi-Fi**: As configurações exigidas são substancialmente diferentes das rotinas convencionais e impactaram módulos que já estavam parcialmente concluídos, demandando ajustes profundos no código.

- **Integração dos módulos em firmware único**: Conciliar as leituras de sensores, o gerenciamento da máquina de estados, o controle do display Nextion, armazenamento em cartão SD e reprodução de áudio em paralelo trouxe dificuldades de sincronização e desempenho. Esta integração ainda está em processo de refinamento.

- **Adaptação do display Nextion**: A incorporação do display ao ecossistema completo exigiu padronização na comunicação serial e ajustes para garantir que o fluxo da triagem fosse seguido corretamente e de forma estável.

## Funcionamento do Protótipo

O protótipo desenvolvido representa o ecossistema completo da solução, abrangendo desde a interação inicial do paciente até o envio das informações para o dashboard de monitoramento.

O fluxo de funcionamento segue a máquina de estados definida no projeto:
1. Tela inicial
2. Cadastro
3. Sintomas
4. Medição dos sinais vitais
5. Resultado
6. Envio dos dados ao servidor
7. Fim da triagem

No vídeo demonstrativo (link do vídeo), é possível observar a integração prática de todos os módulos do sistema:
- Sensores biomédicos (SpO₂, BPM, temperatura corporal e distância)
- Display Nextion para interação com o paciente
- Armazenamento em cartão SD
- Reprodução de áudio orientativo
- Comunicação com o servidor para exibição dos resultados em dashboard

O protótipo está funcional e representa uma prova de conceito robusta do ecossistema proposto.

## Melhorias Planejadas

Com base nos desafios enfrentados, algumas melhorias já foram definidas para as próximas etapas do projeto:

1. **Lógica do Protocolo de Manchester**: Implementar algoritmo baseado nesse protocolo internacionalmente reconhecido para prever automaticamente a cor da pulseira de triagem.

2. **Tela de confirmação LGPD**: Inclusão de uma etapa de consentimento explícito, conforme exigências da Lei Geral de Proteção de Dados, para garantir conformidade legal e segurança da informação.

3. **Interface de Anamnese**: Ampliar o módulo de sintomas na interface de usuário, permitindo registrar informações mais completas de anamnese.

4. **Alertas automáticos para dados críticos**: Implementação de notificações visuais e sonoras no dashboard sempre que forem detectados sinais vitais em faixas de risco, auxiliando na resposta rápida da equipe médica.

5. **Estruturação do protótipo em formato embarcado**: Migração do sistema atual de prototipagem para uma versão compacta, robusta e funcional, com design físico adequado para uso em unidades de saúde.

6. **Aprimoramento da estabilidade da integração**: Ajustes no barramento I²C, na sincronização de tarefas do RTOS e na comunicação Wi-Fi, visando eliminar travamentos e garantir confiabilidade em ambiente real.

7. **Formatação FHIR**: Verificar e assegurar que a estrutura de dados e envio ao servidor estejam compatíveis com o padrão FHIR (Fast Healthcare Interoperability Resources), garantindo interoperabilidade com sistemas de saúde.

Essas melhorias consolidam o potencial do projeto não apenas como prova de conceito, mas como uma solução prática, escalável e aplicável em diferentes cenários de atendimento em saúde.

[▶️ Assista ao vídeo demonstrativo](./Video_prototipo.mp4)