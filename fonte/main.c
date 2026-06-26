#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include <SDL_timer.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <string.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h> // so no navegador: pra disparar o efeito de desligar a tv
#endif

#define LINHAS 100 // Define o máximo de linhas do txt (placar)
#define TAMANHO 100 // Define o tamanho máximo de uma linha do txt
#define LARGURA_TELA 640 //Define a largura da janela
#define ALTURA_TELA 480 // Define a altura da janela
#define TAMANHO_BLOCO 32 // Define o tamanho do bloco
#define LARGURA_MAPA 11 // Define a largura do mapa
#define ALTURA_MAPA 11 // Define a altura do mapa

char mapa[LARGURA_MAPA][ALTURA_MAPA]; // Definindo o mapa

enum StatusEntidade
{
    vivo,
    morto
}; // Lógica de jogador/inimigo

enum ultimoMov
{
    esquerda,
    direita,
    frente,
    tras
}; // Lógica de movimentação jogador/inimigo

enum StatusJogo
{
    naoIniciado,
    aguardandoNome,
    vitoria,
    derrota,
    pausado,
    iniciado
}; // Status pra lógica do jogo

typedef struct {
    int x;
    int y;
    enum StatusEntidade status;
    enum ultimoMov ultimoMov;
} Entidade; // Estrutura pra movimentar jogador / inimigo

typedef struct {
    int x;
    int y;
} Bomba; // Estrutura pra bomba

Entidade jogador;

// Definindo status iniciais de cada item do jogo:
Entidade jogador = {1 ,1, vivo};
//Entidade inimigos[3] = {{9, 9, vivo}, {9, 9, vivo}, {9, 9, vivo}};
Entidade* inimigos;
Bomba bomba = {-1, -1}; // Coordenadas da bomba, inicialmente fora do mapa

// Definindo variáveis do jogo:
int pontuacao_inicial = 0;
int pontuacao_final = 0;
int temporizadorBomba = 0; // Temporizador para detonar a bomba
int menuPrincipal = 0; // 1: Iniciar Jogo, 2: Placar, 3: Sair
Uint32 ultimaTeclaMenu = 0; // pra ignorar toque duplicado nas telas de menu
int desligado = 0;          // quando aperta "sair": congela o jogo e deixa a tv desligar
enum StatusJogo statusJogo = naoIniciado; // Indica se o jogo está em iniciado, não iniciado ou aguardando o nome do jogador
char nomeJogador[20] = "";
char auxNomeJogador[20];
int numInimigos = 1; // Número de inimigos que será gerado (baseado na dificuldade)
int salvo = 0;
int contadorRecordes;

// sons 8-bit do jogo (carregados la no main)
Mix_Chunk* somMenu = NULL;
Mix_Chunk* somBomba = NULL;
Mix_Chunk* somExplosao = NULL;
Mix_Chunk* somPasso = NULL;
Mix_Chunk* somInimigoMorto = NULL;
Mix_Chunk* somVitoria = NULL;
Mix_Chunk* somGameOver = NULL;
Mix_Chunk* somDesligar = NULL;
Mix_Chunk* musicaMenu = NULL;     // trilha calminha do menu
Mix_Chunk* musicaArcade = NULL;   // trilha agitada do jogo
int musicaAtual = -1;             // -1 nenhuma, 0 menu, 1 arcade

// Inicializa o mapa com paredes e espaços vazios
void inicializarMapa() {
    for (int i = 0; i < LARGURA_MAPA; i++) {
        for (int j = 0; j < ALTURA_MAPA; j++) {
            if (i == 0 || i == LARGURA_MAPA - 1 || j == 0 || j == ALTURA_MAPA - 1) {
                mapa[i][j] = '#'; // Parede
            } else if (i % 2 == 0 && j % 2 == 0) {
                mapa[i][j] = '#'; // Obstáculo
            } else {
                mapa[i][j] = ' '; // Espaço vazio
            }
        }
    }
}

//Função para renderizar o plano de fundo desejado
void desenharfundo(SDL_Renderer* renderizador, SDL_Surface* fundo){
    SDL_Rect retangulo;
    retangulo.w = TAMANHO_BLOCO;
    retangulo.h = TAMANHO_BLOCO;

    SDL_Texture* textura = NULL;
    SDL_Surface* superficie = NULL;
    superficie = fundo;
    textura = SDL_CreateTextureFromSurface(renderizador, superficie);
    SDL_FreeSurface(superficie);

    SDL_Rect fundoRect;
    fundoRect.x = 0;
    fundoRect.y = 0;
    fundoRect.w = LARGURA_TELA;
    fundoRect.h = ALTURA_TELA;
    SDL_RenderCopy(renderizador, textura, NULL, &fundoRect);
    SDL_DestroyTexture(textura);

}

// Renderiza o mapa e o plano de fundo do jogo na tela
void desenharMapa(SDL_Renderer* renderizador) {
    SDL_Rect retangulo;
    retangulo.w = TAMANHO_BLOCO;
    retangulo.h = TAMANHO_BLOCO;

    int offsetX = (LARGURA_TELA - LARGURA_MAPA * TAMANHO_BLOCO) / 2;
    int offsetY = (ALTURA_TELA - ALTURA_MAPA * TAMANHO_BLOCO) / 2;

    SDL_Texture* textura = NULL;
    SDL_Surface* superficie = NULL;
    superficie = IMG_Load("Props\\areia.jpg");
    textura = SDL_CreateTextureFromSurface(renderizador, superficie);
    SDL_FreeSurface(superficie);

    SDL_Rect fundoRect;
    fundoRect.x = offsetX;
    fundoRect.y = offsetY;
    fundoRect.w = LARGURA_MAPA * TAMANHO_BLOCO;
    fundoRect.h = ALTURA_MAPA * TAMANHO_BLOCO;
    SDL_RenderCopy(renderizador, textura, NULL, &fundoRect);

    // textura de tijolinho pro muro, no lugar do quadrado preto
    SDL_Surface* superficieTijolo = IMG_Load("Props\\tijolos.png");
    SDL_Texture* texturaTijolo = SDL_CreateTextureFromSurface(renderizador, superficieTijolo);
    SDL_FreeSurface(superficieTijolo);

    for (int i = 0; i < LARGURA_MAPA; i++) {
        for (int j = 0; j < ALTURA_MAPA; j++) {
            retangulo.x = offsetX + j * TAMANHO_BLOCO;
            retangulo.y = offsetY + i * TAMANHO_BLOCO;

            if (mapa[i][j] == '#') {
                SDL_RenderCopy(renderizador, texturaTijolo, NULL, &retangulo);
            }
        }
    }

    SDL_DestroyTexture(texturaTijolo);
}

// Renderiza o inimigo na tela
void desenharInimigo(SDL_Renderer* renderizador, Entidade* inimigo) {
    if(inimigo->status == vivo)
    {
        SDL_Texture* textura = NULL;
        SDL_Surface* superficie = NULL;
        if(inimigo->ultimoMov == frente)
        {
            superficie = IMG_Load("Props\\inimigo_baixo.png");
        }
        else if(inimigo->ultimoMov == tras)
        {
            superficie = IMG_Load("Props\\inimigo_cima.png");
        }
        else if(inimigo->ultimoMov == esquerda)
        {
            superficie = IMG_Load("Props\\inimigo_esquerda.png");
        }
        else if(inimigo->ultimoMov == direita)
        {
            superficie = IMG_Load("Props\\inimigo_direita.png");
        }
        textura = SDL_CreateTextureFromSurface(renderizador, superficie);
        SDL_FreeSurface(superficie);


        SDL_Rect retangulo;
        int eixoX = (LARGURA_TELA - LARGURA_MAPA * TAMANHO_BLOCO) / 2;
        int eixoY = (ALTURA_TELA - ALTURA_MAPA * TAMANHO_BLOCO) / 2;
        retangulo.x = eixoX + inimigo->x * TAMANHO_BLOCO;
        retangulo.y = eixoY + inimigo->y * TAMANHO_BLOCO;

        retangulo.w = TAMANHO_BLOCO;
        retangulo.h = TAMANHO_BLOCO;

        SDL_RenderCopy(renderizador, textura, NULL, &retangulo);
    }
}

//Renderiza o jogador na tela
void desenharJogador(SDL_Renderer* renderizador, Entidade* jogador) {
    if(jogador->status == vivo)
    {
        SDL_Texture* textura = NULL;
        SDL_Surface* superficie = NULL;

        if(strcmp(nomeJogador, "raissa") == 0)
        {
            if(jogador->ultimoMov == frente)
            {
                superficie = IMG_Load("Props\\Dog\\tras.png");
            }
            else if(jogador->ultimoMov == tras)
            {
                superficie = IMG_Load("Props\\Dog\\frente.png");
            }
            else if(jogador->ultimoMov == esquerda)
            {
                superficie = IMG_Load("Props\\Dog\\esquerda.png");
            }
            else if(jogador->ultimoMov == direita)
            {
                superficie = IMG_Load("Props\\Dog\\direita.png");
            }
            textura = SDL_CreateTextureFromSurface(renderizador, superficie);
            SDL_FreeSurface(superficie);
        }
        else
        {
            if(jogador->ultimoMov == frente)
            {
                superficie = IMG_Load("Props\\tras.png");
            }
            else if(jogador->ultimoMov == tras)
            {
                superficie = IMG_Load("Props\\frente.png");
            }
            else if(jogador->ultimoMov == esquerda)
            {
                superficie = IMG_Load("Props\\esquerda.png");
            }
            else if(jogador->ultimoMov == direita)
            {
                superficie = IMG_Load("Props\\direita.png");
            }
            textura = SDL_CreateTextureFromSurface(renderizador, superficie);
            SDL_FreeSurface(superficie);
        }

        SDL_Rect retangulo;
        int eixoX = (LARGURA_TELA - LARGURA_MAPA * TAMANHO_BLOCO) / 2;
        int eixoY = (ALTURA_TELA - ALTURA_MAPA * TAMANHO_BLOCO) / 2;
        retangulo.x = eixoX + jogador->x * TAMANHO_BLOCO;
        retangulo.y = eixoY + jogador->y * TAMANHO_BLOCO;

        retangulo.w = TAMANHO_BLOCO;
        retangulo.h = TAMANHO_BLOCO;

        SDL_RenderCopy(renderizador, textura, NULL, &retangulo);
    }
}

//Renderiza a bomba na tela
void desenharBomba(SDL_Renderer* renderizador, Bomba* bomba){
    SDL_Texture* textura = NULL;
    SDL_Surface* superficie = IMG_Load("Props\\bomba.png");
    textura = SDL_CreateTextureFromSurface(renderizador, superficie);
    SDL_FreeSurface(superficie);

    SDL_Rect retangulo;
    int eixoX = (LARGURA_TELA - LARGURA_MAPA * TAMANHO_BLOCO) / 2;
    int eixoY = (ALTURA_TELA - ALTURA_MAPA * TAMANHO_BLOCO) / 2;
    retangulo.x = eixoX + bomba->x * TAMANHO_BLOCO;
    retangulo.y = eixoY + bomba->y * TAMANHO_BLOCO;

    retangulo.w = TAMANHO_BLOCO;
    retangulo.h = TAMANHO_BLOCO;

    SDL_RenderCopy(renderizador, textura, NULL, &retangulo);
}

//Renderiza a explosão da bomba na tela
void desenharBombaExplodida(SDL_Renderer* renderizador, Bomba* bomba){
    SDL_Texture* textura = NULL;
    SDL_Surface* superficie = IMG_Load("Props\\bombaexplodida.png");
    textura = SDL_CreateTextureFromSurface(renderizador, superficie);
    SDL_FreeSurface(superficie);

    SDL_Rect retangulo_1, retangulo_2, retangulo_3, retangulo_4, retangulo_5;
    int eixoX = (LARGURA_TELA - LARGURA_MAPA * TAMANHO_BLOCO) / 2;
    int eixoY = (ALTURA_TELA - ALTURA_MAPA * TAMANHO_BLOCO) / 2;

    retangulo_1.x = eixoX + bomba->x * TAMANHO_BLOCO;
    retangulo_1.y = eixoY + bomba->y * TAMANHO_BLOCO;
    retangulo_1.w = TAMANHO_BLOCO;
    retangulo_1.h = TAMANHO_BLOCO;
    SDL_RenderCopy(renderizador, textura, NULL, &retangulo_1);

    if (mapa[bomba->x+1][bomba->y] != '#')
    {
        retangulo_2.x = eixoX + (bomba->x + 1) * TAMANHO_BLOCO;
        retangulo_2.y = eixoY + bomba->y * TAMANHO_BLOCO;
        retangulo_2.w = TAMANHO_BLOCO;
        retangulo_2.h = TAMANHO_BLOCO;

        SDL_RenderCopy(renderizador, textura, NULL, &retangulo_2);
    }

    if (mapa[bomba->x-1][bomba->y] != '#')
    {
        retangulo_3.x = eixoX + (bomba->x - 1) * TAMANHO_BLOCO;
        retangulo_3.y = eixoY + bomba->y * TAMANHO_BLOCO;
        retangulo_3.w = TAMANHO_BLOCO;
        retangulo_3.h = TAMANHO_BLOCO;

        SDL_RenderCopy(renderizador, textura, NULL, &retangulo_3);
    }

    if (mapa[bomba->x][bomba->y+1] != '#')
    {
        retangulo_4.x = eixoX + bomba->x * TAMANHO_BLOCO;
        retangulo_4.y = eixoY + (bomba->y + 1) * TAMANHO_BLOCO;
        retangulo_4.w = TAMANHO_BLOCO;
        retangulo_4.h = TAMANHO_BLOCO;

        SDL_RenderCopy(renderizador, textura, NULL, &retangulo_4);
    }

    if (mapa[bomba->x][bomba->y-1] != '#')
    {
        retangulo_5.x = eixoX + bomba->x * TAMANHO_BLOCO;
        retangulo_5.y = eixoY + (bomba->y - 1) * TAMANHO_BLOCO;
        retangulo_5.w = TAMANHO_BLOCO;
        retangulo_5.h = TAMANHO_BLOCO;

        SDL_RenderCopy(renderizador, textura, NULL, &retangulo_5);
    }
}

// Move uma entidade (jogador, inimigo) no mapa
void moverEntidade(Entidade* entidade, int dx, int dy)
{
    if (mapa[entidade->x + dx][entidade->y + dy] != '#' && entidade->status == vivo) {
        entidade->x += dx;
        entidade->y += dy;
    }
}

// move os inimigos com um cerebrinho: foge da bomba e persegue o jogador
void mover_inimigos()
{
    // mexe o inimigo a cada 3 frames (mais devagar que o gato), senao fica impossivel
    static int relogio = 0;
    relogio++;
    if (relogio % 3 != 0) return;

    int dirs[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}}; // cima, baixo, esq, dir

    for (int i = 0; i < numInimigos; i++)
    {
        if (inimigos[i].status != vivo) continue; // morto nao anda

        int ex = inimigos[i].x;
        int ey = inimigos[i].y;

        // so se assusta com a bomba quando ela ta bem coladinha (reage tarde)
        int fugindo = (bomba.x != -1 && bomba.y != -1 &&
                       abs(ex - bomba.x) + abs(ey - bomba.y) <= 2);

        // chance de andar pra um lado aleatorio. perseguindo, escala com a fase
        // (mais coelho = mais burro). fugindo, e bem alto de proposito: ele entra
        // em panico e erra a fuga bastante, pra dar pra explodir ele.
        int chanceAleatoria = fugindo ? 65 : (15 + numInimigos * 12);
        int aleatorio = (rand() % 100) < chanceAleatoria;

        // escolhe a melhor das 4 direcoes livres
        int melhor = -1;
        int melhorNota = -1000000;
        for (int dir = 0; dir < 4; dir++)
        {
            int nx = ex + dirs[dir][0];
            int ny = ey + dirs[dir][1];
            if (mapa[nx][ny] == '#') continue; // parede, nem tenta

            int nota;
            if (aleatorio)
                nota = rand() % 100; // qualquer direcao livre serve
            else if (fugindo)
                nota = abs(nx - bomba.x) + abs(ny - bomba.y);     // longe da bomba e melhor
            else
                nota = -(abs(nx - jogador.x) + abs(ny - jogador.y)); // perto do jogador e melhor

            if (nota > melhorNota)
            {
                melhorNota = nota;
                melhor = dir;
            }
        }

        if (melhor >= 0)
        {
            int dx = dirs[melhor][0];
            int dy = dirs[melhor][1];
            inimigos[i].x = ex + dx;
            inimigos[i].y = ey + dy;
            if (dx == -1) inimigos[i].ultimoMov = esquerda;
            else if (dx == 1) inimigos[i].ultimoMov = direita;
            else if (dy == -1) inimigos[i].ultimoMov = frente;
            else if (dy == 1) inimigos[i].ultimoMov = tras;
        }
    }
}

// Coloca uma bomba na posição do jogador
void colocarBomba() {
    if (bomba.x == -1 && bomba.y == -1) {
        bomba.x = jogador.x;
        bomba.y = jogador.y;
        temporizadorBomba = 0; // Pra reiniciar o temporizador
        Mix_PlayChannel(-1, somBomba, 0); // som de plantar a bomba
    }
}

// Detona a bomba se o temporizador atingir 3 segundos
void detonarBomba()
{
    // Verifica se passaram 3 segundos desde que a bomba foi colocada
    if (temporizadorBomba >= 10) {
        Mix_PlayChannel(-1, somExplosao, 0); // bum
        for (int i = 0; i < numInimigos; i++)
        {
            // Verifica se a bomba está no range do inimigo
            if ((bomba.x == inimigos[i].x && abs(bomba.y - inimigos[i].y) == 1)
                || (bomba.y == inimigos[i].y && abs(bomba.x - inimigos[i].x) == 1)
                || (bomba.x == inimigos[i].x && bomba.y == inimigos[i].y)){
                inimigos[i].x = -1;
                inimigos[i].y = -1;
                inimigos[i].status = morto;
                Mix_PlayChannel(-1, somInimigoMorto, 0); // coelhinho foi
            }
        }
        bomba.x = -1;
        bomba.y = -1; // Remove a bomba do mapa
    }
}

// Confera se o jogador ganhou
void confere_vitoria()
{
    int contador_inimigos_mortos = 0;
    for (int i = 0; i < numInimigos; i++)
    {
        if(inimigos[i].status == morto)
        {
            contador_inimigos_mortos++;
        }
        if(contador_inimigos_mortos == numInimigos)
        {
            for (int i = 0; i < numInimigos; i++)
            {
                inimigos[i].x = 9;
                inimigos[i].y = 9;
                inimigos[i].status = vivo;
                jogador.x = 1;
                jogador.y = 1;
            }
            statusJogo = vitoria;
            Mix_PlayChannel(-1, somVitoria, 0); // venceu, toca o som feliz
        }
    }
}

// Confere se o jogador perdeu
void confere_derrota()
{
    for (int i = 0; i < numInimigos; i++)
    {
        if (jogador.x == inimigos[i].x && jogador.y == inimigos[0].y)
        {
            for (int i = 0; i < numInimigos; i++)
            {
                inimigos[i].x = 9;
                inimigos[i].y = 9;
                inimigos[i].status = vivo;
                jogador.x = 1;
                jogador.y = 1;
            }
            statusJogo = derrota;
            Mix_PlayChannel(-1, somGameOver, 0); // game over, som triste
            bomba.x = -1;
            bomba.y = -1; // Remove a bomba do mapa
            pontuacao_final = pontuacao_inicial; // Reseta a pontuação
        }
    }
}

// Função auxiliar para renderizar texto
int renderizarTexto(SDL_Renderer* renderizador, TTF_Font* fonte, const char* texto, SDL_Color cor, int x, int y) {
    SDL_Surface* superficie = TTF_RenderText_Solid(fonte, texto, cor);
    if (superficie == NULL) {
        //Erro ao criar superfície de texto
        return 1; //Código de erro a partir daqui só aumenta
    }
    SDL_Texture* textura = SDL_CreateTextureFromSurface(renderizador, superficie);
    if (textura == NULL) {
        //Erro ao criar textura de texto
        SDL_FreeSurface(superficie);
        return 2;
    }
    SDL_Rect destino = {x, y, superficie->w, superficie->h};
    SDL_FreeSurface(superficie);
    SDL_RenderCopy(renderizador, textura, NULL, &destino);
    SDL_DestroyTexture(textura);
}

// Renderiza o menu principal na tela
void exibirMenuPrincipal(SDL_Renderer* renderizador, TTF_Font* fonte) {
    SDL_SetRenderDrawColor(renderizador, 255, 255, 255, 255);
    SDL_RenderClear(renderizador);

    desenharfundo(renderizador, IMG_Load("Props\\menu_principal.png"));

    SDL_RenderPresent(renderizador);
}

// Renderiza o menu de dificuldade na tela
void exibirMenuDificuldade(SDL_Renderer* renderizador, TTF_Font* fonte) {
    SDL_SetRenderDrawColor(renderizador, 255, 255, 255, 255);
    SDL_RenderClear(renderizador);

    desenharfundo(renderizador, IMG_Load("Props\\menu_dificuldades.png"));

    SDL_RenderPresent(renderizador);
}

// Renderiza a tela para inserir o nome do jogador
void exibirInserirNome(SDL_Renderer* renderizador, TTF_Font* fonte) {
    SDL_SetRenderDrawColor(renderizador, 255, 255, 255, 255);
    SDL_RenderClear(renderizador);

    SDL_Color corPreta = {0, 0, 0, 0}; // Preto

    desenharfundo(renderizador, IMG_Load("Props\\menu_inserirnome.png"));

    // Renderiza instruções e o nome digitado
    renderizarTexto(renderizador, fonte, "Digite seu nome:", corPreta, 200, ALTURA_TELA / 2 - 50);
    renderizarTexto(renderizador, fonte, nomeJogador, corPreta, 200, ALTURA_TELA / 2);

    SDL_RenderPresent(renderizador);
}

// Renderiza o menu de pausa na tela
void exibirMenuPausa(SDL_Renderer* renderizador, TTF_Font* fonte) {
    SDL_SetRenderDrawColor(renderizador, 255, 255, 255, 255);
    SDL_RenderClear(renderizador);

    desenharfundo(renderizador, IMG_Load("Props\\menu_jogopausado.png"));

    SDL_RenderPresent(renderizador);
}

// Atualiza a lógica do jogo a cada ciclo
void atualizarJogo() {
    // Diminui a pontuação do jogador por ciclo do jogo
    pontuacao_final--;

    // Atualiza o temporizador da bomba se estiver colocada
    if (bomba.x != -1 && bomba.y != -1) {
        temporizadorBomba++;
    }

    // Detona a bomba se o temporizador atingir 3 segundos
    detonarBomba();

    confere_vitoria(); // Confere se o jogador por acaso ganhou
    confere_derrota(); // Ou se ele perdeu
}

// Salva os dados do jogador no arquivo de texto
int SalvarPlacar() {
    if(salvo == 0){
        salvo = 1;
        // nome que a pessoa digitou; se nao digitou nada, vai como "jogador"
        const char* nome = (strlen(nomeJogador) > 0) ? nomeJogador : "jogador";

        // guarda o ultimo resultado num arquivinho e avisa o navegador pra
        // mandar pro placar global (planilha do google)
        FILE* ult = fopen("ultimo.txt", "w");
        if (ult) { fprintf(ult, "%s\n%d\n", nome, pontuacao_final); fclose(ult); }
#ifdef __EMSCRIPTEN__
        emscripten_run_script("if(window.bcEnviarPontuacao)bcEnviarPontuacao()");
#endif
    }
}

typedef struct {
    char nome[LINHAS];
    int pontuacao;
} Recorde; // Struct pra definir um recorde

// Função pra ordenar o placar
int ordenar_e_salvar(const char *nomeArquivo) {
    FILE *pont_arq = fopen("PlacarJogo.txt", "r");
    if (pont_arq == NULL) {
        // Erro ao abrir o arquivo
        return 4 ;
    }

    Recorde recordes[LINHAS];
    int contador = 0;
    char line[TAMANHO];

    // Lê o arquivo
    while (fgets(line, sizeof(line), pont_arq)) {
        if (sscanf(line, "%[^:]: %d", recordes[contador].nome, &recordes[contador].pontuacao) == 2) {
            contador++;
        }
    }
    fclose(pont_arq);


    // Lógica pra armazenar no arquivo as pontuações em ordem decrescente
    for (int i = 0; i < contador - 1; i++) {
        for (int j = 0; j < contador - 1 - i; j++) {
            if (recordes[j].pontuacao < recordes[j + 1].pontuacao) {
                Recorde temp = recordes[j];
                recordes[j] = recordes[j + 1];
                recordes[j + 1] = temp;
            }
        }
    }

    // Abre o arquivo no modo de escrita para salvar os dados ordenados
    pont_arq = fopen(nomeArquivo, "w");
    if (pont_arq == NULL) {
        //Erro ao abrir o arquivo.
        return 5;
    }

    for (int i = 0; i < contador; i++) {
        fprintf(pont_arq, "%s: %d\n", recordes[i].nome, recordes[i].pontuacao);
    }
    fclose(pont_arq);
}

// Renderiza o menu de placar :p
void exibirMenuPlacar(SDL_Renderer* renderizador, TTF_Font* fonte){
    ordenar_e_salvar("PlacarJogo.txt");
    SDL_Color corTextoTitulo = {0,0,0,255}; // Preto
    SDL_Color corPrimeiro = {0, 0, 0, 255}; // Preto
    SDL_Color corSegundo = {0, 0, 0, 255}; // Preto
    SDL_Color corTerceiro = {0, 0, 0, 255}; // Preto

    FILE* file = fopen("PlacarJogo.txt", "r");
    if (file == NULL)
    {
        // ainda nao tem placar salvo. (nao da pra dar fclose num arquivo nulo,
        // isso travava o jogo no navegador e nao deixava voltar pro menu)
        renderizarTexto(renderizador, fonte, "Ninguem ousou me ganhar ainda!", corTextoTitulo, 125, ALTURA_TELA / 2 - 50);
        renderizarTexto(renderizador, fonte, "Tenta ai e volta mais tarde.", corTextoTitulo, 150, ALTURA_TELA / 2);
        return;
    }

    char lines[3][20];
    int count = 0;  // Contador de linhas

    // Lendo até 3 linhas do arquivo pra compor o placar
    while (fgets(lines[count], 20, file) != NULL && count < 3) {
        // Remove o caractere de nova linha, se houver
        size_t len = strlen(lines[count]);
        if (len > 0 && lines[count][len - 1] == '\n') {
            lines[count][len - 1] = '\0';
        }
        count++;
    }

    fclose(file);

    // Renderiza opções do menu de pausa
    if(count == 0)
    {
        renderizarTexto(renderizador, fonte, "Ninguem ousou me ganhar ainda!", corTextoTitulo, 125, ALTURA_TELA / 2 - 50);
        renderizarTexto(renderizador, fonte, "Tenta ai e volta mais tarde.", corTextoTitulo, 150, ALTURA_TELA / 2);
    }
    if(count>0)
    {
        renderizarTexto(renderizador, fonte, lines[0], corPrimeiro, 160, ALTURA_TELA / 2 );
    }
    if(count>1)
    {
        renderizarTexto(renderizador, fonte, lines[1], corSegundo, 160, ALTURA_TELA / 2 + 50);
    }
    if(count>2)
    {
        renderizarTexto(renderizador, fonte, lines[2], corSegundo, 160, ALTURA_TELA / 2 + 100);
    }
}

int main(int argc, char* args[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
    {
        //Erro ao inicializar o SDL
        return 6;
    }

    if (TTF_Init() == -1)
    {
        //Erro ao inicializar SDL_ttf
        return 7;
    }

    SDL_Window* janela = SDL_CreateWindow("BomberCAT  :)", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, LARGURA_TELA, ALTURA_TELA, SDL_WINDOW_SHOWN);
    if (janela == NULL)
    {
        //Erro ao criar janela
        return 8;
    }

    SDL_Renderer* renderizador = SDL_CreateRenderer(janela, -1, SDL_RENDERER_ACCELERATED);
    if (renderizador == NULL)
    {
        //Erro ao criar renderizador
        return 9;
    }

    TTF_Font* fonte = TTF_OpenFont("Props\\Arial.ttf", 42);
    if (fonte == NULL)
    {
        //Erro ao carregar a fonte
        return 10;
    }

    // abre o audio e carrega os efeitos 8-bit
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
    Mix_Volume(-1, 55); // volume geral dos efeitos bem baixinho
    Mix_Volume(0, 40);  // canal 0 e a musica, mais baixa ainda (de fundo)
    somMenu = Mix_LoadWAV("Props\\sons\\menu.wav");
    somBomba = Mix_LoadWAV("Props\\sons\\bomba.wav");
    somExplosao = Mix_LoadWAV("Props\\sons\\explosao.wav");
    somPasso = Mix_LoadWAV("Props\\sons\\passo.wav");
    somInimigoMorto = Mix_LoadWAV("Props\\sons\\inimigo_morto.wav");
    somVitoria = Mix_LoadWAV("Props\\sons\\vitoria.wav");
    somGameOver = Mix_LoadWAV("Props\\sons\\gameover.wav");
    somDesligar = Mix_LoadWAV("Props\\sons\\desligar.wav");
    musicaMenu = Mix_LoadWAV("Props\\sons\\musica_menu.wav");
    musicaArcade = Mix_LoadWAV("Props\\sons\\musica_arcade.wav");

    inicializarMapa();

    int quit = 0;
    SDL_Event e;
    while (!quit) {
        while (SDL_PollEvent(&e) != 0)
        {
            if (e.type == SDL_QUIT)
            {
                quit = 1;
            }
            else if (statusJogo == naoIniciado)
            {
                if(e.type == SDL_KEYDOWN)
                {
                    Mix_PlayChannel(-1, somMenu, 0); // bipe ao mexer no menu
                    if (menuPrincipal == 0)
                    {
                        switch (e.key.keysym.sym) {
                            case SDLK_1:
                                menuPrincipal = 1;
                                break;
                            case SDLK_2:
                                menuPrincipal = 2;
#ifdef __EMSCRIPTEN__
                                emscripten_run_script("if(window.bcCarregarRanking)bcCarregarRanking()"); // busca o ranking global
#endif
                                break;
                            case SDLK_3:
                                // "sair" no navegador nao fecha um app, entao a gente desliga a tv:
                                // efeito pixelado de desligando + barulhinho de tv velha
                                Mix_HaltChannel(0);                  // corta a musica
                                Mix_PlayChannel(-1, somDesligar, 0); // som de tv desligando
                                desligado = 1;                       // congela o jogo (libera a thread pro efeito)
#ifdef __EMSCRIPTEN__
                                emscripten_run_script("if(window.bcDesligar)bcDesligar()");
#endif
                                break;
                        }
                    }
                    else if (menuPrincipal == 1)
                    {
                        salvo = 0;
                        switch (e.key.keysym.sym)
                        {
                            case SDLK_1:
                                numInimigos = 1;
                                inimigos = realloc(inimigos,numInimigos * sizeof(Entidade));
                                inimigos->x = 9;
                                inimigos->y = 9;
                                inimigos->status = vivo;
                                pontuacao_inicial = 1000;
                                pontuacao_final = pontuacao_inicial;
                                statusJogo = aguardandoNome;
                                SDL_StopTextInput();
                                break;
                            case SDLK_2:
                                numInimigos = 2;
                                inimigos = realloc(inimigos,numInimigos * sizeof(Entidade));
                                for(int i = 0;i<numInimigos;i++){
                                    inimigos[i].x = 9;
                                    inimigos[i].y = 9;
                                    inimigos[i].status = vivo;
                                }
                                pontuacao_inicial = 2000;
                                pontuacao_final = pontuacao_inicial;
                                statusJogo = aguardandoNome;
                                SDL_StopTextInput();
                                break;
                            case SDLK_3:
                                numInimigos = 3;
                                inimigos = realloc(inimigos,numInimigos * sizeof(Entidade));
                                for(int i = 0;i<numInimigos;i++){
                                    inimigos[i].x = 9;
                                    inimigos[i].y = 9;
                                    inimigos[i].status = vivo;
                                }
                                pontuacao_inicial = 3000;
                                pontuacao_final = pontuacao_inicial;
                                statusJogo = aguardandoNome;
                                SDL_StopTextInput();
                                break;
                        }
                    }
                    else if(menuPrincipal == 2)
                    {
                        switch (e.key.keysym.sym)
                        {
                            case SDLK_1:
                                menuPrincipal = 0;
                                break;
                        }
                    }
                }
            }
            else if(statusJogo == aguardandoNome)
            {
                SDL_StartTextInput();
                if(e.type == SDL_TEXTINPUT)
                {
                    strcat(nomeJogador, e.text.text);
                }
                else if(e.type == SDL_KEYDOWN)
                {
                    if (e.key.keysym.sym == SDLK_RETURN)
                    {
                        strcpy(auxNomeJogador,nomeJogador);
                        statusJogo = iniciado;
                        SDL_StopTextInput();
                    }
                    else if(e.key.keysym.sym == SDLK_BACKSPACE && strlen(nomeJogador) > 0)
                    {
                        nomeJogador[strlen(nomeJogador) - 1] = '\0';
                    }
                }
            }
            else if (statusJogo == iniciado)
            {
                if(e.type == SDL_KEYDOWN)
                {
                    switch (e.key.keysym.sym)
                    {
                        case SDLK_UP:
                        case SDLK_w: // wasd igual setinha
                            moverEntidade(&jogador, 0, -1);
                            jogador.ultimoMov = frente;
                            Mix_PlayChannel(-1, somPasso, 0); // passinho
                            break;
                        case SDLK_DOWN:
                        case SDLK_s:
                            moverEntidade(&jogador, 0, 1);
                            jogador.ultimoMov = tras;
                            Mix_PlayChannel(-1, somPasso, 0); // passinho
                            break;
                        case SDLK_RIGHT:
                        case SDLK_d:
                            moverEntidade(&jogador, 1, 0);
                            jogador.ultimoMov = direita;
                            Mix_PlayChannel(-1, somPasso, 0); // passinho
                            break;
                        case SDLK_LEFT:
                        case SDLK_a:
                            moverEntidade(&jogador, -1, 0);
                            jogador.ultimoMov = esquerda;
                            Mix_PlayChannel(-1, somPasso, 0); // passinho
                            break;
                        case SDLK_SPACE:
                            colocarBomba();
                            break;
                        case SDLK_ESCAPE:
                            statusJogo = pausado;
                    }

                }
            }
            else if (statusJogo == pausado)
            {
                if(e.type == SDL_KEYDOWN)
                {
                    switch(e.key.keysym.sym)
                    {
                        case SDLK_1:
                            statusJogo = iniciado;
                            break;
                        case SDLK_2:
                            statusJogo = naoIniciado;
                            menuPrincipal = 0;
                            nomeJogador[0] = '\0';
                            jogador.x = 1;
                            jogador.y = 1;
                            break;
                    }
                }
            }
            else if(statusJogo == vitoria)
            {
                SalvarPlacar();
                if(e.type == SDL_KEYDOWN)
                {
                    switch(e.key.keysym.sym)
                    {
                        case SDLK_1:
                            statusJogo = naoIniciado;
                            nomeJogador[0] = '\0'; //Apaga o nome do jogador
                            menuPrincipal = 0;
                            break;
                    }
                }
            }
            else if(statusJogo == derrota)
            {
                if(e.type == SDL_KEYDOWN)
                {
                    switch (e.key.keysym.sym)
                    {
                        case SDLK_1:
                            statusJogo = iniciado;
                            break;
                        case SDLK_2:
                            statusJogo = naoIniciado;
                            nomeJogador[0] = '\0'; // Apaga o nome do jogador
                            menuPrincipal = 0;
                            break;
                    }
                }
            }
        }

        // tv desligada: para de processar e so cede o controle pro navegador,
        // assim a animacao de desligar roda lisa e o jogo nao gasta cpu a toa
        if (desligado) { SDL_Delay(120); continue; }

        // troca a trilha: arcade agitada no jogo, calminha no resto
        if (statusJogo == iniciado) {
            if (musicaAtual != 1) { Mix_PlayChannel(0, musicaArcade, -1); musicaAtual = 1; }
        } else {
            if (musicaAtual != 0) { Mix_PlayChannel(0, musicaMenu, -1); musicaAtual = 0; }
        }

        SDL_SetRenderDrawColor(renderizador, 255, 255, 255, 255);
        SDL_RenderClear(renderizador);

        if (menuPrincipal == 0 && statusJogo == naoIniciado)
        {
            exibirMenuPrincipal(renderizador, fonte);
        }
        else if (menuPrincipal == 1 && statusJogo == naoIniciado)
        {
            exibirMenuDificuldade(renderizador, fonte);
        }
        else if (menuPrincipal == 2 && statusJogo == naoIniciado)
        {
            desenharfundo(renderizador, IMG_Load("Props\\menu_placar.png"));
            exibirMenuPlacar(renderizador, fonte);
        }
        else if (statusJogo == aguardandoNome)
        {
            exibirInserirNome(renderizador, fonte);
        }
        else if (statusJogo == pausado)
        {
            exibirMenuPausa(renderizador, fonte);
        }
        else if(statusJogo == iniciado)
        {
            atualizarJogo();
            desenharfundo(renderizador, IMG_Load("Props\\fundo.jpg"));
            desenharMapa(renderizador);
            desenharJogador(renderizador, &jogador);
            for (int i = 0; i < numInimigos; i++) {
                desenharInimigo(renderizador, &inimigos[i]); // Renderiza os inimigos
            }
            if (bomba.x != -1 && bomba.y != -1) {
                desenharBomba(renderizador, &bomba); // Renderiza a bomba
            }
            if(temporizadorBomba == 9)
            {
                desenharBombaExplodida(renderizador, &bomba);
            }
            mover_inimigos();

            SDL_RenderPresent(renderizador);
            SDL_Delay(100); // Define o tempo de "processamento"
        }
        else if (statusJogo == derrota)
        {
            desenharfundo(renderizador, IMG_Load("Props\\tela_perdeu.png"));
        }
        else if (statusJogo == vitoria)
        {
            SalvarPlacar(); // salva o placar assim que ganha (uma vez so, garantido)
            SDL_Color corfim = {255, 255, 255, 0}; // Preto
            char valor[100];
            if(strcmp(nomeJogador, "raissa") == 0){
                desenharfundo(renderizador, IMG_Load("Props\\tela_ganhou_raissa.png"));
            }
            else{
                desenharfundo(renderizador, IMG_Load("Props\\tela_ganhou.png"));
            }

            snprintf(valor, sizeof(valor), " Pontuacao: %d", pontuacao_final);
            renderizarTexto(renderizador, fonte, valor, corfim, 210, ALTURA_TELA/2 + 80);
        }

        SDL_RenderPresent(renderizador);
    }

    TTF_CloseFont(fonte);
    fonte = NULL;

    SDL_DestroyRenderer(renderizador);
    SDL_DestroyWindow(janela);
    janela = NULL;
    renderizador = NULL;

    // libera os sons
    Mix_FreeChunk(somMenu);
    Mix_FreeChunk(somBomba);
    Mix_FreeChunk(somExplosao);
    Mix_FreeChunk(somPasso);
    Mix_FreeChunk(somInimigoMorto);
    Mix_FreeChunk(somVitoria);
    Mix_FreeChunk(somGameOver);
    Mix_FreeChunk(somDesligar);
    Mix_FreeChunk(musicaMenu);
    Mix_FreeChunk(musicaArcade);
    Mix_CloseAudio();

    TTF_Quit();
    SDL_Quit();
    free(inimigos);


    return 11;
}