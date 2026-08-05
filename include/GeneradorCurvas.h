#pragma once
#include "GrafoCurvas.h"
#include "marchingSquares.h"

#include <cstddef>
#include <vector>

class Terreno;
class MapaExploracion;
class CurvasNivel;

// ============================================================================
//  MODELO: orquesta la generacion de curvas de nivel.
//  Marching Squares -> grafo -> polilineas, para N niveles equiespaciados.
//
//  REGENERACION INCREMENTAL
//  ------------------------
//  Rehacer los 12 niveles de golpe se midio en 8 ms de media y hasta 14,5 ms en
//  el peor caso: un frame entero perdido cada vez, o sea un tiron visible cinco
//  veces por segundo. Por eso el trabajo se reparte en tres filtros:
//
//    1) Temporizador: no se regenera mas de una vez cada 200 ms.
//    2) Rotacion por niveles: cada tick recalcula UN SOLO nivel y lo sustituye
//       en su casilla; los otros 11 se conservan del ciclo anterior. El coste
//       por tick baja a ~1 ms y el mapa completo se refresca en 12 ticks.
//    3) Deteccion de cambios: si el numero de celdas exploradas no ha variado,
//       el mapa ya esta al dia y no se recalcula absolutamente nada.
//
//  El precio es que un nivel concreto puede ir hasta ~2,4 s por detras mientras
//  el jugador explora. En pantalla no se nota, porque las curvas aparecen igual
//  de forma progresiva, y a cambio no se pierde ni un frame.
// ============================================================================
class GeneradorCurvas {
public:
    void reiniciar();

    // Devuelve true si esta llamada toco las curvas (la Vista debe resubir su
    // VBO solo en ese caso).
    bool actualizar(const Terreno& terreno, const MapaExploracion& mapa,
                    CurvasNivel& curvas, float dt);

private:
    float alturaDeNivel(const Terreno& terreno, int indice) const;
    void  regenerarNivel(const Terreno& terreno, const MapaExploracion& mapa, int indice);
    void  volcarEn(const Terreno& terreno, CurvasNivel& curvas) const;

    float       acumulador = 0.0f;
    std::size_t celdasUltimaGeneracion = 0;
    int         nivelesPendientes = 0;   // cuantos quedan por refrescar
    int         indiceNivel = 0;         // siguiente nivel del turno rotatorio

    // Una casilla por nivel: permite sustituir uno sin tocar los demas.
    std::vector<std::vector<Polilinea>> curvasPorNivel;

    GrafoCurvas grafo;
    // Buffers reutilizados entre regeneraciones para no pedir memoria cada vez.
    std::vector<MarchingSquares::Segmento> segmentos;
};
