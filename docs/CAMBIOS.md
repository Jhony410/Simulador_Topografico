# Registro de cambios

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
