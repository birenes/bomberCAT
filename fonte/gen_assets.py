# gera os assets do bombercat: textura de tijolo cotton candy, moldura de tv
# pixelada e os sons 8-bit (tudo procedural, nada baixado)
import os, wave
import numpy as np
from PIL import Image, ImageDraw

BASE = os.path.dirname(os.path.abspath(__file__))
PROPS = os.path.join(BASE, "Props")
SONS = os.path.join(PROPS, "sons")
os.makedirs(SONS, exist_ok=True)

# ---------------------------------------------------------------------------
# textura de tijolinho - rosa cotton candy, 32x32 e tileavel
# ---------------------------------------------------------------------------
W = H = 32
unit_w = 16        # largura do tijolo + rejunte (2 tijolos por bloco)
unit_h = 8         # altura do tijolo + rejunte (deitadinho)
mortar = 2
half = unit_w // 2
img = np.zeros((H, W, 3), dtype=np.uint8)
mortar_col = np.array([255, 241, 248])   # rejunte quase branco rosado
top_col = np.array([255, 192, 216])      # topo do tijolo (algodao doce claro)
bot_col = np.array([247, 152, 190])      # base do tijolo (rosinha mais forte)
for y in range(H):
    row = y // unit_h
    ry = y % unit_h
    offset = half if (row % 2) else 0
    for x in range(W):
        rx = (x + offset) % unit_w
        if ry < mortar or rx < mortar:
            img[y, x] = mortar_col
        else:
            f = (ry - mortar) / (unit_h - mortar)
            img[y, x] = np.clip(top_col * (1 - f) + bot_col * f, 0, 255)
Image.fromarray(img).save(os.path.join(PROPS, "tijolos.png"))
print("tijolos.png ok")

# ---------------------------------------------------------------------------
# moldura de tv pixelada (preto e branco, retro). desenhada em baixa resolucao
# e ampliada com nearest no html. tem um buraco transparente pra tela do jogo.
# ---------------------------------------------------------------------------
TW, TH = 184, 156
tv = Image.new("RGBA", (TW, TH), (0, 0, 0, 0))
d = ImageDraw.Draw(tv)
INK = (24, 24, 24, 255)        # contorno escuro
BODY = (210, 210, 210, 255)    # corpo cinza claro
BODY_D = (176, 176, 176, 255)  # painel / sombra
SHINE = (242, 242, 242, 255)   # brilho do contorno interno
KNOB = (198, 198, 198, 255)

# antena em V com boliquinhas nas pontas e base trapezio (menorzinha)
d.line([(92, 36), (68, 12)], fill=INK, width=3)
d.line([(92, 36), (116, 12)], fill=INK, width=3)
d.ellipse([63, 7, 73, 17], fill=INK)
d.ellipse([111, 7, 121, 17], fill=INK)
d.polygon([(84, 30), (100, 30), (96, 41), (88, 41)], fill=INK)

# corpo arredondado, contorno duplo (escuro por fora, brilho por dentro)
d.rounded_rectangle([6, 36, 178, 148], radius=15, fill=BODY, outline=INK, width=3)
d.rounded_rectangle([11, 41, 173, 143], radius=11, outline=SHINE, width=2)

# tela 4:3 grande a esquerda; bezel preto arredondado (buraco vem depois)
SCR = (18, 48, 130, 132)   # x0,y0,x1,y1 -> 112x84 = 4:3
d.rounded_rectangle([SCR[0]-5, SCR[1]-5, SCR[2]+5, SCR[3]+5], radius=9, fill=INK)

# painel lateral com 3 botoes redondos + tracinhos (que nem a referencia)
d.rounded_rectangle([138, 48, 172, 132], radius=8, fill=BODY_D, outline=INK, width=2)
for cy in (63, 85, 107):
    d.ellipse([146, cy - 9, 164, cy + 9], fill=KNOB, outline=INK, width=2)
    d.line([(155, cy), (161, cy - 6)], fill=INK, width=2)   # ponteirinho
for k in range(3):
    d.rectangle([146, 119 + k * 4, 164, 120 + k * 4], fill=INK)  # tracinhos

# fura a tela (deixa transparente pra ver o jogo por tras)
for yy in range(SCR[1], SCR[3]):
    for xx in range(SCR[0], SCR[2]):
        tv.putpixel((xx, yy), (0, 0, 0, 0))

tv.save(os.path.join(BASE, "moldura_tv.png"))
# geometria da tela em % (pro html posicionar o canvas em cima do buraco)
print("MOLDURA_GEOM left=%.4f top=%.4f w=%.4f h=%.4f" % (
    SCR[0] / TW * 100, SCR[1] / TH * 100,
    (SCR[2] - SCR[0]) / TW * 100, (SCR[3] - SCR[1]) / TH * 100))
print("moldura_tv.png ok (%dx%d)" % (TW, TH))

# ---------------------------------------------------------------------------
# sintese de audio 8-bit
# ---------------------------------------------------------------------------
SR = 44100

def save_wav(name, sig, gain=1.0):
    sig = np.clip(sig * gain, -1.0, 1.0)
    pcm = (sig * 32767).astype("<i2")
    with wave.open(os.path.join(SONS, name), "wb") as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR)
        w.writeframes(pcm.tobytes())
    print(name, "ok")

def f(name):  # nome -> frequencia. ex: 'C4', 'A#3'
    base = {'C': 0, 'D': 2, 'E': 4, 'F': 5, 'G': 7, 'A': 9, 'B': 11}
    i = 1
    sharp = 0
    if name[1] == '#':
        sharp = 1; i = 2
    midi = 12 * (int(name[i:]) + 1) + base[name[0]] + sharp
    return 440.0 * (2 ** ((midi - 69) / 12.0))

def square(freq, dur, duty=0.5, vol=0.5):
    n = int(SR * dur)
    ph = (np.arange(n) / SR * freq) % 1.0
    return np.where(ph < duty, 1.0, -1.0) * vol

def triangle(freq, dur, vol=0.5):
    n = int(SR * dur)
    ph = (np.arange(n) / SR * freq) % 1.0
    return (2 * np.abs(2 * ph - 1) - 1) * vol

def env(n, attack=0.004, release=0.04):
    a = min(int(SR * attack), n // 2)
    r = min(int(SR * release), n // 2)
    e = np.ones(n)
    if a > 0: e[:a] = np.linspace(0, 1, a)
    if r > 0: e[n - r:] = np.linspace(1, 0, r)
    return e

def tone(name, dur, duty=0.5, vol=0.5, tri=False, atk=0.004, rel=0.04):
    s = triangle(f(name), dur, vol) if tri else square(f(name), dur, duty, vol)
    return s * env(len(s), atk, rel)

def rest(dur):
    return np.zeros(int(SR * dur))

# volume geral dos efeitos baixinho (ela reclamou que tava alto)
SFX = 0.42

# menu: bipe curtinho subindo
save_wav("menu.wav", np.concatenate([
    tone("E5", 0.03, 0.5, 0.6), tone("A5", 0.06, 0.5, 0.6)]), gain=SFX)

# passo: tiquinho grave, bem discreto
passo = square(150, 0.045, duty=0.25, vol=0.5) * env(int(SR*0.045), 0.002, 0.02)
save_wav("passo.wav", passo, gain=SFX * 0.5)

# bomba plantada: queda de tom
n = int(SR * 0.18); t = np.arange(n) / SR
sweep = np.linspace(520, 150, n)
ph = np.cumsum(sweep) / SR
bomba = np.where((ph % 1.0) < 0.5, 1.0, -1.0) * 0.6 * env(n, release=0.05)
save_wav("bomba.wav", bomba, gain=SFX)

# explosao: estouro de ruido com cauda grave
n = int(SR * 0.5)
rng = np.random.default_rng(7)
noise = rng.random(n) * 2 - 1
decay = np.exp(-np.linspace(0, 6, n))
rumble = triangle(64, 0.5, 1.0) * np.exp(-np.linspace(0, 4, n))
explosao = (noise * 0.55 + rumble * 0.5) * decay
save_wav("explosao.wav", explosao, gain=SFX)

# coelhinho morto: "pop" descendente fofo
morto = np.concatenate([
    tone("A5", 0.05, 0.25, 0.6), tone("E5", 0.05, 0.25, 0.55),
    tone("A4", 0.09, 0.25, 0.5)])
n = int(SR * 0.06); poof = (rng.random(n) * 2 - 1) * np.exp(-np.linspace(0, 7, n)) * 0.4
morto = np.concatenate([morto, poof])
save_wav("inimigo_morto.wav", morto, gain=SFX)

# vitoria: fanfarra alegre subindo
save_wav("vitoria.wav", np.concatenate([
    tone("C5", 0.1, 0.5, 0.6), tone("E5", 0.1, 0.5, 0.6),
    tone("G5", 0.1, 0.5, 0.6), tone("C6", 0.12, 0.5, 0.6),
    tone("G5", 0.08, 0.5, 0.55), tone("C6", 0.26, 0.5, 0.65)]), gain=SFX)

# game over: descida triste
save_wav("gameover.wav", np.concatenate([
    tone("E5", 0.14, 0.5, 0.6), tone("D5", 0.14, 0.5, 0.58),
    tone("C5", 0.14, 0.5, 0.56), tone("A4", 0.16, 0.5, 0.54),
    tone("A3", 0.34, 0.5, 0.6, tri=False)]), gain=SFX)

# tv desligando: zunido caindo rapido + estalinho de estatica (crt antiga)
n = int(SR * 0.5)
sweep = np.linspace(1500, 70, n)
ph = np.cumsum(sweep) / SR
desligar = np.where((ph % 1.0) < 0.5, 1.0, -1.0) * 0.55 * np.exp(-np.linspace(0, 5, n))
nb = int(SR * 0.05)
desligar[:nb] += (rng.random(nb) * 2 - 1) * 0.4  # tiquinho de estatica no inicio
save_wav("desligar.wav", desligar, gain=SFX)

# ---------------------------------------------------------------------------
# musica do menu: animadinha mas tranquila (C - G - Am - F), ~108 bpm
# ---------------------------------------------------------------------------
def seq(notes, duty=0.5, vol=0.5, tri=False, bpm=108):
    beat = 60.0 / bpm
    parts = []
    for name, beats in notes:
        d = beat * beats
        parts.append(rest(d) if name is None else tone(name, d, duty, vol, tri, rel=0.05))
    return np.concatenate(parts)

lead_menu = seq([
    ("G4", 0.5), ("C5", 0.5), ("E5", 0.5), ("G5", 0.5),
    ("E5", 0.5), ("C5", 0.5), ("D5", 1.0),
    ("B4", 0.5), ("D5", 0.5), ("G5", 0.5), ("B5", 0.5),
    ("A5", 0.5), ("E5", 0.5), ("C5", 1.0),
    ("A4", 0.5), ("C5", 0.5), ("E5", 0.5), ("A5", 0.5),
    ("G5", 0.5), ("E5", 0.5), ("C5", 1.0),
    ("F4", 0.5), ("A4", 0.5), ("C5", 0.5), ("F5", 0.5),
    ("E5", 0.5), ("C5", 0.5), ("G4", 1.0),
], duty=0.3, vol=0.34)
bass_menu = seq([
    ("C3", 1), ("C3", 1), ("G3", 1), ("G3", 1),
    ("A3", 1), ("A3", 1), ("F3", 1), ("F3", 1),
    ("C3", 1), ("C3", 1), ("G3", 1), ("G3", 1),
    ("A3", 1), ("A3", 1), ("F3", 1), ("F3", 1),
], vol=0.4, tri=True)
m = min(len(lead_menu), len(bass_menu))
musica_menu = lead_menu[:m] + bass_menu[:m]
fa = int(SR * 0.02)
musica_menu[:fa] *= np.linspace(0, 1, fa); musica_menu[-fa:] *= np.linspace(1, 0, fa)
save_wav("musica_menu.wav", musica_menu, gain=0.55)

# ---------------------------------------------------------------------------
# musica de arcade: rapida e tensa (Am - Am - Dm - E), ~152 bpm, baixo pulsante
# ---------------------------------------------------------------------------
arp = seq([
    ("A4", 0.5), ("C5", 0.5), ("E5", 0.5), ("C5", 0.5),
    ("A4", 0.5), ("C5", 0.5), ("E5", 0.5), ("G5", 0.5),
    ("D5", 0.5), ("F5", 0.5), ("A5", 0.5), ("F5", 0.5),
    ("E5", 0.5), ("G#5", 0.5), ("B5", 0.5), ("E5", 0.5),
], duty=0.5, vol=0.3, bpm=152)
# baixo em colcheias staccato (pulso de arcade)
bass_arc = seq([
    ("A2", 0.5)] * 8 + [("D3", 0.5)] * 4 + [("E3", 0.5)] * 4,
    duty=0.5, vol=0.42, bpm=152)
m = min(len(arp), len(bass_arc))
musica_arcade = arp[:m] + bass_arc[:m]
fa = int(SR * 0.015)
musica_arcade[:fa] *= np.linspace(0, 1, fa); musica_arcade[-fa:] *= np.linspace(1, 0, fa)
save_wav("musica_arcade.wav", musica_arcade, gain=0.55)

print("\ntudo gerado.")
