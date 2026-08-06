#include "EscalaMundo.h"
#include "Configuracion.h"
#include "Terreno.h"

#include <algorithm>
#include <cmath>
#include <iostream>

EscalaMundo EscalaMundo::calcular(const Terreno& terreno, float diagonalMallaDron) {
    EscalaMundo e;

    const LimitesMundo& lim = terreno.obtenerLimites();
    const float ancho  = std::max(1e-3f, lim.ancho());
    const float fondo  = std::max(1e-3f, lim.profundidad());
    const float alto   = std::max(0.0f,  lim.rangoAltura());

    e.diagonalTerreno = std::sqrt(ancho * ancho + fondo * fondo + alto * alto);
    e.ladoMenor       = std::min(ancho, fondo);
    e.centro          = glm::vec3((lim.minX + lim.maxX) * 0.5f,
                                  (lim.minY + lim.maxY) * 0.5f,
                                  (lim.minZ + lim.maxZ) * 0.5f);

    const float D = e.diagonalTerreno;

    // ---- Dron -------------------------------------------------------------
    // El objetivo es que la diagonal del dron sea RATIO_DRON veces la del
    // terreno. El modelo llega ya normalizado (extension maxima = 1), asi que
    // basta dividir por su propia diagonal para que la relacion se cumpla sea
    // cual sea la forma del GLB que se cargue.
    const float diagDron = (diagonalMallaDron > 1e-4f) ? diagonalMallaDron : 1.0f;
    e.escalaDron = D * Configuracion::RATIO_DRON_TERRENO / diagDron;
    e.radioDron  = e.escalaDron * diagDron * 0.5f;

    // ---- Vuelo ------------------------------------------------------------
    // Cruzar el lado corto del mapa a velocidad maxima debe llevar ~13 s: es lo
    // que hace que el terreno se lea inmenso al volarlo.
    e.velocidadMaxima   = D * 0.055f;
    e.aceleracion       = e.velocidadMaxima * 2.2f;
    e.desaceleracion    = e.velocidadMaxima * 3.4f;   // frena mas rapido de lo que acelera
    e.velocidadVertical = e.velocidadMaxima * 0.45f;
    e.epsilonVelocidad  = e.velocidadMaxima * 0.002f;

    e.alturaSegura  = std::max(e.radioDron * 2.5f, D * 0.006f);
    e.alturaMaxima  = D * 0.30f;
    // Altura de partida contenida: si el dron arranca muy alto, la huella del
    // haz de escaneo cae fuera del encuadre de tercera persona y el efecto no
    // se aprecia. El jugador puede subir con SPACE cuanto quiera.
    e.alturaInicial = D * 0.022f;
    // Margen amplio: con el margen minimo anterior el dron llegaba justo al
    // canto de la malla y la camara acababa asomada al vacio de fuera del mapa.
    // 3,5% de la diagonal (~5 u) mantiene el aparato siempre sobre relieve.
    e.margenMapa    = D * 0.035f;

    // ---- Camara -----------------------------------------------------------
    // Distancia proporcional a la diagonal, NO fija: en un mapa muy alargado la
    // misma distancia absoluta encuadraria de forma distinta. Con 0.040 el dron
    // ocupa ~5% del alto de pantalla: pequeno pero perfectamente reconocible.
    e.distanciaCamara   = D * 0.040f;
    e.distanciaMinima   = D * 0.008f;
    e.distanciaMaxima   = D * 0.55f;
    e.alturaSueloCamara = e.alturaSegura * 0.7f;

    // Cercano ligado al tamano del dron (si no, se recortaria al acercar la
    // camara); lejano suficiente para cubrir el mapa desde el zoom maximo.
    e.planoCercano = std::max(0.02f, e.escalaDron * diagDron * 0.30f);
    e.planoLejano  = D * 3.0f;

    // ---- Escaneo ----------------------------------------------------------
    // Radio doblado respecto a la version anterior (era 0.045): cubre cuatro
    // veces mas superficie por pasada, asi que cartografiar el mapa completo
    // deja de ser tedioso.
    e.radioEscaneo    = D * 0.090f;
    e.radioConoMaximo = e.radioEscaneo * 0.55f;

    // ---- Marcadores -------------------------------------------------------
    e.alturaMarcador = D * Configuracion::RATIO_ALTURA_MARCADOR;
    e.anchoMarcador  = e.alturaMarcador * 0.06f;

    // ---- Atenuacion radial ------------------------------------------------
    e.radioNitido                = D * 0.13f;
    e.radioDesvanecido           = D * 0.52f;
    e.radioNitidoMarcador        = D * 0.22f;
    e.radioDesvanecidoMarcador   = D * 0.80f;

    std::cout << "[ESCALA] diagonal " << e.diagonalTerreno
              << " | dron x" << e.escalaDron
              << " (" << (e.escalaDron * diagDron) << " u)"
              << " | vel " << e.velocidadMaxima
              << " | camara " << e.distanciaCamara
              << " | radar " << e.radioEscaneo
              << " | estaca " << e.alturaMarcador << "\n";
    return e;
}
