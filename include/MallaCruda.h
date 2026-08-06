#pragma once
#include <vector>

// ============================================================================
//  Malla en memoria CPU: es dato puro del Modelo, sin una sola llamada a
//  OpenGL. La Vista la lee para subirla a la GPU; asi el Modelo puede cargar,
//  transformar o analizar geometria sin depender del contexto grafico.
// ============================================================================
struct MallaCruda {
    std::vector<float>        vertices;              // atributos intercalados
    std::vector<unsigned int> indices;               // triangulos
    // Aristas caracteristicas (pares para GL_LINES). Solo se rellena en mallas
    // que se dibujan como armazon, como el dron.
    std::vector<unsigned int> aristas;
    int                       floatsPorVertice = 3;  // 3 = xyz, 7 = xyz + normal + idHelice

    void limpiar() {
        vertices.clear();
        indices.clear();
        aristas.clear();
    }
    std::size_t numeroVertices() const {
        return floatsPorVertice > 0 ? vertices.size() / floatsPorVertice : 0;
    }
    bool vacia() const { return vertices.empty(); }
};
