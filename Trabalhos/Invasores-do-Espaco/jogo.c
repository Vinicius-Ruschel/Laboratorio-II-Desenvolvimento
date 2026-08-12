#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#define TIROS_INICIAIS 10
#define INIMIGOS_INICIAIS 5
#define POS_DIA 10
#define VAZIO ' '

typedef struct timespec crono_t;

typedef struct {
    bool terminou;
    int pontos;
    int tiros;
    int inativos;
    char ataques[POS_DIA];
    int n_pos;
    int arma_idx;
    const char *armas;
    int n_armas;
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

void inicializa_estado(estado_t *est)
{
    srand((unsigned) time(NULL));
    est->terminou = false;
    est->pontos = 0;
    est->tiros = TIROS_INICIAIS;
    est->inativos = INIMIGOS_INICIAIS;
    est->n_pos = POS_DIA;
    est->arma_idx = 0;
    est->armas = "0123456789";
    est->n_armas = 10;
    for (int i = 0; i < est->n_pos; i++) {
        est->ataques[i] = VAZIO;
    }
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
    est->ataques[pos] = VAZIO;
    est->pontos++;
}

void processar_teclado(estado_t *est)
{
    char c = lechar();
    if (c == 27) {
        est->terminou = true;
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

void mover_ataques(estado_t *est)
{
    if (est->ataques[0] != VAZIO) {
        est->terminou = true;
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
    if (crono_parcial(&est->crono) < 1.0) {
        return;
    }
    crono_inicia(&est->crono);
    if (tem_ataque_ativo(est)) {
        mover_ataques(est);
    }
    if (est->inativos > 0) {
        gerar_novo_ataque(est);
    }
}

void apresenta(estado_t *est)
{
    printf(" %d %d %c ", est->pontos, est->tiros,
           est->armas[est->arma_idx]);
    for (int i = 0; i < est->n_pos; i++) {
        putchar(est->ataques[i]);
    }
    printf("   \r");
    fflush(stdout);
}

void joga_onda(estado_t *est)
{
    crono_inicia(&est->crono);
    while (!onda_completa(est) && !est->terminou) {
        processar_teclado(est);
        processar_tempo(est);
        apresenta(est);
    }
}

void joga_partida(estado_t *est)
{
    while (!est->terminou) {
        joga_onda(est);
        est->terminou = true;
    }
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
