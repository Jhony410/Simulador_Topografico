# Implementación técnica de GeoDrone

## Pipeline de renderizado

Cada frame sigue este orden:

1. GLFW actualiza teclas, ratón, zoom y acciones discretas.
2. `Escena` integra el dron, consulta el heightmap, actualiza exploración, zonas y transformaciones.
3. `Renderizador` construye el frustum y consulta quadtree/BVH.
4. Terreno, balizas, zonas, mediciones y dron se dibujan en un FBO 4× MSAA con dos renderbuffers de color.
5. Ambos attachments se resuelven a texturas: color normal y emisión.
6. La emisión se reduce a media resolución, filtra y desenfoca en ping-pong.
7. `composicion.frag` suma glow, degradado, partículas y viñeta.
8. Minimapa y HUD se dibujan al final sin depth test ni bloom.

## VAO, VBO y EBO

Un VAO conserva la descripción de atributos. Un VBO contiene vértices; un EBO contiene índices reutilizables. La rejilla del terreno comparte posiciones entre líneas, puntos y superficie. El quadtree concatena índices de sus nodos en un EBO y cada nodo visible se dibuja mediante offset/cantidad.

El dron comparte un VBO `[x,y,z,nx,ny,nz,idHelice]` entre un EBO triangular y otro de aristas características. Las normales se calculan por vértice tras el skinning; el identificador permite que el vertex shader rote solo la hélice correspondiente.

Las vistas de exploración y minimapa mantienen VBO dinámicos y llaman `glBufferData`/`glBufferSubData` sobre objetos existentes; no crean buffers por frame.

`RecursoGL.h` define recursos move-only para buffers, VAO, texturas, framebuffer, renderbuffer, shader y programa. `GestorRecursos` usa RAII para programas y shaders cacheados. Las vistas heredadas conservan liberación explícita y se destruyen antes de terminar GLFW.

## Transformaciones Model, View y Projection

- Model: la entidad vuelca posición, yaw, pitch, roll y escala a su nodo.
- View: `glm::lookAt(posicion, objetivo, up)` desde `Camara`.
- Projection: perspectiva de 45°, aspecto vigente y planos 0,1–1500.

El grafo propaga `mundo = padre × local`. Las hélices tienen nodos hijos, aunque el giro visible de sus vértices se ejecuta en shader usando pivotes locales.

## Carga y normalización del terreno

`Terreno::cargarDesdeArchivo` delega por extensión. Después:

1. calcula bounding box;
2. detecta el eje vertical como el de menor extensión;
3. reorienta a Y-up;
4. centra y escala el eje horizontal mayor a 100 unidades;
5. guarda límites normalizados;
6. construye heightmap, rejilla y quadtree.

Para CSV se mantiene topología de líneas y no se informa un número falso de triángulos.

## Heightmap

Es un `vector<float>` aplanado 256×256. Cada vértice contribuye el máximo Y de su celda. Los huecos se rellenan iterativamente con vecinos conocidos para evitar pozos. `alturaEn(x,z)` transforma mundo a celda y hace interpolación bilineal de cuatro alturas.

Usos: dron, cámara, zonas, anillos, ruta, ray casting, minimapa y métricas.

## Curvas de nivel

En superficie se normaliza la altura y se generan catorce bandas procedurales con `fract`/`smoothstep`. En minimapa se repite el método sobre la textura normalizada. Los módulos `marchingSquares`, `GeneradorCurvas` y `GrafoCurvas` se conservan para isolíneas geométricas incrementales.

## Sistema de exploración

Hay dos escalas:

- cobertura continua: `MapaExploracion` y `SistemaEscaneo` revelan celdas bajo el dron;
- objetivos: `SistemaExploracion` mantiene zonas con progreso temporal y estados.

La generación usa `mt19937` con semilla fija, margen del 8 %, rechazo de pendiente y distancia mínima. El objetivo actual es el incompleto más cercano.

Una textura `GL_R8` representa la máscara. Su revisión aumenta solo al cambiar una celda; las vistas evitan resubir si la revisión no cambió.

## Cálculo de progreso

Progreso de misión:

```text
suma(progreso de cada zona) / número de zonas
```

Cobertura:

```text
celdas exploradas / celdas totales
```

Ambos son reales. El HUD muestra puntos completos y cobertura; la misión termina cuando todos los objetivos están completos.

## Movimiento del dron

El controlador fija una velocidad objetivo. `Dron::actualizar` usa:

```text
t = 1 - exp(-constante * deltaTime)
velocidad = mix(velocidad, objetivo, t)
posición += velocidad × deltaTime
```

Hay constantes distintas para aceleración y frenado. Yaw usa el arco mínimo; pitch depende de rapidez y roll de la componente lateral. Al detenerse, pitch/roll convergen a cero y aparece flotación senoidal pequeña.

Tras integrar se limitan X/Z y se consulta altura. Y se corrige a `suelo + ALTURA_MINIMA`, garantizando que nunca permanezca bajo la superficie.

## Cámara

La posición y el foco se interpolan con `1 - base^dt`, independiente de FPS. Seguimiento interpola yaw más lentamente para producir retraso cinematográfico. Orbital conserva control directo; superior coloca la cámara casi vertical. Al final se eleva contra el heightmap si fuera necesario.

## Shaders

- `terreno`: altura, normales, exploración, seis modos, fog y bandas.
- `dron`: rotación de hélices y MRT para relleno/aristas emisivas.
- `exploracion`: líneas, anillos, ruta y puntos de medición.
- `marcadores`: billboards y atenuación radial.
- `minimapa`: heightmap, cobertura, contornos e iconos.
- `hud`: geometría 2D y texto.
- postproceso: brillo, blur y composición.

`GestorRecursos` cachea por ruta, informa errores de compilación/enlace y no devuelve programas inválidos.

## Minimap

Alturas y máscara son texturas independientes. Las posiciones se convierten con:

```text
u = (x - minX) / (maxX - minX)
v = (z - minZ) / (maxZ - minZ)
```

Los iconos se generan en píxeles, comparten un VBO dinámico y se dibujan agrupados como líneas/puntos.

## Mediciones

El clic se convierte a NDC y se desproyecta con la inversa de `Projection × View`. El rayo se recorre en intervalos; al cambiar de encima a debajo del heightmap se refina por bisección.

Para dos puntos A/B:

```text
horizontal = length((Bx-Ax, Bz-Az))
desnivel   = By-Ay
3D         = length(B-A)
pendiente  = desnivel / horizontal × 100
ángulo     = atan2(desnivel, horizontal)
```

Para tres o más, el área XZ usa la fórmula del cordón.

## Gestión de recursos y errores

La aplicación destruye vistas/FBO/programas antes de destruir la ventana y terminar GLFW. Cada cambio de mapa elimina buffers reemplazados antes de crear otros. Se validan GLFW, ventana, GLAD, shader, enlace, framebuffer, archivo, OBJ, CSV, GLB, dron y heightmap.

Los mensajes tienen categorías `[INIT]`, `[SHADER]`, `[TERRAIN]`, `[HEIGHTMAP]`, `[SCAN]`, `[MISSION]`, `[TEST]` y `[ERROR]`; no se imprime por frame.

## Decisiones sobre texturas y audio

`difuso.png` es una fotografía del edificio y `especular.png` un gráfico, no mapas adecuados para UV/PBR. No se conectaron. Los modelos sin UV siguen usando color. Tampoco se agregó audio: no había biblioteca disponible y una dependencia nueva no mejora el objetivo gráfico principal.
