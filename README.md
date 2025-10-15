# Lava Lamp Screensaver (C++)

Simulación tipo *lámpara de lava* en SDL2 con partículas y clusters. El flujo reproduce un ciclo estable: **subida por el centro caliente**, **desviación en la parte superior hacia los lados**, **descenso por rampas laterales** y **reencauce hacia el centro**.

> Este README documenta la versión con **foco cilíndrico estático** (calor mayor cerca del eje vertical) y controles de paleta/fondo.

---

## Demo rápida

Compilar:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Ejecutar:

```bash
./build/lavalamp -n 260 -w 1024 -h 768 -L 0.6 -p red
```

Parámetros principales:
- `-n` cantidad de moléculas (partículas base).
- `-w` ancho de ventana (mín. 640).
- `-h` alto de ventana (mín. 480).
- `-L` intensidad de luz de fondo [0..1].
- `-p` paleta (`red`, `orange`, `green`, `blue`, `purple`, `rainbow`).

---

## Controles en tiempo real

- `C` → cambia a la siguiente **paleta**.
- `A` → alterna **auto‑cambio de paleta** on/off.
- `Q` → hace **más rápido** el auto‑cambio.
- `E` → hace **más lento** el auto‑cambio.
- `B` → alterna **fondo** entre negro/blanco.
- `Esc` → salir.

---

## ¿Qué hay bajo el capó?

### Flujo general
1. **Faro de calor central**: un *cilindro vertical estático* centrado en la pantalla. La temperatura depende casi sólo de la **distancia horizontal al eje**; cuanto más cerca del eje, más caliente. Entre un **tope** y un **fondo** del cilindro se aplica calor, conservando la forma fija.
2. **Subida por convección**: las partículas dentro de la columna reciben empuje vertical. Muy cerca del eje hay un refuerzo para romper la inercia.
3. **Techo curvo**: en la parte superior hay un “arco” que desvía el flujo a izquierda/derecha y amortigua la subida.
4. **Bajada lateral**: en los bordes laterales se empuja **hacia abajo** y un poco **hacia el centro** para que el material vuelva a la zona caliente.
5. **No‑caída por el centro**: si una partícula intenta **bajar** por el eje, se le aplica un empuje lateral para obligarla a tomar las rampas.
6. **Brillo por temperatura**: el color base depende de la paleta y el **brillo** se escala con el exceso de temperatura de cada partícula respecto al entorno; cerca del foco se ve más luminoso.

### Archivos clave
- `src/Molecule.*` – Partículas individuales (posición, velocidad, temperatura, volumen/radio).
- `src/Blob.*` – Clusters suaves para dar aspecto de masas.
- `src/LavaLamp.*` – Física del flujo, foco cilíndrico, techo, rampas, colisiones y *spawning* en el centro.
- `src/Renderer.*` – SDL2, paletas, luz de fondo, brillo dependiente del calor y controles.

### Parámetros relevantes (resumen)
> Todos están en `LavaLamp.h` y se pueden ajustar para cambiar el carácter del flujo.

- **Columna**: `columnHalfWidth`, `columnEdgeFeather`, `columnTopFactor`, `columnBottomFactor`, `columnHeatBonus`.
- **Subida central**: `updraftPush`, `centerLiftMinUp`, `hotJitterX`.
- **Techo**: `archBaseYFactor`, `archSlope`, `archPushSide`, `archPushDown`.
- **Rampas**: `rampStartYFactor`, `basinYFactor`, `rampHalfSpanX`, `rampCurvePower` (parabólico), `rampCenterKick`.
- **No‑caída centro**: `centerNoFallSidePush`.
- **Salida superior**: `topOutflowSidePush`.
- **Laterales**: `sideDownPush`, `sideBandWidth`.
- **Render**: paletas en `Palette.*`, luz con `Renderer::setLightIntensity` y brillo ligado al calor.

---

## Instalación de dependencias (Ubuntu/WSL)

```bash
sudo apt update
sudo apt install -y build-essential cmake libsdl2-dev
```

En WSL, usa un servidor X (p. ej. X410, GWSL o VcXsrv) y exporta `DISPLAY` si es necesario:
```bash
export DISPLAY=:0
```

---

## Ejemplos de uso

Más partículas y resolución FullHD:
```bash
./build/lavalamp -n 600 -w 1920 -h 1080 -L 0.7 -p orange
```

Modo presentación con auto‑cambio de paleta: pulsa `A` y luego ajusta la velocidad con `Q`/`E`.

Fondo blanco para contraste fotográfico: pulsa `B`.

---

## Estructura del proyecto

```
.
├── CMakeLists.txt
├── src
│   ├── main.cpp
│   ├── Molecule.{h,cpp}
│   ├── Blob.{h,cpp}
│   ├── Palette.{h,cpp}
│   ├── LavaLamp.{h,cpp}
│   └── Renderer.{h,cpp}
└── README.md
```

---

## Ajustes recomendados

- Si el flujo no alcanza el techo, aumenta `updraftPush` o `columnHeatBonus`.
- Si suben por los lados, baja `outsideUpwardDampen` y sube `sideDownPush`.
- Si se atascan arriba, incrementa `topOutflowSidePush`.
- Si aún caen por el centro, sube `centerNoFallSidePush`.

---

## Licencia

Uso académico/educativo.
