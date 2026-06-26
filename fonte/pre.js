// ----------------------------------------------------------------------------
// ponte de caminhos pro navegador - isso aqui nao faz parte do jogo (main.c intocado)
//
// o jogo abre os arquivos no estilo windows: "Props\\areia.jpg". no sistema de
// arquivos virtual do navegador eles ficam em "Props/areia.jpg". entao antes do
// jogo rodar a gente cria um "apelido" (symlink) com barra invertida pra cada
// arquivo, apontando pro caminho de verdade com barra normal. o symlink e
// preguicoso: nao precisa que o alvo ja exista pra criar, so resolve na hora de
// abrir. assim o fopen/IMG_Load/TTF_OpenFont/Mix_LoadWAV acha tudo certinho.
// ----------------------------------------------------------------------------
Module['preRun'] = Module['preRun'] || [];
Module['preRun'].push(function () {
  var assets = [
    "Props/Arial.ttf",
    "Props/Dog/direita.png",
    "Props/Dog/esquerda.png",
    "Props/Dog/frente.png",
    "Props/Dog/tras.png",
    "Props/areia.jpg",
    "Props/bomba.png",
    "Props/bombaexplodida.png",
    "Props/direita.png",
    "Props/esquerda.png",
    "Props/frente.png",
    "Props/fundo.jpg",
    "Props/inimigo_baixo.png",
    "Props/inimigo_cima.png",
    "Props/inimigo_direita.png",
    "Props/inimigo_esquerda.png",
    "Props/menu_dificuldades.png",
    "Props/menu_inserirnome.png",
    "Props/menu_jogopausado.png",
    "Props/menu_placar.png",
    "Props/menu_principal.png",
    "Props/sons/bomba.wav",
    "Props/sons/desligar.wav",
    "Props/sons/explosao.wav",
    "Props/sons/gameover.wav",
    "Props/sons/inimigo_morto.wav",
    "Props/sons/menu.wav",
    "Props/sons/musica_arcade.wav",
    "Props/sons/musica_menu.wav",
    "Props/sons/passo.wav",
    "Props/sons/vitoria.wav",
    "Props/tela_ganhou.png",
    "Props/tela_ganhou_raissa.png",
    "Props/tela_perdeu.png",
    "Props/tijolos.png",
    "Props/tras.png"
  ];
  assets.forEach(function (real) {
    var bs = real.replace(/\//g, '\\');   // ex: Props\Dog\frente.png
    try { FS.symlink(real, bs); } catch (e) { /* ja existe, ignora */ }
  });
});
