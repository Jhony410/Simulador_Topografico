# Registro de cambios

## 2026-08-06 (b) — Límite del mapa, niebla de guerra, radar y menú

### 1. Límite del área

- `EscalaMundo::margenMapa` de 0,004 a **0,035 × diagonal** (~4,9 u). Antes el
  margen era casi nulo y el dron llegaba al canto de la malla.
- `Camara::evitarTerreno` se convierte en `Camara::confinarAlTerreno`: además de
  mantenerse sobre el relieve, recorta su XZ a la caja del terreno. Sin esto la
  órbita sacaba la cámara fuera del mapa y se veía el vacío, que es justo lo que
  mostraba la captura de referencia.

### 2. Niebla de guerra real

- `shaders/terreno.frag`: se elimina el suelo `max(0.16, …)`. Lo no escaneado ya
  no se dibuja (`discard`), así que el mapa arranca **a oscuras**.
- El halo del dron pasa a derivarse del radar (`uRadioRevelado`) en vez de
  `uRadioNitido`/`uRadioDesvanecido`, que abarcaban media diagonal y revelaban
  el mapa gratis. Esos dos uniformes quedan solo como atenuación estética de lo
  ya explorado.
- `shaders/marcadores.frag`: una torreta en zona sin escanear se descarta. Antes
  habrían quedado antenas flotando sobre el vacío negro.
- `VistaExploracion`: las guías de zona de sondeo solo se dibujan si su celda
  está explorada o el radar la alcanza.
- **Corregido de paso**: `SistemaExploracion` revelaba `punto.radio * 2.5` al
  completar una zona. Con el radar ya doblado eso son 28,6 unidades, es decir un
  cuarto del mapa de golpe — medido: 27 % de cobertura con el dron inmóvil en el
  arranque. Ahora es `punto.radio * 1.15` y el arranque queda en ~6 %.

### 3. Radar doblado

- `radioEscaneo` de 0,045 a **0,090 × diagonal** (6,4 → 12,7 u): cuadruplica la
  superficie cubierta por pasada.
- `CELDAS_POR_FRAME` de 1024 a 2048, porque la cola inicial crece con el área.
- El cono de luz sigue el mismo tope relativo (`radioEscaneo * 0.55`), así que
  crece con el radar sin necesidad de retocarlo aparte.

### 4. Ayuda en pantalla

- La lista de controles pasa de una columna corrida a una **tabla tecla/acción**
  de 11 filas con rótulo `CONTROLES`, fondo semitransparente y filete amarillo.
  Antes era texto gris sobre la rejilla y se perdía.
- Bloque ensanchado de 176 a 250 px para que quepa la acción más larga.

### 5. Menú de configuración

- Nuevo `include/MenuConfiguracion.h`: índices y etiquetas de las filas,
  compartidos por Controlador y Vista para que no se desincronicen.
- `Esc` (o `P`) abre el menú; el simulador se detiene. Filas: CONTINUAR,
  TERRENO (con selector de mapa), REINICIAR ESCANEO, AYUDA EN PANTALLA, SALIR.
- El cambio de terreno solo se carga al confirmar con Enter, no al mover la
  selección: hay GLB de más de un millón de vértices.
- Nuevo `Escena::reiniciarEscaneo()`: vacía máscara, cola del escáner, zonas de
  sondeo y curvas **sin recargar el terreno ni mover el dron**. Mucho más barato
  que `cargarMapa()`. La tecla `R` pasa a usarlo.
- `Esc` ya no cierra la aplicación directamente: salir es ahora una fila del
  menú. Es un cambio respecto a la revisión anterior, pedido explícitamente.

## 2026-08-06 — Escala del mundo, vuelo estable y escáner

Revisión centrada en la proporción dron/terreno, la estabilidad del pilotaje y
la reducción de la interfaz. Referencias de proporción, cámara en tercera
persona y luz inferior tomadas solo como concepto; no se copió interfaz,
recursos ni identidad visual de ningún simulador.

### Nuevo: `EscalaMundo` (WorldScaleConfig)

`include/EscalaMundo.h` + `src/EscalaMundo.cpp`. Se recalcula en cada
`Escena::cargarMapa()` a partir de la diagonal del terreno normalizado y de la
diagonal del modelo del dron. De ahí salen: escala del dron, velocidad máxima,
aceleración, desaceleración, velocidad vertical, epsilon de zona muerta, altura
segura y máxima, altura inicial, margen del mapa, distancia y límites de cámara,
planos near/far, radio de escaneo, altura y ancho de los marcadores y los radios
de atenuación radial.

Antes esos valores eran constantes sueltas en `Configuracion.h` calibradas para
un solo mapa. `Configuracion.h` ahora solo guarda lo adimensional
(proporciones, resoluciones, semillas, ritmos, colores, rutas).

### Escala

- `TAMANIO_DRON` pasa de 5.0 a 1.0: la malla se normaliza a extensión 1 y la
  escala real la aplica `ComponenteTransformada::escala` por mapa. Un mismo GLB
  sirve para terrenos de tamaños distintos.
- Proporción resultante: dron ≈ 0,29–0,31 unidades sobre terrenos de diagonal
  132–141, es decir 0,0022 constante en los cinco mapas.
- `RESOLUCION_REJILLA` de 129 a 257: con la cámara mucho más cerca del relieve,
  la cuadrícula anterior se leía gruesa. El quadtree sigue descartando por
  frustum, así que el coste real apenas sube (90–115 FPS medidos).

### Movimiento (`Dron`)

- Modelo nuevo: entrada → velocidad objetivo → aproximación con **aceleración
  limitada** (`tasa * dt` por frame) → `posición += velocidad * dt`.
  A diferencia del lerp exponencial anterior, llega exactamente al objetivo y al
  cero en tiempo finito, que es lo que elimina el deslizamiento residual.
- Zona muerta: por debajo de `epsilonVelocidad` la velocidad se pone a cero exacto.
- **Eliminada la flotación automática** (`sin(tiempoVuelo)`), que era la causa de
  que el dron subiera y bajara solo al detenerse.
- El clamp de bordes ya no rebota (`velocidad *= -0.15`): anula el eje.
- Corrección de terreno solo cuando el dron está por debajo de la altura mínima.
- Inclinación con interpolación exponencial (nunca sobrepasa) y fijada a cero
  bajo 0,05° sin entrada.
- El giro de las hélices sigue viviendo solo en `shaders/dron.vert`: no toca la
  transformada del cuerpo.
- Sensibilidad de mouse por defecto de 0,30 a 0,10.

### Marcadores

- `MARCADOR_PROB_FLOTANTE` y `MARCADOR_FLOTE_MAX` **eliminadas**: eran las que
  levantaban el 18 % de las estacas hasta 22 unidades sobre el suelo.
- La base es siempre `terreno.alturaEn(x, z)` y la geometría se construye desde
  la base hacia arriba, de modo que el pivote coincide con el apoyo.
- Geometría de torreta (mástil, cuatro tirantes, dos crucetas, luz mínima) en
  lugar de un poste con billboard grande.
- Altura = 0,0045 × diagonal del terreno (≈0,6 u), ancho = 6 % de la altura.
- Estado (no explorado gris / bajo radar ámbar / explorado cian) resuelto en el
  fragment shader muestreando la máscara: no cambia el tamaño físico ni obliga a
  resubir el VBO.

### Nuevo: luz de escaneo (`VistaEscaner`)

`include/VistaEscaner.h`, `src/VistaEscaner.cpp`, `shaders/escaner.vert|frag`.
Cono semitransparente desde la panza del dron, huella elíptica que sigue la
altura real del relieve y anillos de radar que se expanden perdiendo alpha.
Mezcla aditiva sin escritura de profundidad y polygon offset en la huella.
La altura del haz es `dron.y - alturaEn(dron.xz)`, así que termina exactamente
en la superficie. Activo automáticamente mientras se vuela.

### Exploración y minimapa

- **Corregido un fallo real**: `MapaExploracion` escribía el byte `1` en las
  celdas exploradas, pero esa misma máscara se sube como textura `GL_R8`
  (formato normalizado), así que el shader leía `1/255 ≈ 0.004`. Por eso el
  minimapa nunca podía revelar nada y el shader original solo podía atenuar.
  Ahora se escribe `MapaExploracion::MARCADA = 255`.
- `Terreno` expone `obtenerCobertura()`: qué celdas recibieron un vértice real
  antes de rellenar huecos. `MapaExploracion` usa esa cobertura como denominador,
  así que el porcentaje no cuenta terreno inexistente y el 100 % es alcanzable.
- El porcentaje del HUD pasa a ser la cobertura real de la máscara, no el
  progreso de los puntos de sondeo.
- Minimapa: el shader oculta lo no explorado (azul casi negro) en vez de
  atenuarlo, y revela relieve y curvas de nivel con `smoothstep` sobre la
  máscara filtrada en `GL_LINEAR`. Rampa topográfica azul → verde → amarillo →
  naranja.
- La textura de máscara se asigna una vez por mapa y después solo se reescribe
  con `glTexSubImage2D`, y únicamente cuando cambia la revisión del Modelo.

### Cámara

- Distancia, mínimos, máximos, altura sobre el suelo y planos near/far salen de
  `EscalaMundo`. Distancia = 0,040 × diagonal.
- FOV de 45° a 58°, elevación inicial 24° (ligeramente por encima del dron, para
  ver a la vez el aparato y el terreno que barre).
- Zoom geométrico (porcentaje del radio) en vez de incremento fijo.
- Seguimiento de yaw más lento, para que girar no produzca latigazo lateral.

### Interfaz

- Se eliminan de la pantalla principal: panel de misión, panel lateral de
  informe, botones de mapa, botón de salida, icono de menú, pie de proyecto,
  clúster de teclas, panel de medición fijo, alertas y etiqueta de modo visual.
- Queda: `GEODRONE` + subtítulo arriba a la izquierda; minimapa, porcentaje y
  barra fina abajo a la izquierda; lista de controles abajo a la derecha.
- **F3** alterna un panel con toda la información técnica, oculto por defecto.
- Se elimina el menú inicial: la aplicación entra directamente en vuelo con una
  pista discreta que se desvanece.
- Completar el mapa ya no abre una pantalla modal: se anuncia una vez y se sigue
  volando.
- `DisenoHUD` se reduce a los tres rectángulos que siguen existiendo.

### Ventana

- Se abre al 95 % × 92 % de la zona de trabajo del monitor principal y se centra
  (`glfwGetMonitorWorkarea`), en lugar de 1280×720 fijos.
- **F11** alterna pantalla completa con `glfwSetWindowMonitor`, memorizando
  posición y tamaño previos y recalculando el viewport.
- El callback de framebuffer ignora tamaños 0×0 (minimizado).

### Pruebas

`simulador.exe --pruebas-modelo` añade tres comprobaciones automáticas por mapa:
velocidad exactamente cero tras frenar, posición idéntica tras un segundo sin
entrada y proporción dron/terreno dentro de rango.

## 2026-08-05 — GeoDrone

### Arquitectura

- Se conservó MVC y los loaders existentes.
- Se añadieron `Ajustes`, `EstadoAplicacion`, `SistemaExploracion`, `PuntoEscaneo`, `SistemaMedicion`, `VistaExploracion`, `VistaMinimapa` y wrappers `RecursoGL`.
- `main.cpp` incluye validaciones CLI sin OpenGL.
- `compilar.bat` soporta rutas con espacios, reporta falta de g++ y no ejecuta salvo argumento `ejecutar`.

### Visual

- Paleta propia GeoDrone centralizada.
- Fondo degradado, partículas, viñeta, fog y glow.
- MSAA 4× compatible con postproceso mediante resolve.
- Seis modos de terreno, superficie iluminada, elevación, puntos, contornos y revelado.
- Dron amarillo con aristas emisivas y hélices GPU.

### Interacción

- Aceleración, freno, estabilización, inclinación y flotación.
- Límites del mapa y altura relativa segura.
- Cámara orbital, seguimiento y superior; recentrado y colisión con terreno.
- Diez zonas reproducibles con anillos, pulso, estados y progreso por deltaTime.
- Ruta al objetivo, cobertura por textura y resumen de misión.
- Minimapa de altura/cobertura y herramientas de medición por ray casting.

### Interfaz

- Inicio, pausa, configuración y misión completada.
- HUD con telemetría, progreso, puntos, cobertura, modo visual/cámara, alertas y datos topográficos.

### Robustez

- Compilación/enlace de shaders falla de forma explícita.
- Validación de índices OBJ.
- Corrección del conteo de segmentos CSV.
- Programas y shaders cacheados con RAII move-only.

### Documentación

- README reescrito.
- Añadidos `IMPLEMENTACION.md`, `CONTROLES.md`, `PRUEBAS.md` y este registro.

## Decisiones deliberadas

- No se usan `difuso.png` ni `especular.png`: no son mapas adecuados para UV/PBR.
- No se agregó audio por falta de biblioteca ligera existente.
- No se sustituyeron modelos ni loaders.
- No se añadió CMake como requisito; MinGW y `compilar.bat` siguen siendo la vía principal.
