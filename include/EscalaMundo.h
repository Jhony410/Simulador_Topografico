#pragma once
#include <glm/glm.hpp>

class Terreno;

// ============================================================================
//  MODELO: escala derivada del terreno activo (WorldScaleConfig).
//
//  Todo lo que depende del "tamano del mundo" (velocidad del dron, distancia
//  de camara, altura de las estacas, radio del radar, planos de recorte) se
//  calcula AQUI a partir de la diagonal del terreno ya normalizado, y no con
//  constantes sueltas repartidas por el codigo.
//
//  Motivo: cada mapa se normaliza a ANCHO_OBJETIVO en su eje horizontal mayor,
//  pero su relieve y su proporcion X/Z cambian de un DEM a otro. Derivar todo
//  de la diagonal real hace que la MISMA proporcion visual se mantenga en
//  cualquier terreno sin retocar constantes a mano.
// ============================================================================
struct EscalaMundo {
    // ---- Referencia ----
    float diagonalTerreno = 141.0f;   // length(max - min) del terreno normalizado
    float ladoMenor       = 100.0f;   // min(ancho, profundidad)
    glm::vec3 centro{0.0f};

    // ---- Dron ----
    float escalaDron      = 0.0022f;  // multiplicador del modelo YA normalizado a 1
    float radioDron       = 0.15f;    // media extension: colision y offsets visuales
    float velocidadMaxima = 7.8f;     // u/s horizontales
    float aceleracion     = 17.0f;    // u/s^2 al pedir movimiento
    float desaceleracion  = 25.0f;    // u/s^2 al soltar (mayor: frena antes de lo que arranca)
    float velocidadVertical = 3.5f;   // u/s en el eje Y
    float epsilonVelocidad  = 0.015f; // por debajo de esto la velocidad se pone a CERO exacto
    float alturaSegura    = 0.85f;    // separacion minima sobre el relieve
    float alturaMaxima    = 42.0f;    // techo sobre el relieve
    float alturaInicial   = 7.0f;
    float margenMapa      = 0.6f;

    // ---- Camara ----
    float distanciaCamara = 3.7f;
    float distanciaMinima = 1.1f;
    float distanciaMaxima = 78.0f;
    float alturaSueloCamara = 0.6f;
    float planoCercano    = 0.05f;
    float planoLejano     = 425.0f;

    // ---- Escaneo ----
    float radioEscaneo    = 6.4f;
    float radioConoMaximo = 3.2f;     // tope del cono de luz, aunque el dron suba mucho

    // ---- Marcadores de sondeo ----
    float alturaMarcador  = 0.64f;
    float anchoMarcador   = 0.038f;

    // ---- Atenuacion radial de la rejilla y las estacas ----
    float radioNitido            = 18.0f;
    float radioDesvanecido       = 72.0f;
    float radioNitidoMarcador    = 30.0f;
    float radioDesvanecidoMarcador = 110.0f;

    // Recalcula todo a partir del terreno cargado.
    // diagonalMallaDron = diagonal del bounding box del modelo del dron tal y
    // como quedo tras normalizarlo en el cargador (extension maxima = 1).
    static EscalaMundo calcular(const Terreno& terreno, float diagonalMallaDron);
};
