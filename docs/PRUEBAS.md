# Pruebas de GeoDrone

## Revisión 2026-08-06 (b) — límite, niebla de guerra, radar y menú

Verificado en ejecución sobre `SnowTerrain.obj` y `calles_puno.csv`.
`compilar.bat` termina con código 0; `--pruebas-modelo` y `--validar-assets`
devuelven `RESULTADO: OK`.

| Cambio | Estado | Evidencia |
|---|---|---|
| 1. El dron no sale del terreno | Aprobada | Manteniendo `W` 16 s (≈120 u sobre un mapa de 100) el dron para en `POS Z 43.33` con `VELOCIDAD 0.000`, a 4,86 u del canto. La cámara tampoco sale de la caja |
| 2. El mapa arranca a oscuras | Aprobada | Captura inicial: solo el disco del radar alrededor del dron; el horizonte es negro. `EXPLORADO 6 %` frente al 27 % del intento anterior |
| 2b. El mapa aparece al moverse | Aprobada | El rastro explorado se dibuja detrás del dron y permanece; el minimapa muestra el corredor recorrido |
| 3. Radar al doble | Aprobada | Panel F3: `RADAR 12.50` (antes 6,25). Cobertura 28 % → 41 % en una sola pasada de 16 s |
| 4. Controles visibles | Aprobada | Tabla de 11 filas tecla/acción con rótulo, fondo semitransparente y filete amarillo, abajo a la derecha |
| 5. Menú de configuración | Aprobada | `Esc` abre el panel; `S` mueve a TERRENO; `D` cambia `SnowTerrain 1/5` → `calles_puno 3/5`; `Enter` carga (título de ventana pasa a `calles_puno.csv [3/5]`) |

### Nota sobre la inyección de teclado en estas pruebas

Dos falsos negativos que conviene registrar para no perseguirlos otra vez:

1. `SetForegroundWindow` desde un proceso en segundo plano lo bloquea Windows.
   Se comprobó con `GetForegroundWindow()` que la ventana **no** tenía el foco;
   hace falta pulsar y soltar `ALT` antes para levantar el bloqueo.
2. `keybd_event` con `bScan = 0` no llega a GLFW para todas las teclas, porque
   en Windows GLFW resuelve la tecla por **scancode**. Hay que pasar el scancode
   real con `MapVirtualKey(vk, 0)`. Con `Enter` esto hacía parecer que el menú
   no aplicaba el cambio de mapa cuando en realidad sí lo hacía.

`TAB` sigue sin poder verificarse por inyección (Windows lo intercepta antes);
requiere pulsación manual. Usa la misma ruta de código que las teclas numéricas,
que sí están verificadas.

---

## Revisión 2026-08-06 — escala, vuelo, escáner, minimapa e interfaz

Entorno: Windows 11, MinGW-w64 UCRT g++, OpenGL del equipo local.
Todo lo de esta sección se ejecutó realmente; lo que no se pudo comprobar está
marcado como tal.

### Automatizadas

```bat
compilar.bat
simulador.exe --pruebas-modelo
```

`compilar.bat` termina con código 0 tras añadir `src/EscalaMundo.cpp` y
`src/VistaEscaner.cpp`. `--pruebas-modelo` devuelve `RESULTADO: OK` en los cinco
mapas, con tres comprobaciones nuevas por terreno:

1. **Velocidad cero tras frenar.** Se acelera 1 s con entrada, se sueltan las
   teclas 3 s y se exige `obtenerRapidez() == 0.0f` exacto (no aproximado).
2. **Inmovilidad sin entrada.** Un segundo más sin entrada y se exige
   `length(posición - posiciónReposo) == 0.0f` exacto.
3. **Proporción dron/terreno** dentro de 0,0005–0,006.

Escalas calculadas por mapa (traza `[ESCALA]`):

| Mapa | Diagonal | Dron (u) | Ratio | V.máx | Cámara | Radar | Estaca |
|---|---:|---:|---:|---:|---:|---:|---:|
| SnowTerrain.obj | 141,3 | 0,311 | 0,0022 | 7,77 | 5,65 | 6,36 | 0,64 |
| cabo_tinoso…glb | 135,0 | 0,297 | 0,0022 | 7,42 | 5,40 | 6,07 | 0,61 |
| calles_puno.csv | 138,9 | 0,306 | 0,0022 | 7,64 | 5,56 | 6,25 | 0,62 |
| ciudad_universitaria…glb | 141,5 | 0,311 | 0,0022 | 7,78 | 5,66 | 6,37 | 0,64 |
| modelo_digital…glb | 132,7 | 0,292 | 0,0022 | 7,30 | 5,31 | 5,97 | 0,60 |

La proporción es idéntica en los cinco pese a diagonales distintas.

### Verificación en ejecución

Se lanzó `simulador.exe`, se pilotó por inyección de teclado y se capturó
pantalla. Evidencia recogida del panel F3 y de las capturas:

| # | Comprobación | Estado | Evidencia |
|---|---|---|---|
| 1 | El dron se ve pequeño frente al terreno | Aprobada | ocupa ~3,5 % del alto de pantalla |
| 2 | La proporción funciona en todos los mapas | Aprobada | tabla `[ESCALA]`, ratio 0,0022 constante |
| 3 | La cámara lo deja ver sin agrandarlo | Aprobada | distancia 5,3–5,7 según mapa, no se tocó la escala |
| 4 | Movimiento inicial no hipersensible | Aprobada | V.máx 7,8 u/s sobre mapa de 100 u (antes 50) |
| 5 | Acelera gradualmente | Aprobada | aceleración limitada a 2,2 × V.máx por segundo |
| 6 | Frena correctamente | Aprobada | desaceleración 3,4 × V.máx |
| 7 | Queda inmóvil sin entrada | Aprobada | F3: `VELOCIDAD 0.000`; test automático exige `== 0` |
| 8 | Sin desplazamiento residual | Aprobada | test automático de posición idéntica |
| 9 | Sin flotación automática | Aprobada | `flotacion` eliminada del Modelo y de `Escena::actualizar` |
| 10 | No atraviesa el terreno | Aprobada | test de colisión vertical en los 5 mapas |
| 11 | Los marcadores tocan el suelo | Aprobada | base = `alturaEn(x,z)`; código de flotación eliminado |
| 12 | Marcadores pequeños | Aprobada | 0,60–0,64 u frente a 2,5–6,5 anteriores |
| 13 | Ninguno llega al cielo | Aprobada | `MARCADOR_FLOTE_MAX` eliminada |
| 14 | La luz nace bajo el dron | Aprobada | captura: vértice del cono en la panza |
| 15 | La luz termina en el terreno | Aprobada | captura: huella elíptica sobre el relieve |
| 16 | Los anillos se animan | Aprobada por código y captura | fase por `tiempo/PERIODO_ANILLO`, alpha decreciente |
| 17 | La exploración sube al visitar zonas nuevas | Aprobada | 7 % → 10 % volando; minimapa revela el corredor |
| 18 | No sube estando quieto | Aprobada | `marcarCelda` devuelve false si la celda ya estaba |
| 19 | El minimapa empieza oculto | Aprobada | captura inicial: solo rejilla y triángulo del dron |
| 20 | Revela el terreno progresivamente | Aprobada | captura tras volar: mancha topográfica irregular |
| 21 | El porcentaje coincide con la máscara | Aprobada | ambos salen de `porcentajeExplorado()` |
| 22 | Cambiar de mapa reinicia el progreso | Aprobada | 8 % → 1 % al pasar a `modelo_digital…` |
| 23 | La ventana abre casi a pantalla completa | Aprobada | 95 % × 92 % de la zona de trabajo, centrada |
| 24 | F11 funciona | Aprobada | captura en pantalla completa, HUD recolocado |
| 25 | HUD solo con lo esencial | Aprobada | marca, minimapa, porcentaje y controles |
| 26 | F3 muestra/oculta lo técnico | Aprobada | capturas con y sin panel |
| 27 | `compilar.bat` sigue funcionando | Aprobada | exit code 0 |
| 28 | Sin errores GLSL | Aprobada | `recursos.tieneErrores()` aborta el arranque; la app arranca |
| 29 | Sin recursos recreados por frame | Aprobada por código | máscaras con `glTexSubImage2D` tras asignar una vez; VBO del escáner y de iconos con capacidad fija y `glBufferSubData`; guardias por número de revisión |
| 30 | Liberación correcta | Aprobada por inspección | `VistaEscaner` usa `RecursoGL` RAII; `VistaMarcadores` borra su textura; `liberar()` antes de `glfwTerminate`. No se ejecutó un detector externo de fugas de GPU |

Rendimiento medido con F3: 88–115 FPS, 53–118 draw calls, 16 976–22 592 de
131 584 segmentos enviados (el quadtree descarta el resto).

**Nota sobre `TAB`:** el cambio de mapa se verificó con las teclas numéricas
`3` y `5`, que usan exactamente la misma ruta de código tres líneas más arriba
en `ControladorEntrada::manejarTecla`. `TAB` no se pudo verificar por inyección
de teclado porque Windows lo intercepta antes de llegar a la ventana; requiere
pulsación manual.

### Caso límite verificado: mapa de líneas

`calles_puno.csv` (terreno plano de segmentos) funciona con el resto de mejoras:
`CELDAS 491 / 5069` confirma que la máscara de cobertura restringe el
denominador del porcentaje a las celdas donde realmente hay calles, en vez de
usar las 65 536 de la rejilla completa.

---

## Revisión anterior

Fecha: 2026-08-05. Entorno: Windows, MinGW g++ 15.2.0, OpenGL del equipo local.

## Automatizadas

### Compilación

Comando:

```bat
compilar.bat
```

Resultado: aprobado. Generó `simulador.exe` con C++17 y todas las unidades nuevas.

### Recursos

```bat
simulador.exe --validar-assets
```

Resultado: aprobado.

| Recurso | Vértices | Triángulos/segmentos |
|---|---:|---:|
| SnowTerrain.obj | 4.225 | 8.192 triángulos |
| cabo_tinoso_modelo3d_acb-dem.glb | 94.887 | 183.200 triángulos |
| calles_puno.csv | 47.894 | 35.615 segmentos |
| ciudad_universitaria_unam (1).glb | 1.363.290 | 454.430 triángulos |
| modelo_digital_do_terreno_-_chapada_diamantina.glb | 201.372 | 397.424 triángulos |
| animated_drone.glb | 59.334 | 83.712 triángulos |

El dron produjo 35.137 aristas características.

### Modelo

```bat
simulador.exe --pruebas-modelo
```

Resultado: aprobado en los cinco mapas. Para cada terreno se generaron las diez zonas de la misión, se mantuvo el dron en cada una durante tiempo simulado y se alcanzó misión completa. También se comprobó que un dron colocado fuera/debajo vuelve a límites seguros y que la distancia entre (0,0,0) y (3,4,0) es 5.

## Prueba gráfica

Se ejecutó `simulador.exe`, se inició con Enter y se cambiaron los mapas 1–5 mediante teclado. La captura final mostró el quinto terreno, dron, rejilla, minimapa, HUD, objetivo y panel de medición. Los shaders cargaron y la aplicación continuó respondiendo.

La apertura final verificó el FBO MSAA y los shaders sin error visible. La aplicación se cerró normalmente enviando Escape desde `Intro` y el proceso terminó. La liberación se verificó además por inspección de las rutas RAII/`liberar()`; no se ejecutó un detector externo de fugas de GPU.

## Matriz de aceptación

| Prueba | Estado | Evidencia |
|---|---|---|
| Compila con `compilar.bat` | Aprobada | exit code 0 |
| Abre ventana / shaders | Aprobada | capturas y prueba gráfica |
| Todos los terrenos cargan | Aprobada | `--validar-assets` |
| Dron y hélices | Aprobada visualmente | GLB visible; shader/pivotes cargados |
| Movimiento suave | Implementada; prueba manual parcial | dinámica por deltaTime |
| Cámara, órbita y zoom | Aprobada en ejecución base | callbacks y captura |
| Colisión con terreno | Aprobada automáticamente | `--pruebas-modelo` |
| Puntos y progreso | Aprobada automáticamente | diez zonas en aplicación y test |
| Minimapa | Aprobada visualmente | captura desde heightmap |
| Cambio de mapa reinicia | Aprobada por código y humo | mapas 1–5 |
| Pausa/configuración | Implementada; inicio probado | navegación por teclado |
| Misión completa/resumen | Lógica aprobada automáticamente | pantalla depende del mismo estado |
| Reinicio | Implementado | recarga mapa y subsistemas |
| Liberación OpenGL | Aprobada por ruta y cierre normal | RAII + `liberar()` antes de `glfwTerminate` |

## Pruebas manuales recomendadas

Antes de exposición: completar una misión real; medir tres puntos; probar rueda/arrastre en las tres cámaras; recorrer V seis veces; guardar/cargar; redimensionar a 16:9 y 4:3; cerrar mediante menú y mediante X; observar consola sin `[ERROR]`.
