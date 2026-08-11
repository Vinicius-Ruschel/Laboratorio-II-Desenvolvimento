#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#define TIROS_INICIAIS 10
#define INIMIGOS_INICIAIS 5

typedef struct {
    bool terminou;
    int pontos;
    int inimigos_inativos;
    int tiros;
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

typedef struct timespec crono_t;

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

void inicializa_estado(estado_t *est)
{
    est->terminou = false;
    est->pontos = 0;
    est->tiros = TIROS_INICIAIS;
    est->inimigos_inativos = INIMIGOS_INICIAIS;
}

void inicializa_tela()
{
    configura_terminal();
}

void desinicializa_tela()
{
    normaliza_terminal();
}

void processar_teclado(estado_t *est)
{
    char c = lechar();
    if (c == 27) {
        est->terminou = true;
    } else if (c == 13 || c == 10) {
        if (est->tiros > 0) {
            est->tiros--;
        }
    }
}

void processar_tempo(estado_t *est)
{

    
}

void apresenta(estado_t *est)
{
    printf(" pontos: %d  tiros: %d   \r", est->pontos, est->tiros);
    fflush(stdout);
}

void joga_onda(estado_t *est)
{
    while (est->tiros > 0 && !est->terminou) {
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
