#include "marchingSquares.h"
#include "MapaExploracion.h"
#include "Terreno.h"

#include <algorithm>

namespace {

// Interpolacion lineal sobre una arista: devuelve el punto exacto donde la
// altura cruza el nivel. Sin esto las curvas saldrian "escalonadas", pegadas
// siempre al centro de la arista.
glm::vec2 interpolar(const glm::vec2& p1, float v1, const glm::vec2& p2, float v2, float nivel) {
    float denominador = v2 - v1;
    if (std::abs(denominador) < 1e-9f) return p1;   // arista plana: cualquier punto vale
    float t = std::clamp((nivel - v1) / denominador, 0.0f, 1.0f);
    return p1 + (p2 - p1) * t;
}

} // namespace

namespace MarchingSquares {

void generarSegmentos(const Terreno& terreno,
                      const MapaExploracion& mapa,
                      float nivel,
                      int paso,
                      std::vector<Segmento>& salida) {
    const int N = terreno.obtenerAnchoGrilla();
    if (N < 2 || paso < 1) return;

    const std::vector<float>& alturas = terreno.obtenerAlturas();
    const LimitesMundo& lim = terreno.obtenerLimites();

    // Grilla -> mundo.
    auto mundoX = [&](int gx) { return lim.minX + lim.ancho()       * (float)gx / (N - 1); };
    auto mundoZ = [&](int gz) { return lim.minZ + lim.profundidad() * (float)gz / (N - 1); };
    auto alturaEnCelda = [&](int gx, int gz) { return alturas[(std::size_t)gz * N + gx]; };

    for (int z = 0; z + paso < N; z += paso) {
        for (int x = 0; x + paso < N; x += paso) {
            int x1 = x + paso, z1 = z + paso;

            // Solo se dibuja lo que el dron ya escaneo: si falta una esquina,
            // la celda pertenece a la niebla y se salta.
            if (!mapa.estaExplorada(x,  z ) || !mapa.estaExplorada(x1, z ) ||
                !mapa.estaExplorada(x1, z1) || !mapa.estaExplorada(x,  z1)) continue;

            // Esquinas en sentido antihorario desde la inferior izquierda.
            const glm::vec2 p0(mundoX(x ), mundoZ(z ));
            const glm::vec2 p1(mundoX(x1), mundoZ(z ));
            const glm::vec2 p2(mundoX(x1), mundoZ(z1));
            const glm::vec2 p3(mundoX(x ), mundoZ(z1));

            const float v0 = alturaEnCelda(x,  z );
            const float v1 = alturaEnCelda(x1, z );
            const float v2 = alturaEnCelda(x1, z1);
            const float v3 = alturaEnCelda(x,  z1);

            // Indice de 4 bits: un bit por esquina que queda por encima del nivel.
            int caso = 0;
            if (v0 >= nivel) caso |= 1;
            if (v1 >= nivel) caso |= 2;
            if (v2 >= nivel) caso |= 4;
            if (v3 >= nivel) caso |= 8;
            if (caso == 0 || caso == 15) continue;   // celda entera dentro o fuera

            // Cruces sobre cada arista (e0 abajo, e1 derecha, e2 arriba, e3 izquierda).
            const glm::vec2 e0 = interpolar(p0, v0, p1, v1, nivel);
            const glm::vec2 e1 = interpolar(p1, v1, p2, v2, nivel);
            const glm::vec2 e2 = interpolar(p2, v2, p3, v3, nivel);
            const glm::vec2 e3 = interpolar(p3, v3, p0, v0, nivel);

            auto emitir = [&salida](const glm::vec2& a, const glm::vec2& b) {
                salida.push_back({a, b});
            };

            switch (caso) {
                case 1:  emitir(e3, e0); break;
                case 2:  emitir(e0, e1); break;
                case 3:  emitir(e3, e1); break;
                case 4:  emitir(e1, e2); break;
                case 6:  emitir(e0, e2); break;
                case 7:  emitir(e3, e2); break;
                case 8:  emitir(e2, e3); break;
                case 9:  emitir(e2, e0); break;
                case 11: emitir(e2, e1); break;
                case 12: emitir(e1, e3); break;
                case 13: emitir(e1, e0); break;
                case 14: emitir(e0, e3); break;

                // ---- Casos ambiguos 5 y 10 (sillas de montar) ----
                // Dos esquinas opuestas por encima del nivel admiten DOS
                // trazados validos. Se decide mirando el promedio de las cuatro
                // esquinas, que aproxima el valor en el centro de la celda: si
                // el centro esta dentro, las esquinas altas estan conectadas
                // entre si; si esta fuera, son dos islas separadas.
                // Sin este desempate las curvas se cortan de forma incoherente
                // entre celdas vecinas y el grafo no logra cerrarlas.
                case 5: {
                    float centro = (v0 + v1 + v2 + v3) * 0.25f;
                    if (centro >= nivel) { emitir(e0, e1); emitir(e2, e3); }  // c0-c2 unidas
                    else                 { emitir(e3, e0); emitir(e1, e2); }  // dos islas
                    break;
                }
                case 10: {
                    float centro = (v0 + v1 + v2 + v3) * 0.25f;
                    if (centro >= nivel) { emitir(e3, e0); emitir(e1, e2); }  // c1-c3 unidas
                    else                 { emitir(e0, e1); emitir(e2, e3); }  // dos islas
                    break;
                }
                default: break;
            }
        }
    }
}

} // namespace MarchingSquares
