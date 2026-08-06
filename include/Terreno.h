#pragma once
#include "Limites.h"
#include "MallaCruda.h"
#include "Quadtree.h"

#include <glm/glm.hpp>
#include <string>
#include <vector>

// ============================================================================
//  MODELO: Terreno
//  Carga la geometria (.obj / .glb / .gltf / .csv), la normaliza y construye
//  un mapa de alturas. CERO llamadas a OpenGL: solo datos y matematica.
//
//  El mapa de alturas es un ARRAY 2D APLANADO en un std::vector<float> con
//  acceso [z * ancho + x]. Se prefiere a un vector<vector<float>> porque
//  garantiza memoria contigua (una sola reserva, mejor cache) y porque la
//  consulta bilineal toca 4 celdas vecinas que asi caen casi siempre en la
//  misma linea de cache.
// ============================================================================
class Terreno {
public:
    // Carga y normaliza. Devuelve false si el formato no se reconoce o falla.
    bool cargarDesdeArchivo(const std::string& ruta);

    // Altura del terreno en coordenadas de mundo, con interpolacion bilineal.
    float alturaEn(float x, float z) const;

    const MallaCruda&    obtenerMalla()   const { return malla; }
    const LimitesMundo&  obtenerLimites() const { return limites; }
    bool                 esMallaDeLineas() const { return mallaDeLineas; }

    // Rejilla regular de lineas (estetica de cuadricula topografica). Se
    // construye muestreando el heightmap, no la malla original: asi cualquier
    // terreno, venga de donde venga, se dibuja con la misma cuadricula limpia.
    const MallaCruda& obtenerRejilla()           const { return rejilla; }
    int               obtenerResolucionRejilla() const { return resolucionRejilla; }

    // Particion espacial de la rejilla: nivel de detalle y descarte por frustum.
    const Quadtree&   obtenerQuadtree()          const { return quadtree; }

    const std::vector<float>& obtenerAlturas() const { return alturas; }
    int  obtenerAnchoGrilla() const { return anchoGrilla; }
    bool estaCargado()        const { return anchoGrilla > 0 && !malla.vacia(); }

    const std::string& obtenerNombreArchivo() const { return nombreArchivo; }
    float obtenerAlturaMedia() const { return alturaMedia; }
    std::size_t obtenerNumeroVertices() const { return malla.numeroVertices(); }
    std::size_t obtenerNumeroTriangulos() const { return mallaDeLineas ? 0 : malla.indices.size() / 3; }
    float obtenerDensidad() const;

private:
    // Reorienta a Y-up, centra, escala a ANCHO_OBJETIVO y rellena el heightmap.
    void finalizar(std::vector<glm::vec3>& crudo);

    // Propaga alturas hacia las celdas sin muestra (mallas irregulares dejan
    // huecos que, sin esto, se hunden hasta el minimo y rompen la cuadricula).
    void rellenarHuecos();

    // Construye la rejilla de lineas: vertices muestreados del heightmap y un
    // EBO con las aristas horizontales y verticales.
    void generarRejilla(int resolucion);

    MallaCruda   malla;
    MallaCruda   rejilla;
    Quadtree     quadtree;
    LimitesMundo limites;
    bool         mallaDeLineas = false;   // true para mapas de calles (CSV)

    std::vector<float> alturas;           // grilla [z * anchoGrilla + x]
    int                anchoGrilla = 0;
    int                resolucionRejilla = 0;
    float              alturaMedia = 0.0f;

    std::string  nombreArchivo;
};
