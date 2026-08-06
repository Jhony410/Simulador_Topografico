#include "VistaMarcadores.h"
#include "Configuracion.h"
#include "GestorRecursos.h"
#include "MapaExploracion.h"

#include <glm/gtc/type_ptr.hpp>

void VistaMarcadores::inicializar(GestorRecursos& recursos) {
    programa            = recursos.obtenerPrograma("shaders/marcadores.vert", "shaders/marcadores.frag");
    locModelo           = glGetUniformLocation(programa, "model");
    locVista            = glGetUniformLocation(programa, "view");
    locProyeccion       = glGetUniformLocation(programa, "projection");
    locAlphaMaximo      = glGetUniformLocation(programa, "uAlphaMaximo");
    locPosicionDron     = glGetUniformLocation(programa, "uPosicionDron");
    locRadioNitido      = glGetUniformLocation(programa, "uRadioNitido");
    locRadioDesvanecido = glGetUniformLocation(programa, "uRadioDesvanecido");
    locRadioEscaneo     = glGetUniformLocation(programa, "uRadioEscaneo");
    locLimites          = glGetUniformLocation(programa, "uLimites");
    locMascara          = glGetUniformLocation(programa, "uMascara");

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    // [x, y, z, desplazaX, desplazaY, alturaRelativa]
    const GLsizei paso = 6 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, paso, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, paso, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, paso, (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBindVertexArray(0);

    // Mascara propia: evita acoplar esta vista con VistaTerreno solo para leer
    // una textura de un canal de 256x256 (64 KB).
    glGenTextures(1, &texturaMascara);
    glBindTexture(GL_TEXTURE_2D, texturaMascara);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    const unsigned char vacio = 0;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, 1, 1, 0, GL_RED, GL_UNSIGNED_BYTE, &vacio);
}

void VistaMarcadores::subirMarcadores(const MarcadoresSondeo& marcadores,
                                      const LimitesMundo& limites) {
    limitesMundo = glm::vec4(limites.minX, limites.maxX, limites.minZ, limites.maxZ);
    totalMarcadores = 0;
    baseLuces = 0;
    revisionMascara = ~std::uint64_t(0);
    const auto& lista = marcadores.obtener();
    if (lista.empty()) return;

    std::vector<float> datos;
    datos.reserve(lista.size() * (VERTICES_ARMAZON + VERTICES_LUZ) * 6);

    auto agregar = [&datos](const glm::vec3& p, float dx, float dy, float alturaRel) {
        datos.push_back(p.x); datos.push_back(p.y); datos.push_back(p.z);
        datos.push_back(dx);  datos.push_back(dy);  datos.push_back(alturaRel);
    };

    // ---- 1) Armazon: 18 vertices por torreta, todos con desplazamiento 0 ----
    // La geometria se construye DESDE LA BASE hacia arriba, nunca desde un
    // centro: asi el pivote coincide con el punto de apoyo en el relieve y la
    // torreta no puede quedar medio enterrada ni medio flotando.
    for (const auto& m : lista) {
        const glm::vec3 b = m.base;
        const float h = m.altura;
        const float a = m.ancho;

        auto punto = [&](float dx, float dy, float dz) {
            return glm::vec3(b.x + dx, b.y + dy, b.z + dz);
        };
        auto segmento = [&](const glm::vec3& p, const glm::vec3& q) {
            agregar(p, 0.0f, 0.0f, (p.y - b.y) / h);
            agregar(q, 0.0f, 0.0f, (q.y - b.y) / h);
        };

        // Mastil (2 vertices)
        segmento(punto(0, 0, 0), punto(0, h, 0));

        // Cuatro tirantes de base: dan la lectura de torre y no de palo (8)
        const float alturaTirante = h * 0.22f;
        const float extension = a * 2.4f;
        segmento(punto(-extension, 0, 0), punto(0, alturaTirante, 0));
        segmento(punto( extension, 0, 0), punto(0, alturaTirante, 0));
        segmento(punto(0, 0, -extension), punto(0, alturaTirante, 0));
        segmento(punto(0, 0,  extension), punto(0, alturaTirante, 0));

        // Dos crucetas horizontales, como las antenas de la referencia (8)
        for (float f : {0.58f, 0.84f}) {
            segmento(punto(-a * 1.6f, h * f, 0), punto(a * 1.6f, h * f, 0));
            segmento(punto(0, h * f, -a * 1.6f), punto(0, h * f, a * 1.6f));
        }
    }
    baseLuces = (int)(datos.size() / 6);

    // ---- 2) Luz de punta: billboard MINIMO en la cima del mastil -----------
    // Se ancla siempre al mismo punto y son los desplazamientos los que abren
    // el cuadrado, ya en espacio de vista (ver marcadores.vert).
    const float esquinas[6][2] = {
        {-1.0f, -1.0f}, { 1.0f, -1.0f}, { 1.0f,  1.0f},
        {-1.0f, -1.0f}, { 1.0f,  1.0f}, {-1.0f,  1.0f}
    };
    for (const auto& m : lista) {
        glm::vec3 punta(m.base.x, m.base.y + m.altura, m.base.z);
        const float lado = m.ancho * 1.4f;
        for (int k = 0; k < 6; ++k)
            agregar(punta, esquinas[k][0] * lado, esquinas[k][1] * lado, 1.0f);
    }
    totalMarcadores = (int)lista.size();

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(datos.size() * sizeof(float)),
                 datos.data(), GL_STATIC_DRAW);

    // El EBO se rellena cada frame con lo que sobreviva al frustum; se reserva
    // ya el tamano maximo para no reasignar nunca.
    indicesArmazon.reserve((std::size_t)totalMarcadores * VERTICES_ARMAZON);
    indicesLuces.reserve((std::size_t)totalMarcadores * VERTICES_LUZ);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 (GLsizeiptr)((std::size_t)totalMarcadores *
                              (VERTICES_ARMAZON + VERTICES_LUZ) * sizeof(unsigned int)),
                 nullptr, GL_DYNAMIC_DRAW);
}

void VistaMarcadores::actualizarExploracion(const MapaExploracion& mapa) {
    if (revisionMascara == mapa.obtenerRevision() || mapa.obtenerMascara().empty()) return;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBindTexture(GL_TEXTURE_2D, texturaMascara);
    if (revisionMascara == ~std::uint64_t(0)) {
        // Primera subida tras cambiar de mapa: hay que (re)dimensionar.
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, mapa.obtenerAncho(), mapa.obtenerAlto(),
                     0, GL_RED, GL_UNSIGNED_BYTE, mapa.obtenerMascara().data());
    } else {
        // El resto de frames solo se reescribe el contenido: la textura ya
        // existe con el tamano correcto y no hay que reasignar memoria de GPU.
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mapa.obtenerAncho(), mapa.obtenerAlto(),
                        GL_RED, GL_UNSIGNED_BYTE, mapa.obtenerMascara().data());
    }
    revisionMascara = mapa.obtenerRevision();
}

int VistaMarcadores::dibujar(const Camara& camara, const glm::mat4& matrizModelo,
                             const glm::vec3& posicionDron, float aspecto,
                             const EscalaMundo& escala, const std::vector<int>& visibles) {
    if (!vao || totalMarcadores == 0 || visibles.empty()) return 0;

    // Indices de las torretas que pasaron el filtro del BVH.
    indicesArmazon.clear();
    indicesLuces.clear();
    for (int i : visibles) {
        for (int k = 0; k < VERTICES_ARMAZON; ++k)
            indicesArmazon.push_back((unsigned int)(i * VERTICES_ARMAZON + k));
        for (int k = 0; k < VERTICES_LUZ; ++k)
            indicesLuces.push_back((unsigned int)(baseLuces + i * VERTICES_LUZ + k));
    }

    glUseProgram(programa);
    glUniformMatrix4fv(locModelo,     1, GL_FALSE, glm::value_ptr(matrizModelo));
    glUniformMatrix4fv(locVista,      1, GL_FALSE, glm::value_ptr(camara.matrizVista()));
    glUniformMatrix4fv(locProyeccion, 1, GL_FALSE, glm::value_ptr(camara.matrizProyeccion(aspecto)));

    glUniform1f(locAlphaMaximo, Configuracion::ALPHA_MARCADORES);
    glUniform3fv(locPosicionDron, 1, glm::value_ptr(posicionDron));
    glUniform1f(locRadioNitido,      escala.radioNitidoMarcador);
    glUniform1f(locRadioDesvanecido, escala.radioDesvanecidoMarcador);
    glUniform1f(locRadioEscaneo,     escala.radioEscaneo);
    glUniform4fv(locLimites, 1, glm::value_ptr(limitesMundo));
    glUniform1i(locMascara, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texturaMascara);

    glBindVertexArray(vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    int drawCalls = 0;

    // Ambos tramos se escriben en zonas DISTINTAS del mismo EBO antes de
    // dibujar: si se reutilizara el offset 0 para los dos, el driver tendria
    // que sincronizar entre el primer draw y la segunda escritura.
    const GLsizeiptr bytesArmazon = (GLsizeiptr)(indicesArmazon.size() * sizeof(unsigned int));
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, bytesArmazon, indicesArmazon.data());
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, bytesArmazon,
                    (GLsizeiptr)(indicesLuces.size() * sizeof(unsigned int)),
                    indicesLuces.data());

    glDrawElements(GL_LINES, (GLsizei)indicesArmazon.size(), GL_UNSIGNED_INT, 0);
    ++drawCalls;
    glDrawElements(GL_TRIANGLES, (GLsizei)indicesLuces.size(), GL_UNSIGNED_INT,
                   (void*)(std::size_t)bytesArmazon);
    ++drawCalls;

    glBindVertexArray(0);
    return drawCalls;
}

void VistaMarcadores::liberar() {
    if (vao) { glDeleteVertexArrays(1, &vao); glDeleteBuffers(1, &vbo); glDeleteBuffers(1, &ebo); }
    if (texturaMascara) glDeleteTextures(1, &texturaMascara);
    vao = vbo = ebo = texturaMascara = 0;
    programa = 0;   // el programa lo posee el GestorRecursos
}
