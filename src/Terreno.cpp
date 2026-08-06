#include "Terreno.h"
#include "CargadorModelos.h"
#include "Configuracion.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

bool Terreno::cargarDesdeArchivo(const std::string& ruta) {
    std::string ext = fs::path(ruta).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    std::vector<glm::vec3> crudo;
    malla.limpiar();
    malla.floatsPorVertice = 3;
    mallaDeLineas = false;

    bool ok = false;
    if (ext == ".obj")                          ok = CargadorModelos::cargarOBJ(ruta, crudo, malla.indices);
    else if (ext == ".glb" || ext == ".gltf")   ok = CargadorModelos::cargarGLBTerreno(ruta, crudo, malla.indices);
    else if (ext == ".csv") { ok = CargadorModelos::cargarCSVCalles(ruta, crudo, malla.indices); mallaDeLineas = true; }
    else { std::cerr << "  Formato no soportado\n"; return false; }

    if (!ok) return false;

    finalizar(crudo);
    nombreArchivo = fs::path(ruta).filename().string();
    std::cout << "[TERRAIN] Vertices: " << crudo.size();
    if (mallaDeLineas) std::cout << "  Segmentos: " << (malla.indices.size() / 2);
    else std::cout << "  Triangulos: " << (malla.indices.size() / 3);
    std::cout << "\n[HEIGHTMAP] Generado " << anchoGrilla << "x" << anchoGrilla << "\n";
    return true;
}

void Terreno::finalizar(std::vector<glm::vec3>& crudo) {
    // 1) Detectar cual eje es el "arriba" real: el de menor extension.
    //    Muchos DEM vienen Z-up y quedarian de canto si no se reorientan.
    glm::vec3 bMin(1e9f), bMax(-1e9f);
    for (auto& p : crudo) { bMin = glm::min(bMin, p); bMax = glm::max(bMax, p); }
    glm::vec3 extension = bMax - bMin;

    int arriba = 0;
    if (extension.y <= extension.x && extension.y <= extension.z)      arriba = 1;
    else if (extension.z <= extension.x && extension.z <= extension.y) arriba = 2;
    int a = (arriba + 1) % 3, b = (arriba + 2) % 3;
    for (auto& p : crudo) { glm::vec3 q(p[a], p[arriba], p[b]); p = q; }

    // 2) Centrar y escalar para que todo mapa ocupe el mismo espacio de juego.
    bMin = glm::vec3(1e9f); bMax = glm::vec3(-1e9f);
    for (auto& p : crudo) { bMin = glm::min(bMin, p); bMax = glm::max(bMax, p); }
    glm::vec3 centro = (bMin + bMax) * 0.5f;
    float extensionMax = std::max(bMax.x - bMin.x, bMax.z - bMin.z);
    float escala = (extensionMax > 1e-6f) ? (Configuracion::ANCHO_OBJETIVO / extensionMax) : 1.0f;

    malla.vertices.clear();
    malla.vertices.reserve(crudo.size() * 3);
    for (auto& p : crudo) {
        glm::vec3 n = (p - centro) * escala;
        n.y *= Configuracion::EXAGERACION_Y;
        malla.vertices.push_back(n.x);
        malla.vertices.push_back(n.y);
        malla.vertices.push_back(n.z);
    }

    limites.minX = (bMin.x - centro.x) * escala;
    limites.maxX = (bMax.x - centro.x) * escala;
    limites.minZ = (bMin.z - centro.z) * escala;
    limites.maxZ = (bMax.z - centro.z) * escala;
    limites.minY = (bMin.y - centro.y) * escala * Configuracion::EXAGERACION_Y;
    limites.maxY = (bMax.y - centro.y) * escala * Configuracion::EXAGERACION_Y;
    if (limites.ancho()       < 1e-3f) limites.maxX = limites.minX + 1.0f;
    if (limites.profundidad() < 1e-3f) limites.maxZ = limites.minZ + 1.0f;

    // 3) Mapa de alturas por "maximo por celda": nos interesa la superficie
    //    visible, no un promedio que hundiria las crestas.
    anchoGrilla = Configuracion::RESOLUCION_GRILLA;
    alturas.assign(static_cast<std::size_t>(anchoGrilla) * anchoGrilla, -1e9f);
    for (std::size_t i = 0; i + 2 < malla.vertices.size(); i += 3) {
        float x = malla.vertices[i], y = malla.vertices[i + 1], z = malla.vertices[i + 2];
        int gx = std::clamp((int)((x - limites.minX) / limites.ancho()       * (anchoGrilla - 1) + 0.5f), 0, anchoGrilla - 1);
        int gz = std::clamp((int)((z - limites.minZ) / limites.profundidad() * (anchoGrilla - 1) + 0.5f), 0, anchoGrilla - 1);
        float& celda = alturas[(std::size_t)gz * anchoGrilla + gx];
        if (y > celda) celda = y;
    }

    // Cobertura ANTES de rellenar: solo aqui se sabe que celdas recibieron un
    // vertice de verdad. Despues de la dilatacion todas tienen altura y el
    // porcentaje de exploracion contaria terreno que no existe.
    cobertura.assign(alturas.size(), 0);
    celdasValidas = 0;
    for (std::size_t i = 0; i < alturas.size(); ++i)
        if (alturas[i] > -1e8f) { cobertura[i] = 1; ++celdasValidas; }
    // Blindaje: si la malla es tan dispersa que apenas toca celdas, se cuenta
    // la grilla entera para no dejar el porcentaje bloqueado en un valor absurdo.
    if (celdasValidas < alturas.size() / 20) {
        cobertura.assign(alturas.size(), 1);
        celdasValidas = alturas.size();
    }
    std::cout << "[HEIGHTMAP] Celdas validas: " << celdasValidas << " / " << alturas.size() << "\n";

    rellenarHuecos();
    double sumaAlturas = 0.0;
    for (float h : alturas) sumaAlturas += h;
    alturaMedia = alturas.empty() ? 0.0f
                                  : static_cast<float>(sumaAlturas / alturas.size());
    generarRejilla(Configuracion::RESOLUCION_REJILLA);
    // El quadtree se construye SOBRE la rejilla ya generada: necesita sus
    // vertices para calcular el AABB real de cada cuadrante.
    quadtree.construir(rejilla, resolucionRejilla, limites);
}

void Terreno::rellenarHuecos() {
    const float VACIO = -1e8f;   // marca puesta en el paso anterior

    // Dilatacion iterativa: cada pasada rellena un anillo de una celda con el
    // promedio de sus vecinos ya conocidos. Se prefiere a poner minY de golpe
    // porque aquello abriria pozos verticales en mitad de la cuadricula.
    std::vector<float> siguiente;
    for (int pasada = 0; pasada < 64; ++pasada) {
        siguiente = alturas;
        bool quedanVacias = false;
        bool seLlenoAlguna = false;

        for (int z = 0; z < anchoGrilla; ++z) {
            for (int x = 0; x < anchoGrilla; ++x) {
                std::size_t i = (std::size_t)z * anchoGrilla + x;
                if (alturas[i] > VACIO) continue;

                float suma = 0.0f;
                int vecinos = 0;
                for (int dz = -1; dz <= 1; ++dz) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        if (dx == 0 && dz == 0) continue;
                        int nx = x + dx, nz = z + dz;
                        if (nx < 0 || nz < 0 || nx >= anchoGrilla || nz >= anchoGrilla) continue;
                        float h = alturas[(std::size_t)nz * anchoGrilla + nx];
                        if (h > VACIO) { suma += h; ++vecinos; }
                    }
                }
                if (vecinos > 0) { siguiente[i] = suma / vecinos; seLlenoAlguna = true; }
                else             { quedanVacias = true; }
            }
        }
        alturas.swap(siguiente);
        if (!quedanVacias) break;     // ya no queda ningun hueco
        if (!seLlenoAlguna) break;    // no hay desde donde propagar (mapa vacio)
    }

    for (auto& h : alturas) if (h <= VACIO) h = limites.minY;
}

void Terreno::generarRejilla(int resolucion) {
    resolucionRejilla = std::max(2, resolucion);
    const int R = resolucionRejilla;

    rejilla.limpiar();
    rejilla.floatsPorVertice = 3;
    rejilla.vertices.reserve((std::size_t)R * R * 3);

    // Vertices: muestreo regular del heightmap con interpolacion bilineal.
    for (int z = 0; z < R; ++z) {
        for (int x = 0; x < R; ++x) {
            float wx = limites.minX + limites.ancho()       * (float)x / (R - 1);
            float wz = limites.minZ + limites.profundidad() * (float)z / (R - 1);
            rejilla.vertices.push_back(wx);
            rejilla.vertices.push_back(alturaEn(wx, wz));
            rejilla.vertices.push_back(wz);
        }
    }

    // Aristas: se indexan como segmentos sueltos (GL_LINES) en vez de usar
    // glPolygonMode sobre triangulos, porque el modo alambre dibujaria tambien
    // la diagonal de cada celda y la cuadricula perderia su lectura limpia.
    rejilla.indices.reserve((std::size_t)2 * R * (R - 1) * 2);
    for (int z = 0; z < R; ++z)
        for (int x = 0; x + 1 < R; ++x) {
            rejilla.indices.push_back((unsigned int)(z * R + x));
            rejilla.indices.push_back((unsigned int)(z * R + x + 1));
        }
    for (int z = 0; z + 1 < R; ++z)
        for (int x = 0; x < R; ++x) {
            rejilla.indices.push_back((unsigned int)(z * R + x));
            rejilla.indices.push_back((unsigned int)((z + 1) * R + x));
        }
}

float Terreno::alturaEn(float x, float z) const {
    if (anchoGrilla <= 0) return 0.0f;

    float fx = std::clamp((x - limites.minX) / limites.ancho()       * (anchoGrilla - 1), 0.0f, (float)(anchoGrilla - 1));
    float fz = std::clamp((z - limites.minZ) / limites.profundidad() * (anchoGrilla - 1), 0.0f, (float)(anchoGrilla - 1));

    int x0 = (int)fx, z0 = (int)fz;
    int x1 = std::min(x0 + 1, anchoGrilla - 1), z1 = std::min(z0 + 1, anchoGrilla - 1);
    float tx = fx - x0, tz = fz - z0;

    float superior = alturas[(std::size_t)z0 * anchoGrilla + x0] * (1 - tx) + alturas[(std::size_t)z0 * anchoGrilla + x1] * tx;
    float inferior = alturas[(std::size_t)z1 * anchoGrilla + x0] * (1 - tx) + alturas[(std::size_t)z1 * anchoGrilla + x1] * tx;
    return superior * (1 - tz) + inferior * tz;
}

float Terreno::obtenerDensidad() const {
    float area = limites.ancho() * limites.profundidad();
    return area > 1e-5f ? static_cast<float>(obtenerNumeroVertices()) / area : 0.0f;
}
