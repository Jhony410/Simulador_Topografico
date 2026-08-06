# Controles de GeoDrone

La simulación arranca **directamente en vuelo**: no hay menú inicial que atravesar.
Durante unos segundos aparece la pista `WASD PARA INICIAR LA EXPLORACIÓN` y se
desvanece sola.

## Pilotaje

| Entrada | Acción |
|---|---|
| W/A/S/D | Avanzar, izquierda, retroceder, derecha respecto a la cámara |
| Flechas | Equivalentes a W/A/S/D |
| Espacio | Ascender |
| Shift izquierdo/derecho | Descender |
| Arrastre con botón izquierdo | Orbitar la cámara |
| Rueda | Acercar/alejar (zoom geométrico, limitado por la escala del mapa) |
| F | Recentrar la cámara detrás del dron |
| Esc | Abrir el menú de configuración |

El dron **solo se mueve mientras hay una tecla pulsada**. Al soltarlas frena con
desaceleración limitada y se detiene por completo: la velocidad se pone a cero
exacto y la posición deja de cambiar. No hay flotación, oscilación ni deriva.

## Vistas

| Tecla | Acción |
|---|---|
| C | Seguimiento → orbital libre → inspección superior |
| V | Rotar los seis modos de visualización del terreno |
| M | Mostrar/ocultar minimapa y porcentaje |
| H | Curvas de nivel |
| L | Guía hacia la zona de sondeo más cercana |
| F1 | Mostrar/ocultar la lista de controles |

## Menú de configuración (Esc)

`Esc` (o `P`) abre el menú. El simulador se detiene mientras está abierto.

```
GEODRONE
CONFIGURACION

  CONTINUAR
  TERRENO              < SnowTerrain 1/5 >
  REINICIAR ESCANEO
  AYUDA EN PANTALLA    < SI >
  SALIR

W/S MOVER   FLECHAS CAMBIAR VALOR   ENTER CONFIRMAR   ESC VOLVER
```

| Fila | Qué hace |
|---|---|
| CONTINUAR | Vuelve al vuelo |
| TERRENO | Izquierda/derecha eligen mapa; **Enter lo carga**. La carga solo ocurre al confirmar, porque algunos GLB tienen más de un millón de vértices |
| REINICIAR ESCANEO | Borra la exploración sin recargar el terreno: el mapa se vuelve a oscurecer y el porcentaje regresa a 0 % |
| AYUDA EN PANTALLA | Muestra/oculta la lista de controles |
| SALIR | Cierra la aplicación |

Navegación: `W`/`S` o flechas arriba/abajo mueven la selección; `A`/`D` o
flechas izquierda/derecha cambian el valor de las filas que lo tienen; `Enter`
confirma; `Esc` vuelve al vuelo.

## Interfaz y ventana

| Tecla | Acción |
|---|---|
| **F3** | Mostrar/ocultar **toda** la información técnica (FPS, coordenadas, altitud, velocidad, escalas, planos de recorte, celdas) |
| **F11** | Alternar ventana / pantalla completa (restaura tamaño y posición previos) |
| F1 | Mostrar/ocultar la ayuda de controles |

Por defecto la pantalla solo muestra: `GEODRONE` arriba a la izquierda, el
minimapa con su porcentaje abajo a la izquierda y la tabla de controles abajo a
la derecha. Todo lo técnico está oculto tras F3.

## Mapas

| Tecla | Acción |
|---|---|
| Tab / Backspace | Mapa siguiente / anterior |
| 1…9 | Selección directa |
| R | Reiniciar el escaneo del mapa actual |
| F5 / F9 | Guardar / recuperar máscara de exploración y posición del dron |

Cambiar de mapa o reiniciar el escaneo **reinicia el progreso**: la máscara
vuelve a cero, el terreno se oscurece de nuevo y el porcentaje arranca desde 0 %.

## Exploración y niebla de guerra

El mapa arranca **a oscuras**: no se dibuja el relieve que todavía no has
escaneado. Solo se ve lo que cubre el radar del dron en este momento y todo lo
que ya cubrió antes, porque la máscara es acumulativa.

El escaneo está **siempre activo mientras vuelas**: no hay que mantener ninguna
tecla. El haz de luz sale de la panza del dron, termina en la superficie del
relieve y su huella marca como exploradas las celdas de la rejilla que sobrevuela.
El porcentaje es la cobertura real (celdas exploradas / celdas válidas del
terreno) y coincide exactamente con lo que se revela en el minimapa.

Las torretas de sondeo tampoco se dibujan sobre terreno sin explorar: aparecen
cuando el radar llega a ellas.

## Límite del área

El dron **no puede salir del terreno**. Se detiene a una distancia de seguridad
del canto de la malla (3,5 % de la diagonal del mapa) y la cámara se mantiene
también dentro de esa caja, de modo que la órbita no acaba asomada al vacío.

Las zonas de sondeo (anillos sobre el terreno) son hitos opcionales: al
completarlas revelan de golpe el área que las rodea. Para completar una, mantén
el dron dentro de su anillo, por encima de la altura mínima de seguridad y con
rapidez moderada.

## Medición

1. Pulsa `X`.
2. Clic izquierdo sobre el terreno para añadir puntos.
3. Dos puntos producen distancia, desnivel, pendiente y ángulo.
4. Tres o más producen área proyectada.
5. Clic derecho limpia.
6. `X` vuelve a pilotaje normal.

Mientras el modo está activo, el clic izquierdo selecciona y no orbita.
