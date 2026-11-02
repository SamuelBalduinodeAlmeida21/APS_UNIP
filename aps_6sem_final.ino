#include <Arduino.h>

// motores e velocidade
#define motor_df 4
#define motor_dt 3
#define motor_ef 7
#define motor_et 8
#define velocidade1 5
#define velocidade2 6
#define velocidade_motor 150

// sensores de linha
#define sensor_esquerda A0
#define sensor_direita A1

// sensor rgb 
#define rgb_s0 A2
#define rgb_s1 A3
#define rgb_s2 A4
#define rgb_s3 A5
#define out 2

// leds s
#define led_verde 9
#define led_amarelo 10
#define led_vermelho 11
#define led_farol_tras 12
#define led_farol_frente 13

// buzzer 
#define buzzer 1

// tempo de leitura e duração das pausas
#define intervalo_rgb 50
#define tempo_pausa 5000
#define tempo_vermelho 5000

// situações em que o carrinho pode estar
enum estado_tipo {
  espera_verde,
  seguindo,
  pausa_amarela,
  bloqueado_vermelho
};

estado_tipo estado = espera_verde;

// variáveis usadas para controlar os tempos e os leds
unsigned long ultima_leitura_rgb = 0;
unsigned long inicio_pausa = 0;
unsigned long inicio_vermelho = 0;
unsigned long ultimo_pisca = 0;
unsigned long ultimo_pisca_farol = 0;

bool amarelo_bloqueado = false;
bool led_amarelo_estado = false;
bool led_vermelho_estado = false;
bool led_farol_estado = false;
bool iniciar_pisca_farol = false;

String cor_atual = "INDEFINIDO";

// ajusta a velocidade dos motores
void velocidade() {
  analogWrite(velocidade1, velocidade_motor);
  analogWrite(velocidade2, velocidade_motor);
}

// faz o carrinho andar em linha reta
void andar() {
  digitalWrite(motor_df, HIGH);
  digitalWrite(motor_dt, LOW);
  digitalWrite(motor_ef, HIGH);
  digitalWrite(motor_et, LOW);
  velocidade();
}

// interrompe o movimento dos dois motores
void parar() {
  digitalWrite(motor_df, LOW);
  digitalWrite(motor_dt, LOW);
  digitalWrite(motor_ef, LOW);
  digitalWrite(motor_et, LOW);
}

// corrige a direção para a direita
void ajustar_direita() {
  digitalWrite(motor_df, LOW);
  digitalWrite(motor_dt, LOW);
  digitalWrite(motor_ef, HIGH);
  digitalWrite(motor_et, LOW);
  velocidade();
}

// corrige a direção para a esquerda
void ajustar_esquerda() {
  digitalWrite(motor_df, HIGH);
  digitalWrite(motor_dt, LOW);
  digitalWrite(motor_ef, LOW);
  digitalWrite(motor_et, LOW);
  velocidade();
}

// verifica a posição da linha e ajusta o movimento
void seguir_linha() {
  int leitura_direita = digitalRead(sensor_direita);
  int leitura_esquerda = digitalRead(sensor_esquerda);

  if (leitura_direita == LOW && leitura_esquerda == HIGH) {
    ajustar_direita();
  }
  else if (leitura_esquerda == LOW && leitura_direita == HIGH) {
    ajustar_esquerda();
  }
  else if (leitura_direita == HIGH && leitura_esquerda == HIGH) {
    andar();
  }
  else {
    parar();
  }
}

// faz a leitura de cada filtro do sensor rgb
int ler_cor(int filtro_s2, int filtro_s3) {
  digitalWrite(rgb_s2, filtro_s2);
  digitalWrite(rgb_s3, filtro_s3);
  delay(2);

  unsigned long leitura = pulseIn(out, LOW, 15000);

  return (leitura == 0) ? -1 : leitura;
}

// reúne as leituras dos canais vermelho, verde e azul
void ler_rgb(int &r, int &g, int &b) {
  r = ler_cor(LOW, LOW);
  g = ler_cor(HIGH, HIGH);
  b = ler_cor(LOW, HIGH);
}

// identifica qual cor está sendo lida pelo sensor
String detectar_cor(int r, int g, int b) {
  if (r < 0 || g < 0 || b < 0) return "INDEFINIDO";

  if (r < g * 0.75 && r < b * 0.85) return "VERMELHO";
  if (g < r * 0.85 && g < b * 0.90) return "VERDE";
  if (r < b * 0.85 && g < b * 0.90) return "AMARELO";

  return "INDEFINIDO";
}

// atualiza a cor identificada a cada intervalo definido
void atualizar_cor() {
  unsigned long agora = millis();

  if (agora - ultima_leitura_rgb >= intervalo_rgb) {
    ultima_leitura_rgb = agora;

    int r, g, b;
    ler_rgb(r, g, b);

    cor_atual = detectar_cor(r, g, b);
  }
}

// toca os bipes da contagem antes de iniciar o percurso
void som_largada() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(buzzer, HIGH);
    delay(300);
    digitalWrite(buzzer, LOW);
    delay(500);
  }

  digitalWrite(buzzer, HIGH);
  delay(1200);
  digitalWrite(buzzer, LOW);
}

// toca o som quando o carrinho encontra a cor amarela
void som_amarelo() {
  digitalWrite(buzzer, HIGH);
  delay(200);
  digitalWrite(buzzer, LOW);
  delay(150);

  digitalWrite(buzzer, HIGH);
  delay(400);
  digitalWrite(buzzer, LOW);
}

// toca o som quando o carrinho encontra a cor vermelha
void som_vermelho() {
  for (int i = 0; i < 2; i++) {
    digitalWrite(buzzer, HIGH);
    delay(800);
    digitalWrite(buzzer, LOW);

    if (i == 0) delay(200);
  }
}

// apaga os leds de sinalização
void apagar_leds() {
  digitalWrite(led_verde, LOW);
  digitalWrite(led_amarelo, LOW);
  digitalWrite(led_vermelho, LOW);
}

// mantém os dois faróis acesos
void ligar_farois() {
  digitalWrite(led_farol_tras, HIGH);
  digitalWrite(led_farol_frente, HIGH);
}

// realiza a largada depois de identificar a cor verde
void iniciar_largada() {
  parar();
  ligar_farois();
  digitalWrite(led_verde, HIGH);

  som_largada();

  digitalWrite(led_verde, LOW);
  estado = seguindo;
  seguir_linha();
}

// configura os pinos e prepara os componentes
void setup() {
  pinMode(motor_df, OUTPUT);
  pinMode(motor_dt, OUTPUT);
  pinMode(motor_ef, OUTPUT);
  pinMode(motor_et, OUTPUT);

  pinMode(velocidade1, OUTPUT);
  pinMode(velocidade2, OUTPUT);

  pinMode(sensor_esquerda, INPUT);
  pinMode(sensor_direita, INPUT);

  pinMode(rgb_s0, OUTPUT);
  pinMode(rgb_s1, OUTPUT);
  pinMode(rgb_s2, OUTPUT);
  pinMode(rgb_s3, OUTPUT);
  pinMode(out, INPUT);

  // define a frequência de saída do sensor rgb em 100%
  digitalWrite(rgb_s0, HIGH);
  digitalWrite(rgb_s1, HIGH);

  pinMode(led_verde, OUTPUT);
  pinMode(led_amarelo, OUTPUT);
  pinMode(led_vermelho, OUTPUT);

  pinMode(led_farol_tras, OUTPUT);
  pinMode(led_farol_frente, OUTPUT);

  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);

  parar();
  apagar_leds();
  ligar_farois();
}

// verifica as cores e controla o carrinho durante o percurso
void loop() {
  atualizar_cor();

  switch (estado) {

    // espera identificar a primeira marcação verde
    case espera_verde:
      parar();

      if (cor_atual == "VERDE") {
        iniciar_largada();
      }
      break;

    // controla o carrinho enquanto ele segue a linha
    case seguindo:
      if (cor_atual != "AMARELO") {
        amarelo_bloqueado = false;
      }

      // para por cinco segundos ao identificar o amarelo
      if (cor_atual == "AMARELO" && !amarelo_bloqueado) {
        parar();
        digitalWrite(led_amarelo, HIGH);
        som_amarelo();

        amarelo_bloqueado = true;
        inicio_pausa = millis();
        ultimo_pisca = millis();
        led_amarelo_estado = true;

        estado = pausa_amarela;
      }

      // para ao identificar o vermelho
      else if (cor_atual == "VERMELHO") {
        parar();
        digitalWrite(led_vermelho, HIGH);
        som_vermelho();

        inicio_vermelho = millis();
        ultimo_pisca = millis();
        led_vermelho_estado = true;

        iniciar_pisca_farol = false;
        ligar_farois();

        estado = bloqueado_vermelho;
      }
      else {
        seguir_linha();
      }
      break;

    // mantém o carrinho parado durante a pausa amarela
    case pausa_amarela:
      parar();

      // faz o led amarelo piscar
      if (millis() - ultimo_pisca >= 300) {
        ultimo_pisca = millis();
        led_amarelo_estado = !led_amarelo_estado;
        digitalWrite(led_amarelo, led_amarelo_estado);
      }

      // retoma o percurso depois de cinco segundos
      if (millis() - inicio_pausa >= tempo_pausa) {
        digitalWrite(led_amarelo, LOW);
        estado = seguindo;
      }
      break;

    // mantém o carrinho parado enquanto o vermelho estiver ativo
    case bloqueado_vermelho:
      parar();

      // nos primeiros cinco segundos, faz o led vermelho piscar
      if (!iniciar_pisca_farol) {
        if (millis() - ultimo_pisca >= 300) {
          ultimo_pisca = millis();
          led_vermelho_estado = !led_vermelho_estado;
          digitalWrite(led_vermelho, led_vermelho_estado);
        }

        // depois de cinco segundos, começa a piscar os faróis
        if (millis() - inicio_vermelho >= tempo_vermelho) {
          digitalWrite(led_vermelho, LOW);

          iniciar_pisca_farol = true;
          ultimo_pisca_farol = millis();
          led_farol_estado = false;
        }
      }

      // mantém os faróis piscando até identificar o verde
      else {
        if (millis() - ultimo_pisca_farol >= 1000) {
          ultimo_pisca_farol = millis();
          led_farol_estado = !led_farol_estado;

          digitalWrite(led_farol_tras, led_farol_estado);
          digitalWrite(led_farol_frente, led_farol_estado);
        }

        // libera o carrinho somente quando encontrar o verde
        if (cor_atual == "VERDE") {
          parar();
          apagar_leds();
          ligar_farois();

          amarelo_bloqueado = false;
          iniciar_pisca_farol = false;

          iniciar_largada();
        }
      }
      break;
  }
}
