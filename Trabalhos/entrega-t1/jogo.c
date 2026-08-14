#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#define TIROS_MAX 10
#define INIMIGOS_INICIAIS 5
#define POS_DIA 10
#define ESC_MAX 3
#define VAZIO ' '

typedef struct timespec crono_t;

typedef struct {
    bool terminou;
    bool onda_terminada;
    int pontos;
    int onda;
    int tiros;
    int escudos;
    int n_pos;
    char ataques[POS_DIA];
    int inativos;
    int arma_idx;
    const char *armas;
    int n_armas;
    double intervalo;
    crono_t crono;
} estado_t;

void configura_terminal()
{
    if (system("stty raw -echo min 0 time 1 opost") != 0) {
        perror("erro na execução de system(\"stty\")");
        fprintf(stderr, "você tem o programa stty instalado?\n");
        exit(1);
    }
    if (setvbuf(stdin, NULL, _IONBF, 0) != 0) {
        perror("erro na execução de setvbuf()");
        exit(1);
    }
}

void normaliza_terminal()
{
    system("stty sane");
}

char lechar()
{
    fflush(stdout);
    char c;
    if (fread(&c, 1, 1, stdin) == 1) return c;
    return 0;
}

void crono_inicia(crono_t *c)
{
    clock_gettime(CLOCK_MONOTONIC, c);
}

double crono_parcial(crono_t *c)
{
    crono_t agora;
    clock_gettime(CLOCK_MONOTONIC, &agora);
    double segundos = agora.tv_sec - c->tv_sec;
    double nanosegundos = agora.tv_nsec - c->tv_nsec;
    return segundos + 1e-9 * nanosegundos;
}

void toca_som(const char *arquivo)
{
    char comando[128];
    snprintf(comando, sizeof(comando), "aplay -q %s &", arquivo);
    system(comando);
}

char sorteia_tipo(estado_t *est)
{
    return est->armas[rand() % est->n_armas];
}

double calcula_intervalo(int onda)
{
    double base = 2.0;
    for (int i = 1; i < onda; i++) {
        base *= 0.9;
    }
    return base;
}

void inicializa_onda(estado_t *est)
{
    est->tiros = TIROS_MAX;
    est->arma_idx = 0;
    for (int i = 0; i < est->n_pos; i++) {
        est->ataques[i] = VAZIO;
    }
    est->inativos = INIMIGOS_INICIAIS;
    est->intervalo = calcula_intervalo(est->onda);
    est->onda_terminada = false;
    crono_inicia(&est->crono);
}

void inicializa_estado(estado_t *est)
{
    srand((unsigned) time(NULL));
    est->terminou = false;
    est->pontos = 0;
    est->escudos = ESC_MAX;
    est->onda = 1;
    est->n_pos = POS_DIA;
    est->armas = "0123456789";
    est->n_armas = 10;
    inicializa_onda(est);
}

void inicializa_tela()
{
    configura_terminal();
}

void desinicializa_tela()
{
    normaliza_terminal();
}

void trocar_arma(estado_t *est)
{
    est->arma_idx = (est->arma_idx + 1) % est->n_armas;
}

int procura_alvo(estado_t *est, char arma)
{
    for (int i = 0; i < est->n_pos; i++) {
        if (est->ataques[i] == arma) {
            return i;
        }
    }
    return -1;
}

int valor_ataque(estado_t *est, int pos)
{
    int deslocamento = (est->n_pos - 1) - pos;
    return 1 + deslocamento;
}

void atirar(estado_t *est)
{
    if (est->tiros <= 0) {
        return;
    }
    est->tiros--;
    char arma = est->armas[est->arma_idx];
    int pos = procura_alvo(est, arma);
    if (pos < 0) {
        return;
    }
    est->pontos += valor_ataque(est, pos);
    est->ataques[pos] = VAZIO;
}

void processar_teclado(estado_t *est)
{
    char c = lechar();
    if (c == 27) {
        est->terminou = true;
        est->onda_terminada = true;
    } else if (c == 9) {
        trocar_arma(est);
    } else if (c == 13 || c == 10) {
        atirar(est);
    }
}

bool tem_ataque_ativo(estado_t *est)
{
    for (int i = 0; i < est->n_pos; i++) {
        if (est->ataques[i] != VAZIO) {
            return true;
        }
    }
    return false;
}

void colide_saida(estado_t *est)
{
    if (est->escudos > 0) {
        est->escudos--;
    } else {
        est->terminou = true;
        est->onda_terminada = true;
    }
}

void mover_ataques(estado_t *est)
{
    if (est->ataques[0] != VAZIO) {
        colide_saida(est);
    }
    for (int i = 0; i < est->n_pos - 1; i++) {
        est->ataques[i] = est->ataques[i + 1];
    }
    est->ataques[est->n_pos - 1] = VAZIO;
}

void gerar_novo_ataque(estado_t *est)
{
    int pos = est->n_pos - 1;
    if (est->ataques[pos] != VAZIO) {
        return;
    }
    est->ataques[pos] = sorteia_tipo(est);
    est->inativos--;
}

bool onda_completa(estado_t *est)
{
    return est->inativos == 0 && !tem_ataque_ativo(est);
}

void processar_tempo(estado_t *est)
{
    if (crono_parcial(&est->crono) < est->intervalo) {
        return;
    }
    crono_inicia(&est->crono);
    if (tem_ataque_ativo(est)) {
        mover_ataques(est);
    }
    if (est->inativos > 0) {
        gerar_novo_ataque(est);
    }
    if (onda_completa(est)) {
        est->onda_terminada = true;
    }
}

void apresenta(estado_t *est)
{
    printf(" %d %d %c ", est->pontos, est->tiros,
           est->armas[est->arma_idx]);
    for (int i = 0; i < est->escudos; i++) {
        putchar(')');
    }
    for (int i = 0; i < est->n_pos; i++) {
        putchar(est->ataques[i]);
    }
    printf("   \r");
    fflush(stdout);
}

void aguarda_tecla(estado_t *est, char alvo)
{
    char c = 0;
    while (c != alvo) {
        c = lechar();
        if (c == 27) {
            est->terminou = true;
            return;
        }
    }
}

void finaliza_onda(estado_t *est)
{
    est->pontos += est->tiros * 2;
    est->pontos += est->escudos * 10;
    printf("\r\nfim da onda %d! pontos: %d\r\n", est->onda,
           est->pontos);
    printf("digite 'r' pra continuar\r\n");
    aguarda_tecla(est, 'r');
}

void joga_onda(estado_t *est)
{
    inicializa_onda(est);
    while (!est->onda_terminada) {
        processar_teclado(est);
        processar_tempo(est);
        apresenta(est);
    }
    if (!est->terminou) {
        finaliza_onda(est);
        est->onda++;
    }
}

void joga_partida(estado_t *est)
{
    while (!est->terminou) {
        joga_onda(est);
    }
    printf("\r\nfim do jogo. pontuacao final: %d\r\n", est->pontos);
}

int main()
{
    estado_t estado;
    inicializa_tela();
    inicializa_estado(&estado);
    while (!estado.terminou) {
        joga_partida(&estado);
    }
    desinicializa_tela();
}
