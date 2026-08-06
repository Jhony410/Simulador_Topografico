# Pruebas de GeoDrone

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
