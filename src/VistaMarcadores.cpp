#include "VistaMarcadores.h"
#include "Configuracion.h"
#include "GestorRecursos.h"

#include <glm/gtc/type_ptr.hpp>

void VistaMarcadores::inicializar(GestorRecursos& recursos) {
    programa            = recursos.obtenerPrograma("shaders/marcadores.vert", "shaders/marcadores.frag");
    locModelo           = glGetUniformLocation(programa, "model");
    locVista            = glGetUniformLocation(programa, "view");
    locProyeccion       = glGetUniformLocation(programa, "projection");
    locColorBase        = glGetUniformLocation(programa, "uColorBase");
    locAlphaMaximo      = glGetUniformLocation(programa, "uAlphaMaximo");
    locPosicionDron     = glGetUniformLocation(programa, "uPosicionDron");
    locRadioNitido      = glGetUniformLocation(programa, "uRadioNitido");
    locRadioDesvanecido = glGetUniformLocation(programa, "uRadioDesvanecido");

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    const GLsizei paso = 5 * sizeof(float);   // [x, y, z, desplazaX, desplazaY]
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, paso, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, paso, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBindVertexArray(0);
}

void VistaMarcadores::subirMarcadores(const MarcadoresSondeo& marcadores) {
    totalMarcadores = 0;
    baseCabezas = 0;
    const auto& lista = marcadores.obtener();
    if (lista.empty()) return;

    std::vector<float> datos;
    datos.reserve(lista.size() * (2 + 6) * 5);

    auto agregar = [&datos](const glm::vec3& p, float dx, float dy) {
        datos.push_back(p.x); datos.push_back(p.y); datos.push_back(p.z);
        datos.push_back(dx);  datos.push_back(dy);
    };

    // 1) Postes: dos vertices por estaca, sin desplazamiento de billboard.
    for (const auto& m : lista) {
        agregar(m.base, 0.0f, 0.0f);
        agregar(glm::vec3(m.base.x, m.base.y + m.altura, m.base.z), 0.0f, 0.0f);
    }
    baseCabezas = (int)(datos.size() / 5);

    // 2) Cabezas: quad encarando a la camara en la punta del poste. Se ancla
    //    siempre al mismo punto y son los desplazamientos los que abren el
    //    cuadrado, ya en espacio de vista (ver marcadores.vert).
    const float h = Configuracion::MARCADOR_TAM_CABEZA * 0.5f;
    const float esquinas[6][2] = {
        {-h, -h}, { h, -h}, { h,  h},
        {-h, -h}, { h,  h}, {-h,  h}
    };
    for (const auto& m : lista) {
        glm::vec3 punta(m.base.x, m.base.y + m.altura, m.base.z);
        for (int k = 0; k < 6; ++k) agregar(punta, esquinas[k][0], esquinas[k][1]);
    }
    totalMarcadores = (int)lista.size();

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(datos.size() * sizeof(float)),
                 datos.data(), GL_STATIC_DRAW);

    // El EBO se rellena cada frame con lo que sobreviva al frustum; se reserva
    // ya el tamaño maximo para no reasignar nunca.
    indicesPostes.reserve((std::size_t)totalMarcadores * 2);
    indicesCabezas.reserve((std::size_t)totalMarcadores * 6);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 (GLsizeiptr)((std::size_t)totalMarcadores * 8 * sizeof(unsigned int)),
                 nullptr, GL_DYNAMIC_DRAW);
}

int VistaMarcadores::dibujar(const Camara& camara, const glm::mat4& matrizModelo,
                             const glm::vec3& posicionDron, float aspecto,
                             const std::vector<int>& visibles) {
    if (!vao || totalMarcadores == 0 || visibles.empty()) return 0;

    // Indices de las estacas que pasaron el filtro del BVH.
    indicesPostes.clear();
    indicesCabezas.clear();
    for (int i : visibles) {
        indicesPostes.push_back((unsigned int)(i * 2));
        indicesPostes.push_back((unsigned int)(i * 2 + 1));
        for (int k = 0; k < 6; ++k)
            indicesCabezas.push_back((unsigned int)(baseCabezas + i * 6 + k));
    }

    glUseProgram(programa);
    glUniformMatrix4fv(locModelo,     1, GL_FALSE, glm::value_ptr(matrizModelo));
    glUniformMatrix4fv(locVista,      1, GL_FALSE, glm::value_ptr(camara.matrizVista()));
    glUniformMatrix4fv(locProyeccion, 1, GL_FALSE, glm::value_ptr(camara.matrizProyeccion(aspecto)));

    glm::vec3 blanco(1.0f);
    glUniform3fv(locColorBase, 1, glm::value_ptr(blanco));
    glUniform1f(locAlphaMaximo, Configuracion::ALPHA_MARCADORES);
    glUniform3fv(locPosicionDron, 1, glm::value_ptr(posicionDron));
    glUniform1f(locRadioNitido,      Configuracion::RADIO_NITIDO_MARCADOR);
    glUniform1f(locRadioDesvanecido, Configuracion::RADIO_DESVANECIDO_MARCADOR);

    glBindVertexArray(vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    int drawCalls = 0;

    // Ambos tramos se escriben en zonas DISTINTAS del mismo EBO antes de
    // dibujar: si se reutilizara el offset 0 para los dos, el driver tendria
    // que sincronizar entre el primer draw y la segunda escritura.
    const GLsizeiptr bytesPostes = (GLsizeiptr)(indicesPostes.size() * sizeof(unsigned int));
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, bytesPostes, indicesPostes.data());
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, bytesPostes,
                    (GLsizeiptr)(indicesCabezas.size() * sizeof(unsigned int)),
                    indicesCabezas.data());

    glDrawElements(GL_LINES, (GLsizei)indicesPostes.size(), GL_UNSIGNED_INT, 0);
    ++drawCalls;
    glDrawElements(GL_TRIANGLES, (GLsizei)indicesCabezas.size(), GL_UNSIGNED_INT,
                   (void*)(std::size_t)bytesPostes);
    ++drawCalls;

    glBindVertexArray(0);
    return drawCalls;
}

void VistaMarcadores::liberar() {
    if (vao) { glDeleteVertexArrays(1, &vao); glDeleteBuffers(1, &vbo); glDeleteBuffers(1, &ebo); }
    vao = vbo = ebo = 0;
    programa = 0;   // el programa lo posee el GestorRecursos
}
