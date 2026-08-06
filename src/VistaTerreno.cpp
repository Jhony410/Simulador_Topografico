#include "VistaTerreno.h"
#include "Configuracion.h"
#include "GestorRecursos.h"
#include "MapaExploracion.h"

#include <glm/gtc/type_ptr.hpp>
#include <iostream>

namespace {
void subirLineasIndexadas(GLuint& vao, GLuint& vbo, GLuint& ebo,
                          const std::vector<float>& vertices,
                          const std::vector<unsigned int>& indices) {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
                 vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
                 indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}
}

void VistaTerreno::inicializar(GestorRecursos& recursos) {
    programa            = recursos.obtenerPrograma("shaders/terreno.vert", "shaders/terreno.frag");
    locModelo           = glGetUniformLocation(programa, "model");
    locVista            = glGetUniformLocation(programa, "view");
    locProyeccion       = glGetUniformLocation(programa, "projection");
    locColorBase        = glGetUniformLocation(programa, "uColorBase");
    locAlphaMaximo      = glGetUniformLocation(programa, "uAlphaMaximo");
    locPosicionDron     = glGetUniformLocation(programa, "uPosicionDron");
    locRadioNitido      = glGetUniformLocation(programa, "uRadioNitido");
    locRadioDesvanecido = glGetUniformLocation(programa, "uRadioDesvanecido");
    locPosicionCamara   = glGetUniformLocation(programa, "uPosicionCamara");
    locModo             = glGetUniformLocation(programa, "uModo");
    locPasada           = glGetUniformLocation(programa, "uPasada");
    locMinAltura        = glGetUniformLocation(programa, "uMinAltura");
    locMaxAltura        = glGetUniformLocation(programa, "uMaxAltura");
    locLimites          = glGetUniformLocation(programa, "uLimites");
    locCurvas           = glGetUniformLocation(programa, "uCurvas");
    locTiempo           = glGetUniformLocation(programa, "uTiempo");
    locExploracion      = glGetUniformLocation(programa, "uExploracion");
    locNiebla           = glGetUniformLocation(programa, "uNiebla");
    locRadioRevelado    = glGetUniformLocation(programa, "uRadioRevelado");

    glGenTextures(1, &texturaExploracion);
    glBindTexture(GL_TEXTURE_2D, texturaExploracion);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    const unsigned char vacio = 0;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, 1, 1, 0, GL_RED, GL_UNSIGNED_BYTE, &vacio);
}

void VistaTerreno::subirMalla(const Terreno& terreno) {
    if (vaoRejilla) { glDeleteVertexArrays(1, &vaoRejilla); glDeleteBuffers(1, &vboRejilla); glDeleteBuffers(1, &eboRejilla); }
    if (vaoCalles)  { glDeleteVertexArrays(1, &vaoCalles); glDeleteBuffers(1, &vboCalles); glDeleteBuffers(1, &eboCalles); }
    if (vaoSuperficie) { glDeleteVertexArrays(1, &vaoSuperficie); glDeleteBuffers(1, &vboSuperficie); glDeleteBuffers(1, &eboSuperficie); }
    vaoRejilla = vboRejilla = eboRejilla = 0;
    vaoCalles = vboCalles = eboCalles = 0;
    vaoSuperficie = vboSuperficie = eboSuperficie = 0;
    indicesCalles = indicesSuperficie = verticesRejilla = 0;
    revisionExploracion = ~std::uint64_t(0);

    if (terreno.esMallaDeLineas()) {
        subirLineasIndexadas(vaoCalles, vboCalles, eboCalles,
                             terreno.obtenerMalla().vertices, terreno.obtenerMalla().indices);
        indicesCalles = static_cast<int>(terreno.obtenerMalla().indices.size());
        verticesRejilla = static_cast<int>(terreno.obtenerMalla().numeroVertices());
        std::cout << "[TERRAIN] Calles en GPU: " << indicesCalles / 2 << " segmentos\n";
        return;
    }

    const Quadtree& qt = terreno.obtenerQuadtree();
    const MallaCruda& rejilla = terreno.obtenerRejilla();
    subirLineasIndexadas(vaoRejilla, vboRejilla, eboRejilla, rejilla.vertices, qt.obtenerIndices());
    verticesRejilla = static_cast<int>(rejilla.numeroVertices());

    const int R = terreno.obtenerResolucionRejilla();
    std::vector<float> vertices;
    vertices.reserve(static_cast<std::size_t>(R) * R * 6);
    auto posicion = [&](int x, int z) {
        x = std::max(0, std::min(R - 1, x));
        z = std::max(0, std::min(R - 1, z));
        std::size_t i = static_cast<std::size_t>(z * R + x) * 3;
        return glm::vec3(rejilla.vertices[i], rejilla.vertices[i + 1], rejilla.vertices[i + 2]);
    };
    for (int z = 0; z < R; ++z) for (int x = 0; x < R; ++x) {
        glm::vec3 p = posicion(x, z);
        glm::vec3 dx = posicion(x + 1, z) - posicion(x - 1, z);
        glm::vec3 dz = posicion(x, z + 1) - posicion(x, z - 1);
        glm::vec3 n = glm::normalize(glm::cross(dz, dx));
        vertices.insert(vertices.end(), {p.x, p.y, p.z, n.x, n.y, n.z});
    }
    std::vector<unsigned int> triangulos;
    triangulos.reserve(static_cast<std::size_t>(R - 1) * (R - 1) * 6);
    for (int z = 0; z + 1 < R; ++z) for (int x = 0; x + 1 < R; ++x) {
        unsigned int a = static_cast<unsigned int>(z * R + x);
        unsigned int b = a + 1;
        unsigned int c = a + static_cast<unsigned int>(R);
        unsigned int d = c + 1;
        triangulos.insert(triangulos.end(), {a, c, b, b, c, d});
    }
    glGenVertexArrays(1, &vaoSuperficie);
    glGenBuffers(1, &vboSuperficie);
    glGenBuffers(1, &eboSuperficie);
    glBindVertexArray(vaoSuperficie);
    glBindBuffer(GL_ARRAY_BUFFER, vboSuperficie);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboSuperficie);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(triangulos.size() * sizeof(unsigned int)), triangulos.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    indicesSuperficie = static_cast<int>(triangulos.size());

    std::cout << "[TERRAIN] Rejilla " << R << "x" << R << " | Quadtree "
              << qt.numeroNodos() << " nodos | superficie " << indicesSuperficie / 3
              << " triangulos\n";
}

void VistaTerreno::actualizarExploracion(const MapaExploracion& mapa) {
    if (!texturaExploracion || revisionExploracion == mapa.obtenerRevision()) return;
    const auto& mascara = mapa.obtenerMascara();
    if (mascara.empty()) return;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBindTexture(GL_TEXTURE_2D, texturaExploracion);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, mapa.obtenerAncho(), mapa.obtenerAlto(),
                 0, GL_RED, GL_UNSIGNED_BYTE, mascara.data());
    revisionExploracion = mapa.obtenerRevision();
}

int VistaTerreno::dibujar(const Camara& camara, const glm::mat4& matrizModelo,
                          const ComponenteMaterial& material, bool topologiaLineas,
                          const glm::vec3& posicionDron, float aspecto,
                          const Quadtree& quadtree, const std::vector<int>& nodosVisibles,
                          const Ajustes& ajustes, const EscalaMundo& escala,
                          const LimitesMundo& limites,
                          float minAltura, float maxAltura, float tiempo) {
    GLuint vaoLineas = topologiaLineas ? vaoCalles : vaoRejilla;
    if (!vaoLineas) return 0;

    glUseProgram(programa);
    glUniformMatrix4fv(locModelo, 1, GL_FALSE, glm::value_ptr(matrizModelo));
    glUniformMatrix4fv(locVista, 1, GL_FALSE, glm::value_ptr(camara.matrizVista()));
    glUniformMatrix4fv(locProyeccion, 1, GL_FALSE, glm::value_ptr(camara.matrizProyeccion(aspecto)));
    glUniform3fv(locColorBase, 1, glm::value_ptr(material.color));
    glUniform1f(locAlphaMaximo, material.alpha);
    glUniform3fv(locPosicionDron, 1, glm::value_ptr(posicionDron));
    glUniform3fv(locPosicionCamara, 1, glm::value_ptr(camara.obtenerPosicion()));
    // Los radios de atenuacion se miden contra la diagonal del terreno: en un
    // mapa el doble de grande la caida ocurre el doble de lejos.
    glUniform1f(locRadioNitido, escala.radioNitido);
    glUniform1f(locRadioDesvanecido, escala.radioDesvanecido);
    // Niebla proporcional: el relieve lejano se apaga sin llegar a desaparecer,
    // que es lo que da la sensacion de escala del terreno.
    glUniform2f(locNiebla, escala.diagonalTerreno * 0.55f, escala.diagonalTerreno * 2.6f);
    glUniform1f(locRadioRevelado, escala.radioEscaneo);
    glUniform1i(locModo, static_cast<int>(ajustes.visualizacion));
    glUniform1f(locMinAltura, minAltura);
    glUniform1f(locMaxAltura, maxAltura);
    glUniform4f(locLimites, limites.minX, limites.maxX, limites.minZ, limites.maxZ);
    glUniform1i(locCurvas, ajustes.curvasNivel ? 1 : 0);
    glUniform1f(locTiempo, tiempo);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, texturaExploracion);
    glUniform1i(locExploracion, 2);

    int drawCalls = 0;
    const ModoVisualizacion modo = ajustes.visualizacion;
    const bool dibujaSuperficie = !topologiaLineas &&
        (modo == ModoVisualizacion::SuperficieWireframe || modo == ModoVisualizacion::Elevacion ||
         modo == ModoVisualizacion::Escaneo);
    const bool dibujaLineas = modo == ModoVisualizacion::Wireframe ||
        modo == ModoVisualizacion::WireframePuntos || modo == ModoVisualizacion::SuperficieWireframe ||
        modo == ModoVisualizacion::Escaneo || topologiaLineas;
    const bool dibujaPuntos = modo == ModoVisualizacion::Puntos ||
        modo == ModoVisualizacion::WireframePuntos || modo == ModoVisualizacion::Escaneo;

    if (dibujaSuperficie && vaoSuperficie) {
        glUniform1i(locPasada, 2);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0f, 1.0f);
        glBindVertexArray(vaoSuperficie);
        glDrawElements(GL_TRIANGLES, indicesSuperficie, GL_UNSIGNED_INT, nullptr);
        glDisable(GL_POLYGON_OFFSET_FILL);
        ++drawCalls;
    }

    if (dibujaLineas) {
        glUniform1i(locPasada, 0);
        glBindVertexArray(vaoLineas);
        if (topologiaLineas) {
            glDrawElements(GL_LINES, indicesCalles, GL_UNSIGNED_INT, nullptr);
            ++drawCalls;
        } else {
            const auto& nodos = quadtree.obtenerNodos();
            int salto = ajustes.densidadWireframe == 0 ? 3 : (ajustes.densidadWireframe == 1 ? 2 : 1);
            for (std::size_t k = 0; k < nodosVisibles.size(); k += static_cast<std::size_t>(salto)) {
                const Quadtree::Nodo& nodo = nodos[nodosVisibles[k]];
                glDrawElements(GL_LINES, static_cast<GLsizei>(nodo.numIndices), GL_UNSIGNED_INT,
                               reinterpret_cast<void*>(static_cast<std::size_t>(nodo.offsetIndices) * sizeof(unsigned int)));
                ++drawCalls;
            }
        }
    }

    if (dibujaPuntos) {
        glUniform1i(locPasada, 1);
        glUniform1f(locAlphaMaximo, ajustes.intensidadPuntos);
        glEnable(GL_PROGRAM_POINT_SIZE);
        glBindVertexArray(vaoLineas);
        glDrawArrays(GL_POINTS, 0, verticesRejilla);
        ++drawCalls;
    }
    glBindVertexArray(0);
    glActiveTexture(GL_TEXTURE0);
    return drawCalls;
}

void VistaTerreno::liberar() {
    if (vaoRejilla) { glDeleteVertexArrays(1, &vaoRejilla); glDeleteBuffers(1, &vboRejilla); glDeleteBuffers(1, &eboRejilla); }
    if (vaoCalles) { glDeleteVertexArrays(1, &vaoCalles); glDeleteBuffers(1, &vboCalles); glDeleteBuffers(1, &eboCalles); }
    if (vaoSuperficie) { glDeleteVertexArrays(1, &vaoSuperficie); glDeleteBuffers(1, &vboSuperficie); glDeleteBuffers(1, &eboSuperficie); }
    if (texturaExploracion) glDeleteTextures(1, &texturaExploracion);
    vaoRejilla = vboRejilla = eboRejilla = vaoCalles = vboCalles = eboCalles = 0;
    vaoSuperficie = vboSuperficie = eboSuperficie = texturaExploracion = 0;
    programa = 0;
}
