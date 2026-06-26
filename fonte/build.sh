#!/usr/bin/env bash
# ---------------------------------------------------------------------------
# recompila o bombercat pra webassembly: gera um unico bomberCAT.html que roda
# no navegador (jogo + assets + moldura de tv tudo embutido).
#
# pre-requisito: emscripten ativo no shell, ex.:
#   git clone https://github.com/emscripten-core/emsdk.git
#   cd emsdk && ./emsdk install latest && ./emsdk activate latest
#   source ./emsdk_env.sh
#
# rode de dentro da pasta fonte/.
# ---------------------------------------------------------------------------
set -euo pipefail
cd "$(dirname "$0")"

# (opcional) regera tijolo, moldura e sons. precisa de python com pillow e numpy.
# python3 gen_assets.py

# compila pra um unico .js com wasm + assets embutidos
emcc main.c yield_shim.c -o bombercat.js \
  -O2 \
  -sUSE_SDL=2 \
  -sUSE_SDL_IMAGE=2 \
  -sSDL2_IMAGE_FORMATS='["png","jpg"]' \
  -sUSE_SDL_TTF=2 \
  -sUSE_SDL_MIXER=2 \
  -sASYNCIFY=1 \
  -sALLOW_MEMORY_GROWTH=1 \
  -sEXIT_RUNTIME=0 \
  -sSINGLE_FILE=1 \
  --embed-file Props \
  --pre-js pre.js \
  -Wl,--wrap=SDL_RenderPresent \
  -Wl,--wrap=SDL_Delay

# junta a casca (head + js + tail) e inlina a moldura da tv como data-uri,
# produzindo o arquivo unico em ../bomberCAT.html
python3 - <<'PY'
import base64
uri = 'data:image/png;base64,' + base64.b64encode(open('moldura_tv.png','rb').read()).decode()
head = open('head.html').read().replace('__MOLDURA_TV__', uri)
out = head + open('bombercat.js').read() + open('tail.html').read()
open('../bomberCAT.html','w').write(out)
print('pronto: ../bomberCAT.html')
PY
