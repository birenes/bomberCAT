/* ---------------------------------------------------------------------------
 * ponte pro loop principal rodar no navegador - isso nao faz parte do jogo.
 *
 * o main.c tem um while(1) infinito (que nem todo jogo de desktop). no navegador
 * isso travaria a aba, porque o javascript precisa devolver o controle pro
 * browser de vez em quando pra desenhar e ler o teclado.
 *
 * em vez de mexer no loop do jogo, a gente intercepta via linker (--wrap) as
 * duas funcoes que o jogo chama a cada quadro e faz elas "cederem" o controle
 * pro navegador usando asyncify. o main.c continua igualzinho.
 * ------------------------------------------------------------------------- */
#include <SDL.h>
#include <emscripten.h>

extern void __real_SDL_RenderPresent(SDL_Renderer *r);
extern void __real_SDL_Delay(Uint32 ms);

/* chamado uma vez por iteracao do loop, em todas as telas (ate nos menus, que
 * nao tem SDL_Delay). cede o controle pro navegador a cada quadro. */
void __wrap_SDL_RenderPresent(SDL_Renderer *r) {
    __real_SDL_RenderPresent(r);
    emscripten_sleep(0);
}

/* no jogo, o main.c chama SDL_Delay(100) (ritmo de ~10 fps). em vez de travar a
 * aba por 100ms, dorme de verdade devolvendo o controle pro browser. */
void __wrap_SDL_Delay(Uint32 ms) {
    emscripten_sleep(ms);
}
