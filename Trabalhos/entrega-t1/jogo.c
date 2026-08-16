#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#define POS_DIA 10
#define POS_NOITE 5
#define ESC_MAX 3
#define TIROS_MAX 30
#define ATQ_DIA 20
#define ATQ_NOITE 15
#define VAZIO ' '
#define ARQ_RECORDES "recordes.txt"
#define SONS_DIR "Sons/"

// implementação de um cronômetro
typedef struct timespec crono_t;

// estado completo de uma partida do jogo
typedef struct {
    bool terminou;
    bool onda_terminada;
    int pontos;
    int onda;
    bool noturno;
    int tiros;
    int escudos;
    int n_pos;
    char ataques[POS_DIA];
    int inativos;
    int arma_idx;
    const char *armas;
    int n_armas;
    double intervalo;
    int recordes[3];
    crono_t crono;
} estado_t;

// configura o terminal para o modo "cru", para permitir a leitura
//   de cada caractere digitado sem esperar pelo "enter".
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

// configura o terminal para o modo normal, com bufferização por linha.
void normaliza_terminal()
{
    system("stty sane");
}

// lê um caractere do teclado.
// retorna o código do caractere lido ou 0 casa nada tenha sido digitado.
// só funciona corretamente se o terminal estiver em modo "cru".
char lechar()
{
    fflush(stdout);
    char c;
    if (fread(&c, 1, 1, stdin) == 1) {
        return c;
    }
    return 0;
}

// inicializa um cronômetro com a hora atual
void crono_inicia(crono_t *c)
{
    clock_gettime(CLOCK_MONOTONIC, c);
}

// retorna o tempo passado desde que o cronômetro *c foi iniciado, em segundos
double crono_parcial(crono_t *c)
{
    crono_t agora;
    clock_gettime(CLOCK_MONOTONIC, &agora);
    double segundos = agora.tv_sec - c->tv_sec;
    double nanosegundos = agora.tv_nsec - c->tv_nsec;
    return segundos + 1e-9 * nanosegundos;
}

// pede ao sistema para tocar um arquivo de som, sem bloquear o programa.
void toca_som(const char *arquivo)
{
    char comando[128];
    snprintf(comando, sizeof(comando), "aplay -q %s &", arquivo);
    system(comando);
}

// monta em "nome" o arquivo de som correspondente a um tipo ou simbolo
void nome_som(char tipo, char *nome, size_t tam)
{
    if (tipo == 'N' || tipo == 'n') {
        snprintf(nome, tam, "11.3.wav");
    } else if (tipo == 'S') {
        snprintf(nome, tam, "12.3.wav");
    } else if (tipo >= '0' && tipo <= '9') {
        snprintf(nome, tam, "%c.3.wav", tipo);
    } else {
        snprintf(nome, tam, "x.3.wav");
    }
}

// toca de forma assincrona, o som de um tipo de ataque ou arma ou escudo
void toca_som_ataque(char tipo)
{
    char nome[16];
    char caminho[32];
    nome_som(tipo, nome, sizeof(nome));
    snprintf(caminho, sizeof(caminho), SONS_DIR "%s", nome);
    toca_som(caminho);
}

// acrescenta a um comando de som em sequencia o som do tipo dado
void acrescenta_som(char *cmd, char tipo)
{
    char nome[16];
    nome_som(tipo, nome, sizeof(nome));
    strcat(cmd, " " SONS_DIR);
    strcat(cmd, nome);
}

// sorteia um tipo de ataque valido pro periodo atual (dia ou noite)
char sorteia_tipo(estado_t *est)
{
    static const char tipos_dia[] = "0123456789N";
    static const char tipos_noite[] = "02468N";
    const char *tipos = est->noturno ? tipos_noite : tipos_dia;
    int n = est->noturno ? 6 : 11;
    return tipos[rand() % n];
}

// calcula o intervalo, em segundos entre movimentos na onda atual
double calcula_intervalo(int onda, bool noturno)
{
    double base = 2.0;
    for (int i = 1; i < onda; i++) {
        base *= 0.9;
    }
    return noturno ? base * 3.0 : base;
}

// sorteia com base no numero da onda, se ela vai ser noturna ou diurna
bool decide_noturno(int onda)
{
    int p = 20;
    if (onda == 1) {
        p = 100;
    } else if (onda == 2) {
        p = 80;
    } else if (onda == 3) {
        p = 60;
    } else if (onda == 4) {
        p = 40;
    }
    int sorteio = rand() % 100;
    return sorteio >= p;
}

// carrega os 3 maiores recordes do arquivo, ou zera se ele nao existir
void carrega_recordes(int recordes[3])
{
    recordes[0] = recordes[1] = recordes[2] = 0;
    FILE *f = fopen(ARQ_RECORDES, "r");
    if (f == NULL) {
        return;
    }
    int lidos = fscanf(f, "%d %d %d", &recordes[0], &recordes[1],
                        &recordes[2]);
    if (lidos != 3) {
        recordes[0] = recordes[1] = recordes[2] = 0;
    }
    fclose(f);
}

// salva os 3 maiores recordes no arquivo
void salva_recordes(int recordes[3])
{
    FILE *f = fopen(ARQ_RECORDES, "w");
    if (f == NULL) {
        return;
    }
    fprintf(f, "%d %d %d\n", recordes[0], recordes[1], recordes[2]);
    fclose(f);
}

// coloca "pontos" entre os recordes, se for maior que algum deles
bool atualiza_recordes(int recordes[3], int pontos)
{
    if (pontos <= recordes[2]) {
        return false;
    }
    recordes[2] = pontos;
    if (recordes[2] > recordes[1]) {
        int tmp = recordes[1];
        recordes[1] = recordes[2];
        recordes[2] = tmp;
    }
    if (recordes[1] > recordes[0]) {
        int tmp = recordes[0];
        recordes[0] = recordes[1];
        recordes[1] = tmp;
    }
    return true;
}

// mostra os tres maiores recordes ja guardados pelo programa
void mostra_recordes(estado_t *est)
{
    printf("recordes: %d, %d, %d\r\n", est->recordes[0],
           est->recordes[1], est->recordes[2]);
}

// reinicia os campos de uma partida, mantendo os recordes carregados
void reinicia_partida(estado_t *est)
{
    est->terminou = false;
    est->pontos = 0;
    est->escudos = ESC_MAX;
    est->onda = 1;
}

// inicializa o estado para uma nova partida (pontos, ondas, recordes)
void inicializa_estado(estado_t *est)
{
    srand((unsigned) time(NULL));
    carrega_recordes(est->recordes);
    reinicia_partida(est);
}

// configura armas, posicoes e ataques da onda, conforme dia ou noite
void configura_armas(estado_t *est)
{
    static const char armas_dia[] = "0123456789n";
    static const char armas_noite[] = "02468n";
    if (est->noturno) {
        est->armas = armas_noite;
        est->n_armas = 6;
        est->n_pos = POS_NOITE;
        est->inativos = ATQ_NOITE;
    } else {
        est->armas = armas_dia;
        est->n_armas = 11;
        est->n_pos = POS_DIA;
        est->inativos = ATQ_DIA;
    }
}

// inicializa uma nova onda: periodo, arma, tiros e campo de ataques
void inicializa_onda(estado_t *est)
{
    est->noturno = decide_noturno(est->onda);
    configura_armas(est);
    est->tiros = TIROS_MAX;
    est->arma_idx = 0;
    for (int i = 0; i < est->n_pos; i++) {
        est->ataques[i] = VAZIO;
    }
    est->intervalo = calcula_intervalo(est->onda, est->noturno);
    est->onda_terminada = false;
    crono_inicia(&est->crono);
}

// prepara o terminal para o modo de jogo.
void inicializa_tela()
{
    configura_terminal();
}

// devolve o terminal ao estado normal, ao final do jogo
void desinicializa_tela()
{
    normaliza_terminal();
}

// avança a arma selecionada para a proxima da sequencia disponivel
void trocar_arma(estado_t *est)
{
    est->arma_idx = (est->arma_idx + 1) % est->n_armas;
    toca_som_ataque(est->armas[est->arma_idx]);
}

// calcula os pontos obtidos ao destruir um ataque na posicao "pos"
int valor_ataque(estado_t *est, int pos, char tipo)
{
    int deslocamento = (est->n_pos - 1) - pos;
    int base = 1 + deslocamento;
    int mult_tipo = (tipo == 'n' || tipo == 'N') ? 2 : 1;
    int mult_periodo = est->noturno ? 2 : 1;
    return base * mult_tipo * mult_periodo;
}

// procura, da esquerda pra direita, um ataque atingivel pela arma.
int procura_alvo(estado_t *est, char arma)
{
    for (int i = 0; i < est->n_pos; i++) {
        char t = est->ataques[i];
        if (t == arma || (arma == 'n' && t == 'N')) {
            return i;
        }
    }
    return -1;
}

// aplica o efeito de um acerto: danifica ou destroi o ataque atingido
void processar_acerto(estado_t *est, int pos, char arma)
{
    char tipo = est->ataques[pos];
    if (arma == 'n' && tipo == 'N') {
        est->ataques[pos] = 'n';
        toca_som_ataque('n');
        return;
    }
    est->pontos += valor_ataque(est, pos, tipo);
    est->ataques[pos] = VAZIO;
    toca_som_ataque(tipo);
}

// dispara a arma selecionada contra o ataque inimigo compativel
void atirar(estado_t *est)
{
    if (est->tiros <= 0) {
        return;
    }
    est->tiros--;
    char arma = est->armas[est->arma_idx];
    int pos = procura_alvo(est, arma);
    if (pos < 0) {
        toca_som(SONS_DIR "x.3.wav");
        return;
    }
    processar_acerto(est, pos, arma);
}

// emite em sequencia os sons dos escudos e dos ataques ativos
void ativar_sonar(estado_t *est)
{
    char cmd[512] = "aplay -q";
    for (int i = 0; i < est->escudos; i++) {
        acrescenta_som(cmd, 'S');
    }
    for (int i = 0; i < est->n_pos; i++) {
        acrescenta_som(cmd, est->ataques[i]);
    }
    system(cmd);
}

// le uma tecla e executa o comando correspondente do jogador
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
    } else if (c == ' ') {
        ativar_sonar(est);
    }
}

// verifica se existe algum ataque ativo (nao vazio) no campo
bool tem_ataque_ativo(estado_t *est)
{
    for (int i = 0; i < est->n_pos; i++) {
        if (est->ataques[i] != VAZIO) {
            return true;
        }
    }
    return false;
}

// trata a saida de um ataque pela esquerda: atinge escudo ou base
void colide_saida(estado_t *est)
{
    if (est->escudos > 0) {
        est->escudos--;
        toca_som_ataque('S');
    } else {
        est->terminou = true;
        est->onda_terminada = true;
        toca_som(SONS_DIR "x.3.wav");
    }
}

// move cada ataque ativo uma posição à esquerda, tratando colisoes
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

// coloca um novo ataque sorteado na posicao mais à direita do campo
void gerar_novo_ataque(estado_t *est)
{
    int pos = est->n_pos - 1;
    if (est->ataques[pos] != VAZIO) {
        return;
    }
    char tipo = sorteia_tipo(est);
    est->ataques[pos] = tipo;
    est->inativos--;
    toca_som_ataque(tipo);
}

// verifica se a onda terminou (sem ataques ativos ou inativos)
bool onda_completa(estado_t *est)
{
    return est->inativos == 0 && !tem_ataque_ativo(est);
}

// verifica se o intervalo de movimento passou e, se sim, avanca o tempo
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

// desenha a tela: minimalista à noite, com todo o estado de dia
void apresenta(estado_t *est)
{
    if (est->noturno) {
        printf(" %d   \r", est->pontos);
    } else {
        printf(" %d %d %c ", est->pontos, est->tiros,
               est->armas[est->arma_idx]);
        for (int i = 0; i < est->escudos; i++) {
            putchar(')');
        }
        for (int i = 0; i < est->n_pos; i++) {
            putchar(est->ataques[i]);
        }
        printf("   \r");
    }
    fflush(stdout);
}

// bloqueia lendo teclas até que a tecla "alvo" (ou esc) seja digitada
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

// aplica o bonus de fim de onda, mostra o resumo e aguarda o jogador
void finaliza_onda(estado_t *est)
{
    int mult = est->noturno ? 2 : 1;
    est->pontos += est->tiros * 2 * mult;
    est->pontos += est->escudos * 10 * mult;
    toca_som(SONS_DIR "12.3.wav");
    printf("\r\nfim da onda %d! pontos: %d\r\n", est->onda, est->pontos);
    printf("digite 'r' pra continuar...\r\n");
    aguarda_tecla(est, 'r');
}

// pergunta se quer jogar de novo, reinicia a partida se sim
void perguntar_jogar_de_novo(estado_t *est)
{
    printf("jogar de novo? (s/n)\r\n");
    char c = 0;
    while (c != 's' && c != 'n' && c != 27) {
        c = lechar();
    }
    if (c == 's') {
        reinicia_partida(est);
    } else {
        est->terminou = true;
    }
}

// mostra o resumo final da partida, no fim do jogo
void finaliza_partida(estado_t *est)
{
    toca_som(SONS_DIR "11.3.wav");
    printf("\r\nfim do jogo. pontuacao final: %d\r\n", est->pontos);
    if (atualiza_recordes(est->recordes, est->pontos)) {
        printf("novo recorde!\r\n");
        salva_recordes(est->recordes);
    }
    mostra_recordes(est);
    perguntar_jogar_de_novo(est);
}

// executa uma onda de ataques: laço de teclado, tempo e apresentacao
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

// executa uma partida inteira: repete ondas ate que ela termine
void joga_partida(estado_t *est)
{
    while (!est->terminou) {
        joga_onda(est);
    }
    finaliza_partida(est);
}

// funcao principal: inicializa o jogo e executa as partidas
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
