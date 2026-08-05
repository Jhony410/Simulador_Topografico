#include "VistaDron.h"
#include "GestorRecursos.h"

#include <glm/gtc/type_ptr.hpp>

void VistaDron::inicializar(GestorRecursos& recursos) {
    programa      = recursos.obtenerPrograma("shaders/dron.vert", "shaders/dron.frag");
    locModelo     = glGetUniformLocation(programa, "model");
    locVista      = glGetUniformLocation(programa, "view");
    locProyeccion = glGetUniformLocation(programa, "projection");
    locTiempo     = glGetUniformLocation(programa, "uTime");
    locGiro       = glGetUniformLocation(programa, "uSpin");
    locPivotes    = glGetUniformLocation(programa, "uPivots");
    locColor      = glGetUniformLocation(programa, "uColor");
    locAlpha      = glGetUniformLocation(programa, "uAlpha");
    locEmision    = glGetUniformLocation(programa, "uEmision");
}

void VistaDron::subirMalla(const MallaCruda& malla, const glm::vec3 pivotes[4]) {
    if (malla.vacia()) return;
    for (int i = 0; i < 4; i++) pivotesHelices[i] = pivotes[i];

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &eboTriangulos);
    glGenBuffers(1, &eboAristas);
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(malla.vertices.size() * sizeof(float)),
                 malla.vertices.data(), GL_STATIC_DRAW);

    // Dos indexados sobre el MISMO VBO: relleno por triangulos y armazon por
    // aristas. Comparten vertices, asi que el giro de las helices que hace el
    // vertex shader se aplica igual a los dos.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboTriangulos);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(malla.indices.size() * sizeof(unsigned int)),
                 malla.indices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboAristas);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(malla.aristas.size() * sizeof(unsigned int)),
                 malla.aristas.data(), GL_STATIC_DRAW);

    const GLsizei paso = 4 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, paso, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, paso, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    numeroIndices = (int)malla.indices.size();
    numeroAristas = (int)malla.aristas.size();
}

int VistaDron::dibujar(const Camara& camara, const glm::mat4& matrizModelo,
                        const ComponenteMaterial& material, const ComponenteAnimacion& animacion,
                        float aspecto) {
    if (!vao) return 0;

    int drawCalls = 0;
    glUseProgram(programa);
    glUniformMatrix4fv(locModelo,     1, GL_FALSE, glm::value_ptr(matrizModelo));
    glUniformMatrix4fv(locVista,      1, GL_FALSE, glm::value_ptr(camara.matrizVista()));
    glUniformMatrix4fv(locProyeccion, 1, GL_FALSE, glm::value_ptr(camara.matrizProyeccion(aspecto)));
    glUniform1f(locTiempo, animacion.tiempo);
    glUniform1f(locGiro,   animacion.velocidadGiro);
    glUniform3fv(locPivotes, 4, glm::value_ptr(pivotesHelices[0]));
    glBindVertexArray(vao);

    if (material.dibujarRelleno) {
        // Pasada 1: relleno oscuro. El polygon offset lo empuja hacia atras
        // para que las aristas de la pasada 2 no peleen con el en el z-buffer.
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0f, 1.0f);
        glUniform3fv(locColor, 1, glm::value_ptr(material.colorRelleno));
        glUniform1f(locAlpha, material.alphaRelleno);
        glUniform1f(locEmision, 0.0f);   // el relleno oscuro no debe brillar
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboTriangulos);
        glDrawElements(GL_TRIANGLES, numeroIndices, GL_UNSIGNED_INT, 0);
        ++drawCalls;
        glDisable(GL_POLYGON_OFFSET_FILL);
    }

    // Pasada 2: aristas, que son las que dan la estructura del dron y las
    // unicas que alimentan el buffer de brillo.
    glUniform3fv(locColor, 1, glm::value_ptr(material.color));
    glUniform1f(locAlpha, material.alpha);
    glUniform1f(locEmision, material.emision);
    if (numeroAristas > 0) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboAristas);
        glDrawElements(GL_LINES, numeroAristas, GL_UNSIGNED_INT, 0);
        ++drawCalls;
    }

    glBindVertexArray(0);
    return drawCalls;
}

void VistaDron::liberar() {
    if (vao) {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &eboTriangulos);
        glDeleteBuffers(1, &eboAristas);
    }
    vao = vbo = eboTriangulos = eboAristas = 0;
    programa = 0;   // el programa lo posee el GestorRecursos
}
