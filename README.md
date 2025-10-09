# Proyecto1_Paralela_2

---

Compilar con:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Correr con:

```bash
./build/lavalamp -n 'number of molecules' -w 'width' -h 'height'
```