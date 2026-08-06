#include "Aplicacion.h"
#include "CargadorModelos.h"
#include "Configuracion.h"
#include "Dron.h"
#include "MapaExploracion.h"
#include "SistemaExploracion.h"
#include "SistemaMedicion.h"
#include "Terreno.h"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {
int validarModelos(bool probarLogica) {
    std::cout << "[TEST] Descubrimiento y carga de recursos\n";
    std::vector<std::string> mapas = CargadorModelos::descubrirTerrenos(Configuracion::CARPETA_ASSETS);
    if (mapas.empty()) { std::cerr << "[ERROR] No hay mapas para validar\n"; return 1; }

    int fallos = 0;
    for (std::size_t i = 0; i < mapas.size(); ++i) {
        Terreno terreno;
        std::cout << "[TEST] [" << i + 1 << "/" << mapas.size() << "] "
                  << std::filesystem::path(mapas[i]).filename().string() << "\n";
        if (!terreno.cargarDesdeArchivo(mapas[i]) || terreno.obtenerAlturas().empty()) {
            std::cerr << "[FAIL] Terreno o heightmap invalido\n";
            ++fallos;
            continue;
        }
        float centro = terreno.alturaEn(0.0f, 0.0f);
        if (!std::isfinite(centro)) { std::cerr << "[FAIL] Altura no finita\n"; ++fallos; }

        if (probarLogica) {
            MapaExploracion mapa;
            mapa.reiniciar(terreno.obtenerAnchoGrilla(), terreno.obtenerAnchoGrilla(), terreno.obtenerLimites());
            SistemaExploracion exploracion;
            exploracion.generar(terreno, Configuracion::NUM_PUNTOS_ESCANEO,
                                Configuracion::SEMILLA_ESCANEO + static_cast<unsigned int>(i));
            std::vector<glm::vec3> objetivos;
            for (const auto& p : exploracion.obtenerPuntos()) objetivos.push_back(p.posicion);
            for (glm::vec3 p : objetivos) {
                p.y += 10.0f;
                for (int frame = 0; frame < 210; ++frame)
                    exploracion.actualizar(p, 0.0f, 1.0f / 60.0f, mapa);
            }
            if (!objetivos.empty() && !exploracion.estaCompleta()) {
                std::cerr << "[FAIL] La mision no completa sus zonas\n";
                ++fallos;
            }

            Dron dron;
            dron.establecerPosicion(glm::vec3(terreno.obtenerLimites().maxX + 20.0f,
                                               terreno.obtenerLimites().minY - 20.0f,
                                               terreno.obtenerLimites().maxZ + 20.0f));
            dron.mover(glm::vec3(0.0f), 0.0f, 1.0f / 60.0f);
            dron.actualizar(terreno, 1.0f / 60.0f);
            const glm::vec3& dp = dron.obtenerPosicion();
            if (dp.y + 1e-4f < terreno.alturaEn(dp.x, dp.z) + Configuracion::ALTURA_MINIMA) {
                std::cerr << "[FAIL] Colision vertical del dron\n";
                ++fallos;
            }

            SistemaMedicion medicion;
            medicion.agregarPunto({0, 0, 0});
            medicion.agregarPunto({3, 4, 0});
            ResultadoMedicion resultado = medicion.calcular();
            if (std::abs(resultado.distancia3D - 5.0f) > 1e-3f) {
                std::cerr << "[FAIL] Calculo de distancia 3D\n";
                ++fallos;
            }
        }
        std::cout << "[PASS] " << terreno.obtenerNumeroVertices() << " vertices, "
                  << terreno.obtenerNumeroTriangulos() << " triangulos, altura media "
                  << terreno.obtenerAlturaMedia() << "\n";
    }

    MallaCruda dron;
    glm::vec3 pivotes[4]{};
    if (!CargadorModelos::cargarDronAnimado(Configuracion::RUTA_DRON, dron, pivotes,
                                             Configuracion::TAMANIO_DRON)) {
        std::cerr << "[FAIL] Modelo GLB del dron\n";
        ++fallos;
    } else {
        std::cout << "[PASS] Dron: " << dron.numeroVertices() << " vertices, helices identificadas\n";
    }
    std::cout << (fallos == 0 ? "[TEST] RESULTADO: OK\n" : "[TEST] RESULTADO: FALLIDO\n");
    return fallos == 0 ? 0 : 1;
}
}

int main(int argc, char** argv) {
    if (argc > 1) {
        std::string opcion = argv[1];
        if (opcion == "--validar-assets") return validarModelos(false);
        if (opcion == "--pruebas-modelo") return validarModelos(true);
        if (opcion == "--ayuda") {
            std::cout << "GeoDrone\n  --validar-assets  carga todos los mapas y el dron\n"
                         "  --pruebas-modelo   valida carga, escaneo, colision y mediciones\n";
            return 0;
        }
    }
    Aplicacion aplicacion;
    if (!aplicacion.inicializar()) return -1;
    aplicacion.ejecutar();
    aplicacion.liberar();
    return 0;
}
