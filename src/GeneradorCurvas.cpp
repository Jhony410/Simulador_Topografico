#include "GeneradorCurvas.h"
#include "Configuracion.h"
#include "CurvasNivel.h"
#include "MapaExploracion.h"
#include "Terreno.h"

void GeneradorCurvas::reiniciar() {
    acumulador = 0.0f;
    celdasUltimaGeneracion = 0;
    indiceNivel = 0;
    nivelesPendientes = Configuracion::NIVELES_CURVAS;
    curvasPorNivel.assign(Configuracion::NIVELES_CURVAS, {});
    grafo.limpiar();
}

float GeneradorCurvas::alturaDeNivel(const Terreno& terreno, int indice) const {
    const LimitesMundo& lim = terreno.obtenerLimites();
    // Niveles interiores: se evita colocar uno justo en minY o en maxY, donde
    // la isolinea degenera al borde del mapa o a un unico punto.
    return lim.minY + lim.rangoAltura() * (float)(indice + 1) / (Configuracion::NIVELES_CURVAS + 1);
}

bool GeneradorCurvas::actualizar(const Terreno& terreno, const MapaExploracion& mapa,
                                 CurvasNivel& curvas, float dt) {
    acumulador += dt;
    if (acumulador < Configuracion::INTERVALO_CURVAS) return false;
    acumulador = 0.0f;

    if (curvasPorNivel.size() != (std::size_t)Configuracion::NIVELES_CURVAS)
        curvasPorNivel.assign(Configuracion::NIVELES_CURVAS, {});

    // Celdas nuevas => todos los niveles quedan obsoletos y vuelven a la cola.
    std::size_t celdas = mapa.celdasExploradas();
    if (celdas != celdasUltimaGeneracion) {
        celdasUltimaGeneracion = celdas;
        nivelesPendientes = Configuracion::NIVELES_CURVAS;
    }
    if (nivelesPendientes <= 0) return false;   // nada que hacer: mapa al dia

    regenerarNivel(terreno, mapa, indiceNivel);
    indiceNivel = (indiceNivel + 1) % Configuracion::NIVELES_CURVAS;
    --nivelesPendientes;

    volcarEn(terreno, curvas);
    return true;
}

void GeneradorCurvas::regenerarNivel(const Terreno& terreno, const MapaExploracion& mapa,
                                     int indice) {
    auto& destino = curvasPorNivel[indice];
    destino.clear();

    const LimitesMundo& lim = terreno.obtenerLimites();
    const int N = terreno.obtenerAnchoGrilla();
    if (N < 2 || lim.rangoAltura() < 1e-5f) return;

    segmentos.clear();
    MarchingSquares::generarSegmentos(terreno, mapa, alturaDeNivel(terreno, indice),
                                      Configuracion::PASO_CURVAS, segmentos);
    if (segmentos.empty()) return;

    // Dos extremos se funden si distan menos de una fraccion del lado de celda:
    // asi la tolerancia escala con el tamaño del mapa en vez de ser un numero
    // magico en unidades de mundo.
    float ladoCelda = lim.ancho() / (N - 1) * Configuracion::PASO_CURVAS;

    grafo.limpiar();
    grafo.establecerEpsilon(ladoCelda * 0.05f);
    for (const auto& s : segmentos) grafo.agregarSegmento(s.a, s.b);
    grafo.extraerPolilineas(alturaDeNivel(terreno, indice), destino);
}

void GeneradorCurvas::volcarEn(const Terreno& terreno, CurvasNivel& curvas) const {
    const LimitesMundo& lim = terreno.obtenerLimites();
    curvas.limpiar();
    curvas.establecerRango(lim.minY, lim.maxY);
    for (const auto& nivel : curvasPorNivel)
        for (const auto& curva : nivel)
            curvas.agregar(curva);
}
