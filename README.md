# 🚗 Painel LED Santana G3

Projeto de iluminação customizada para painel do Santana G3 utilizando:
- Arduino
- LEDs WS2812B
- Biblioteca FastLED
- EEPROM para persistência de dados
- Controle via 4 botões físicos
- Buzzer para feedback (opcional)

---

# 📦 Estrutura do Sistema

O painel é dividido em 3 zonas independentes:

- **NÚMEROS** (16 LEDs)
- **AGULHAS** (8 LEDs)
- **LCD** (4 LEDs)

Cada zona pode ter:
- Cor independente
- Efeito independente
- Brilho global configurável

---

# 🎛️ Controles

| Botão | Função |
|-------|--------|
| BTN 1 | Modo seleção de cor |
| BTN 3 | Down (Brilho / Cor - / Efeito -) |
| BTN 4 | Up (Brilho / Cor + / Efeito +) |
| BTN EFFECT | Modo seleção de efeito |
| BTN 3 + BTN 4 | Reset total |

---

# 🔆 MODO NORMAL

### Ajuste de brilho
- BTN 4 → Aumenta brilho
- BTN 3 → Diminui brilho
- Níveis: 1 a 10

Se atingir:
- Máximo → indicação visual verde
- Mínimo → indicação visual vermelha

As configurações são salvas automaticamente.

---

# 🎨 MODO SELEÇÃO DE COR

Permite configurar cores individualmente para:

- NÚMEROS
- AGULHAS
- LCD

## Como entrar
Segurar BTN 1 por 2 segundos.

## Selecionar zona
Pressionar BTN 1 (clique curto).

Ordem:
1. NÚMEROS
2. AGULHAS
3. LCD

A zona ativa pisca para identificação.

## Alterar cor
- BTN 4 → Próxima cor
- BTN 3 → Cor anterior

## Sair
Segurar BTN 1 por 2 segundos.

Se ficar 15 segundos sem interação, o modo encerra automaticamente.

---

# 🌈 MODO SELEÇÃO DE EFEITO

Permite aplicar efeitos independentes por zona.

## Como entrar
Segurar BTN EFFECT por 2 segundos.

## Selecionar zona
Pressionar BTN EFFECT (clique curto).

## Alterar efeito
- BTN 4 → Próximo efeito
- BTN 3 → Efeito anterior

## Lista de efeitos

0 - NONE  
1 - WAVE  
2 - RAINBOW_FAST  
3 - RAINBOW_SLOW  
4 - STROBE_FAST  
5 - STROBE_SLOW  
6 - FADE_SLOW  
7 - RGB_CYCLE_FAST  
8 - RGB_CYCLE_SLOW  

## Sair
Segurar BTN EFFECT por 2 segundos.

Encerramento automático após 15 segundos sem interação.

---

# 🔄 RESET COMPLETO

Para restaurar padrões de fábrica:

Segurar BTN 3 + BTN 4 por 5 segundos.

O sistema irá:
- Restaurar cores padrão
- Definir brilho nível 5
- Remover todos os efeitos
- Reiniciar automaticamente

---

# 💾 Persistência

As configurações são salvas na EEPROM:
- Cores
- Brilho
- Efeitos

Salvamento automático após alguns segundos sem interação.

---

# ⚙️ Requisitos

- Arduino compatível
- Biblioteca FastLED
- LEDs WS2812B
- 4 botões com pull-up
- Fonte adequada para LEDs

---

# 🛠️ Observações Técnicas

- Sistema com debounce por software
- Long press: 2 segundos
- Timeout de modos: 15 segundos
- Reset completo via combinação física
- Efeitos não-bloqueantes (baseados em millis)

---

# 📜 Licença

Projeto open-source para uso pessoal e automotivo.
Modifique e adapte conforme necessário.
