#include "VistaExploracion.h"
#include "Configuracion.h"
#include "GestorRecursos.h"
#include "Terreno.h"

#include <cmath>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

namespace {
struct VerticeGuia {
    glm::vec3 posicion;
    glm::vec4 color;
    float tamano;
};

glm::vec4 colorEstado(EstadoPuntoEscaneo estado) {
    switch (estado) {
        case EstadoPuntoEscaneo::Pendiente:   return glm::vec4(Paleta::TEXTO_SEC, 0.68f);
        case EstadoPuntoEscaneo::Cercano:     return glm::vec4(Paleta::ACENTO, 0.95f);
        case EstadoPuntoEscaneo::Escaneando:  return glm::vec4(Paleta::CIAN, 1.0f);
        case EstadoPuntoEscaneo::Completado:  return glm::vec4(Paleta::VERDE, 0.86f);
    }
    return glm::vec4(1.0f);
}
}

void VistaExploracion::inicializar(GestorRecursos& recursos) {
    programa = recursos.obtenerPrograma("shaders/exploracion.vert", "shaders/exploracion.frag");
    locVista = glGetUniformLocation(programa, "view");
    locProyeccion = glGetUniformLocation(programa, "projection");
    GLuint idVao=0,idVbo=0;
    glGenVertexArrays(1, &idVao); vao.adoptar(idVao);
    glGenBuffers(1, &idVbo); vbo.adoptar(idVbo);
    glBindVertexArray(vao.obtener());
    glBindBuffer(GL_ARRAY_BUFFER, vbo.obtener());
    glBufferData(GL_ARRAY_BUFFER, 4096 * sizeof(VerticeGuia), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VerticeGuia), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(VerticeGuia), reinterpret_cast<void*>(sizeof(glm::vec3)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(VerticeGuia), reinterpret_cast<void*>(sizeof(glm::vec3) + sizeof(glm::vec4)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

int VistaExploracion::dibujar(const Camara& camara, const Terreno& terreno,
                              const SistemaExploracion& exploracion,
                              const SistemaMedicion& medicion, const Ajustes& ajustes,
                              const glm::vec3& posicionDron, float aspecto, float tiempo) {
    if (!vao) return 0;
    std::vector<VerticeGuia> lineas;
    std::vector<VerticeGuia> puntos;
    lineas.reserve(2048);
    puntos.reserve(64);
    auto linea = [&](glm::vec3 a, glm::vec3 b, glm::vec4 color) {
        lineas.push_back({a, color, 1.0f});
        lineas.push_back({b, color, 1.0f});
    };

    for (const PuntoEscaneo& p : exploracion.obtenerPuntos()) {
        glm::vec4 color = colorEstado(p.estado);
        float pulso = 1.0f + ((p.estado == EstadoPuntoEscaneo::Cercano ||
                              p.estado == EstadoPuntoEscaneo::Escaneando)
                             ? 0.12f * std::sin(tiempo * 5.0f + p.id) : 0.0f);
        glm::vec3 base = p.posicion + glm::vec3(0.0f, 0.12f, 0.0f);
        float altura = p.estado == EstadoPuntoEscaneo::Completado ? 3.0f : 7.5f;
        linea(base, base + glm::vec3(0.0f, altura, 0.0f), color);
        float cruz = 0.48f * pulso;
        glm::vec3 cima = base + glm::vec3(0.0f, altura, 0.0f);
        linea(cima - glm::vec3(cruz, 0, 0), cima + glm::vec3(cruz, 0, 0), color);
        linea(cima - glm::vec3(0, 0, cruz), cima + glm::vec3(0, 0, cruz), color);
        puntos.push_back({cima, color, p.estado == EstadoPuntoEscaneo::Escaneando ? 8.0f : 6.0f});

        int segmentos = 48;
        int visibles = p.estado == EstadoPuntoEscaneo::Escaneando
                     ? std::max(1, static_cast<int>(segmentos * p.progreso)) : segmentos;
        float radio = p.radio * pulso;
        glm::vec4 colorAnillo = color;
        colorAnillo.a *= p.estado == EstadoPuntoEscaneo::Pendiente ? 0.38f : 0.85f;
        for (int i = 0; i < visibles; ++i) {
            float a0 = 6.283185307f * i / segmentos;
            float a1 = 6.283185307f * (i + 1) / segmentos;
            glm::vec3 q0(base.x + std::cos(a0) * radio,
                         terreno.alturaEn(base.x + std::cos(a0) * radio,
                                          base.z + std::sin(a0) * radio) + 0.16f,
                         base.z + std::sin(a0) * radio);
            glm::vec3 q1(base.x + std::cos(a1) * radio,
                         terreno.alturaEn(base.x + std::cos(a1) * radio,
                                          base.z + std::sin(a1) * radio) + 0.16f,
                         base.z + std::sin(a1) * radio);
            linea(q0, q1, colorAnillo);
        }
    }

    if (ajustes.rutaVuelo && exploracion.obtenerObjetivoActual() >= 0) {
        const glm::vec3 destino = exploracion.obtenerPuntos()[exploracion.obtenerObjetivoActual()].posicion;
        glm::vec4 ruta(Paleta::ACENTO, 0.42f);
        glm::vec3 anterior = posicionDron;
        const int tramos = 24;
        for (int i = 1; i <= tramos; ++i) {
            float t = static_cast<float>(i) / tramos;
            glm::vec3 actual = glm::mix(posicionDron, destino, t);
            actual.y = std::max(actual.y, terreno.alturaEn(actual.x, actual.z) + 1.0f);
            if ((i / 2) % 2 == 0) linea(anterior, actual, ruta);
            anterior = actual;
        }
    }

    if (medicion.estaActivo()) {
        const auto& medidos = medicion.obtenerPuntos();
        glm::vec4 c(Paleta::CIAN, 0.96f);
        for (std::size_t i = 0; i < medidos.size(); ++i) {
            puntos.push_back({medidos[i] + glm::vec3(0, 0.18f, 0), c, 9.0f});
            linea(medidos[i], medidos[i] + glm::vec3(0, 2.0f, 0), c);
            if (i > 0) linea(medidos[i - 1] + glm::vec3(0, 0.22f, 0),
                             medidos[i] + glm::vec3(0, 0.22f, 0), c);
        }
    }

    if (lineas.empty() && puntos.empty()) return 0;
    std::vector<VerticeGuia> datos = lineas;
    datos.insert(datos.end(), puntos.begin(), puntos.end());
    glBindBuffer(GL_ARRAY_BUFFER, vbo.obtener());
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(datos.size() * sizeof(VerticeGuia)),
                 datos.data(), GL_DYNAMIC_DRAW);

    glUseProgram(programa);
    glUniformMatrix4fv(locVista, 1, GL_FALSE, glm::value_ptr(camara.matrizVista()));
    glUniformMatrix4fv(locProyeccion, 1, GL_FALSE, glm::value_ptr(camara.matrizProyeccion(aspecto)));
    glBindVertexArray(vao.obtener());
    int llamadas = 0;
    if (!lineas.empty()) { glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(lineas.size())); ++llamadas; }
    if (!puntos.empty()) {
        glEnable(GL_PROGRAM_POINT_SIZE);
        glDrawArrays(GL_POINTS, static_cast<GLint>(lineas.size()), static_cast<GLsizei>(puntos.size()));
        ++llamadas;
    }
    glBindVertexArray(0);
    return llamadas;
}

void VistaExploracion::liberar() {
    vao.reiniciar();
    vbo.reiniciar();
    programa = 0;
}
