# Screensaver OpenMP — UVG

Visualizador de partículas con líneas entre puntos cercanos. Tiene **dos modos**: secuencial (SEQ) y paralelo (PAR usando OpenMP). La meta del paralelo es **acelerar la construcción de aristas** (líneas) cuando el radio de conexión es razonable y hay muchas partículas.

---

## 🧱 Estructura del proyecto

```
include/
  App.h
  Args.h
  Color.h
  Particle.h
  Timer.h
src/
  App.cpp
  Args.cpp
  Color.cpp
  Particle.cpp
  Timer.cpp
  main.cpp
CMakeLists.txt
```

---

## 🔧 Compilación

Requisitos (Linux/WSL recomendado):
- CMake ≥ 3.16
- g++ (C++17)
- SDL2 (headers y libs). En Debian/Ubuntu: `sudo apt install libsdl2-dev`
- OpenMP (opcional pero recomendado). En g++ viene con `-fopenmp`

Comandos:

```bash
rm -rf build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

> 💡 Si tu toolchain tiene OpenMP, CMake lo detecta y compila con soporte PAR automáticamente. Si no, el binario funciona igual, pero el modo `--par` caerá internamente a SEQ.

---

## ▶️ Ejecución

Ejemplos:

```bash
# Secuencial (por defecto)
./build/omp_screensaver -n 1200 -r 90 -s 1.0 --seq

# Paralelo (OpenMP)
./build/omp_screensaver -n 1200 -r 90 -s 1.0 --par

# Paralelo forzando hilos (si quieres)
./build/omp_screensaver -n 1600 -r 60 -s 1.0 --par --threads 8
```

Parámetros útiles:
- `-n <int>`: número de partículas. (def. 400)
- `-r <float>`: radio de conexión. (def. 120)
- `-s <float>`: velocidad base de partículas. (def. 1.0)
- `--seq` / `--par`: modo secuencial o paralelo.
- `--threads <K>`: fija K hilos de OpenMP (opcional).
- `--seed <int>`: semilla RNG (opcional).
- `--bench <0/1>`: 1 = no dibuja, solo calcula (útil para medir cómputo puro).

---

## 🎮 Controles en tiempo real

- `ESC`: salir
- `SPACE`: pausar / continuar
- `F1`..`F4`: paletas (Neon, Sunset, Aqua, Candy)
- `C`: **Auto-cambio** de paleta ON/OFF (activado en esta versión)
- `R`: rotación del enjambre (OFF → CW → CCW → OFF)
- `↑ / ↓`: subir/bajar radio
- `← / →`: bajar/subir velocidad
- `B`: fondo negro ↔ blanco

La barra de título muestra: modo (PAR/SEQ), N, r, velocidad, auto-ciclo (C), FPS y si está en **BENCH**.

---

## 🧠 ¿Qué se paraleliza?

- **Construcción de aristas (“edges”)**: por cada celda de un grid espacial revisamos pares de partículas **solo** dentro de la misma celda y sus **4** vecinas (derecha, abajo-derecha, abajo, abajo-izquierda).  
- En **PAR**, **cada hilo** procesa un subconjunto de celdas y guarda sus aristas en un **vector local**; al final **se concatenan** todos los vectores. Así **evitamos peleas** por el mismo `edges_` y escalamos mejor.

> Movimiento + rotación de partículas se hace en bloque (secuencial) para mantener el código simple; el cuello de botella real está en la detección de vecinos (no en el movimiento).

---

## 🧭 Cómo funciona el grid (resumen rápido)

- Partimos la pantalla en una **malla** de celdas cuadradas de tamaño `cellSize ≈ radius`.  
- Mapeamos cada partícula a su celda `(cx, cy)`.  
- Construimos tres arreglos planos:
  - `cellCounts_`: cuántas partículas cayeron en cada celda.
  - `cellOffsets_`: prefijos acumulados para saber dónde empieza cada celda dentro de `cellItems_`.
  - `cellItems_`: índices de partículas **ordenados por celda**.

Esto permite, para una celda dada, recorrer sus partículas como un **segmento** contiguo de `cellItems_` en O(1).

---

## 📈 Cuándo se nota el speedup

- Se nota **más** cuando `radius` es **medio/pequeño** (por ejemplo `r ≈ 40–80` con `n` alto).  
- Si el radio es **muy grande**, casi todas las partículas se “ven”, así que el trabajo vuelve a ser casi O(N²) y los beneficios del grid se diluyen (PAR ayuda menos).  
- Si `n` es muy bajo, el overhead de paralelizar puede opacar el beneficio.

> En nuestras pruebas: con `n ≥ 1200` y `r ≈ 50–90`, PAR suele superar a SEQ, especialmente si **no** estamos limitados por VSync o por el **render**.

---

## 🧩 División de trabajo (sugerida para el informe)

- **Joaquín – Movimiento & Grid**
  - Lógica de `Particle::update` y rebotes.
  - Construcción del grid (`rebuildGridSequential`).
  - Verificación de offsets y layout de `cellItems_`.

- **Nelson – Detección de vecinos & Aristas (SEQ)**
  - Búsqueda de vecinos por celda y 4 vecinas.
  - Cálculo de distancia y peso `w = 1 - d²/r²`.
  - Limpieza y reserva de `edges_` + mejoras de legibilidad (nombres descriptivos).

- **Gabriel – Paralelización (PAR) & UX del demo**
  - División por celdas con OpenMP, “bolsitas por hilo” y merge final.
  - Flags `--par/--seq/--threads` y controles (`C`, `B`, `←/→`, `↑/↓`, `R`).
  - Pruebas con distintos `n`, `r` y documentación.

---

## ❓Preguntas guía (resumen para el profe)

- **¿Qué paralelizamos?** La construcción de aristas por celda (misma + 4 vecinas), repartiendo celdas entre hilos y acumulando resultados localmente para luego fusionarlos.
- **Obstáculos al paralelizar:** evitar contención en `edges_`, balancear trabajo (algunas celdas tienen más puntos), mantener código entendible.
- **Decisión de diseño:** mantener el movimiento simple y atacar el cuello real (vecinos). Paralelismo **coarse-grain** por celdas + **concatenación** final para mínima sincronización.

---

## 🛠️ Troubleshooting rápido

- **`stray '#pragma'` o errores con OpenMP:** asegúrate de compilar con soporte (`-fopenmp`). Con CMake debería verse en el log “OpenMP found”.
- **`undefined reference to App::run()`**: suele ser porque no se compiló/ linkeó `App.cpp`. Revisa que esté en `add_executable(...)` del `CMakeLists.txt`.
- **Se congela con `--bench 1`:** en bench no dibuja; si bloquea, verifica el loop o argumentos (usa `--bench 0` para el demo visual).

---

## 📜 Licencia y créditos
Proyecto académico para **UVG – Computación Paralela y Distribuida**. Hecho por: **Gabriel, Joaquín y Nelson**.
