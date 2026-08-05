#include "VistaTerreno.h"
#include "Configuracion.h"
#include "GestorRecursos.h"

#include <glm/gtc/type_ptr.hpp>
#include <iostream>

namespace {
// Sube posiciones xyz + indices a un trio VAO/VBO/EBO recien creado.
void subirLineasIndexadas(GLuint& vao, GLuint& vbo, GLuint& ebo,
                          const std::vector<float>& vertices,
                          const std::vector<unsigned int>& indices) {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(vertices.size() * sizeof(float)),
                 vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(indices.size() * sizeof(unsigned int)),
                 indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}
} // namespace

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
}

void VistaTerreno::subirMalla(const Terreno& terreno) {
    // Los buffers del mapa anterior ya no sirven: se liberan antes de recrear.
    if (vaoRejilla) { glDeleteVertexArrays(1, &vaoRejilla); glDeleteBuffers(1, &vboRejilla); glDeleteBuffers(1, &eboRejilla); }
    if (vaoCalles)  { glDeleteVertexArrays(1, &vaoCalles);  glDeleteBuffers(1, &vboCalles);  glDeleteBuffers(1, &eboCalles); }
    vaoRejilla = vboRejilla = eboRejilla = vaoCalles = vboCalles = eboCalles = 0;
    indicesCalles = 0;

    if (terreno.esMallaDeLineas()) {
        // Mapa de calles: la cuadricula regular seria un plano plano y solo
        // taparia las calles, que son la informacion util.
        subirLineasIndexadas(vaoCalles, vboCalles, eboCalles,
                             terreno.obtenerMalla().vertices, terreno.obtenerMalla().indices);
        indicesCalles = (int)terreno.obtenerMalla().indices.size();
        std::cout << "  Calles en GPU: " << (indicesCalles / 2) << " segmentos\n";
    } else {
        // El EBO ya NO es el de la rejilla completa, sino la concatenacion de
        // los indices de todos los nodos del quadtree.
        const Quadtree& qt = terreno.obtenerQuadtree();
        subirLineasIndexadas(vaoRejilla, vboRejilla, eboRejilla,
                             terreno.obtenerRejilla().vertices, qt.obtenerIndices());
        std::cout << "  Rejilla: " << terreno.obtenerResolucionRejilla() << "x"
                  << terreno.obtenerResolucionRejilla()
                  << "  |  Quadtree: " << qt.numeroNodos() << " nodos, "
                  << (qt.obtenerIndices().size() / 2) << " segmentos indexados\n";
    }
}

int VistaTerreno::dibujar(const Camara& camara, const glm::mat4& matrizModelo,
                          const ComponenteMaterial& material, bool topologiaLineas,
                          const glm::vec3& posicionDron, float aspecto,
                          const Quadtree& quadtree, const std::vector<int>& nodosVisibles) {
    GLuint vao = topologiaLineas ? vaoCalles : vaoRejilla;
    if (!vao) return 0;

    glUseProgram(programa);
    glUniformMatrix4fv(locModelo,     1, GL_FALSE, glm::value_ptr(matrizModelo));
    glUniformMatrix4fv(locVista,      1, GL_FALSE, glm::value_ptr(camara.matrizVista()));
    glUniformMatrix4fv(locProyeccion, 1, GL_FALSE, glm::value_ptr(camara.matrizProyeccion(aspecto)));

    glUniform3fv(locColorBase, 1, glm::value_ptr(material.color));
    glUniform1f(locAlphaMaximo, material.alpha);
    glUniform3fv(locPosicionDron, 1, glm::value_ptr(posicionDron));
    glUniform1f(locRadioNitido,      Configuracion::RADIO_NITIDO);
    glUniform1f(locRadioDesvanecido, Configuracion::RADIO_DESVANECIDO);

    glBindVertexArray(vao);
    int drawCalls = 0;

    if (topologiaLineas) {
        glDrawElements(GL_LINES, indicesCalles, GL_UNSIGNED_INT, 0);
        ++drawCalls;
    } else {
        // Un draw call por cuadrante superviviente, apuntando a su tramo del EBO.
        const auto& nodos = quadtree.obtenerNodos();
        for (int indice : nodosVisibles) {
            const Quadtree::Nodo& nodo = nodos[indice];
            glDrawElements(GL_LINES, (GLsizei)nodo.numIndices, GL_UNSIGNED_INT,
                           (void*)(std::size_t)(nodo.offsetIndices * sizeof(unsigned int)));
            ++drawCalls;
        }
    }
    glBindVertexArray(0);
    return drawCalls;
}

void VistaTerreno::liberar() {
    if (vaoRejilla) { glDeleteVertexArrays(1, &vaoRejilla); glDeleteBuffers(1, &vboRejilla); glDeleteBuffers(1, &eboRejilla); }
    if (vaoCalles)  { glDeleteVertexArrays(1, &vaoCalles);  glDeleteBuffers(1, &vboCalles);  glDeleteBuffers(1, &eboCalles); }
    vaoRejilla = vboRejilla = eboRejilla = vaoCalles = vboCalles = eboCalles = 0;
    programa = 0;   // el programa lo posee el GestorRecursos
}
