#include "VistaEscaner.h"
#include "Configuracion.h"
#include "GestorRecursos.h"
#include "Terreno.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/type_ptr.hpp>

namespace {
constexpr float DOS_PI = 6.28318530718f;
}

void VistaEscaner::inicializar(GestorRecursos& recursos) {
    programa      = recursos.obtenerPrograma("shaders/escaner.vert", "shaders/escaner.frag");
    locVista      = glGetUniformLocation(programa, "view");
    locProyeccion = glGetUniformLocation(programa, "projection");

    GLuint idVao = 0, idVbo = 0;
    glGenVertexArrays(1, &idVao); vao.adoptar(idVao);
    glGenBuffers(1, &idVbo);      vbo.adoptar(idVbo);

    glBindVertexArray(vao.obtener());
    glBindBuffer(GL_ARRAY_BUFFER, vbo.obtener());
    // Se reserva de una vez la capacidad maxima: a partir de aqui solo se
    // reescribe con glBufferSubData, nunca se reasigna memoria de GPU.
    glBufferData(GL_ARRAY_BUFFER, CAPACIDAD_VERTICES * sizeof(Vertice), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertice), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertice),
                          reinterpret_cast<void*>(sizeof(glm::vec3)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    buffer.reserve(CAPACIDAD_VERTICES);
}

int VistaEscaner::dibujar(const Camara& camara, const Terreno& terreno,
                          const EscalaMundo& escala, const glm::vec3& posicionDron,
                          float aspecto, bool activo) {
    if (!vao.obtener() || !activo || !terreno.estaCargado()) return 0;

    // ---- Geometria del haz --------------------------------------------------
    const float alturaSuelo = terreno.alturaEn(posicionDron.x, posicionDron.z);
    const float alturaHaz   = posicionDron.y - alturaSuelo;
    if (alturaHaz <= escala.alturaSegura * 0.5f) return 0;   // pegado al suelo: no hay haz

    // El radio de la huella crece con la altura (como un reflector real) pero
    // se topa en radioConoMaximo para que no acabe iluminando medio mapa.
    const float radio = std::min(alturaHaz * 0.45f, escala.radioConoMaximo);
    const float offsetSuelo = escala.alturaSegura * 0.06f;   // despega del relieve

    // Pulso suave y dependiente del tiempo acumulado con deltaTime.
    const float pulso = 0.86f + 0.14f * std::sin(tiempo * DOS_PI / Configuracion::PERIODO_PULSO);

    // Vertice del cono: justo bajo la panza del dron, no en su centro.
    const glm::vec3 vertice(posicionDron.x,
                            posicionDron.y - escala.radioDron * 0.55f,
                            posicionDron.z);

    const glm::vec3 luz = Paleta::LUZ_ESCANER;
    const int N = Configuracion::SEGMENTOS_CONO;

    buffer.clear();
    auto empujar = [&](const glm::vec3& p, float alpha) {
        buffer.push_back({p, glm::vec4(luz, alpha)});
    };
    // Punto del borde de la huella, siguiendo la altura REAL del relieve.
    auto borde = [&](float angulo, float r) {
        float x = posicionDron.x + std::cos(angulo) * r;
        float z = posicionDron.z + std::sin(angulo) * r;
        return glm::vec3(x, terreno.alturaEn(x, z) + offsetSuelo, z);
    };

    // ---- 1) Cono: mas intenso en el vertice, casi nulo en la base ----------
    const float alphaVertice = Configuracion::ALPHA_CONO * pulso;
    const int inicioCono = 0;
    for (int i = 0; i < N; ++i) {
        float a0 = DOS_PI * i / N;
        float a1 = DOS_PI * (i + 1) / N;
        empujar(vertice, alphaVertice);
        empujar(borde(a0, radio), 0.0f);
        empujar(borde(a1, radio), 0.0f);
    }
    const int cuentaCono = (int)buffer.size() - inicioCono;

    // ---- 2) Huella proyectada: abanico con caida radial --------------------
    const int inicioHuella = (int)buffer.size();
    const float alphaCentro = Configuracion::ALPHA_HUELLA * pulso;
    const glm::vec3 centroHuella(posicionDron.x, alturaSuelo + offsetSuelo, posicionDron.z);
    for (int i = 0; i < N; ++i) {
        float a0 = DOS_PI * i / N;
        float a1 = DOS_PI * (i + 1) / N;
        empujar(centroHuella, alphaCentro);
        empujar(borde(a0, radio), 0.0f);
        empujar(borde(a1, radio), 0.0f);
    }
    const int cuentaHuella = (int)buffer.size() - inicioHuella;

    // ---- 3) Anillos de radar expandiendose ---------------------------------
    const int inicioAnillos = (int)buffer.size();
    const int segmentosAnillo = N;
    for (int k = 0; k < Configuracion::ANILLOS_RADAR; ++k) {
        // Fase decalada por anillo: siempre hay uno naciendo y otro muriendo.
        float fase = std::fmod(tiempo / Configuracion::PERIODO_ANILLO +
                               (float)k / Configuracion::ANILLOS_RADAR, 1.0f);
        float radioAnillo = escala.radioEscaneo * fase;
        if (radioAnillo < 1e-3f) continue;
        // El alpha cae al expandirse: nace brillante en el centro y se apaga.
        float alphaAnillo = (1.0f - fase) * (1.0f - fase) * 0.75f;
        if (alphaAnillo < 0.004f) continue;

        for (int i = 0; i < segmentosAnillo; ++i) {
            float a0 = DOS_PI * i / segmentosAnillo;
            float a1 = DOS_PI * (i + 1) / segmentosAnillo;
            empujar(borde(a0, radioAnillo), alphaAnillo);
            empujar(borde(a1, radioAnillo), alphaAnillo);
        }
    }
    const int cuentaAnillos = (int)buffer.size() - inicioAnillos;

    if (buffer.empty()) return 0;
    const int total = std::min((int)buffer.size(), CAPACIDAD_VERTICES);

    glBindBuffer(GL_ARRAY_BUFFER, vbo.obtener());
    glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)(total * sizeof(Vertice)), buffer.data());

    // ---- Estado: mezcla aditiva, sin escribir profundidad ------------------
    // Aditiva = "suma luz"; sin depth write, los tres elementos se atraviesan
    // entre si sin recortarse, pero SI se comparan contra el z-buffer para que
    // el relieve delante del haz lo siga tapando.
    glUseProgram(programa);
    glUniformMatrix4fv(locVista,      1, GL_FALSE, glm::value_ptr(camara.matrizVista()));
    glUniformMatrix4fv(locProyeccion, 1, GL_FALSE, glm::value_ptr(camara.matrizProyeccion(aspecto)));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.5f, -1.5f);   // acerca la huella para no pelear con la rejilla

    glBindVertexArray(vao.obtener());
    int llamadas = 0;
    if (cuentaCono   > 0) { glDrawArrays(GL_TRIANGLES, inicioCono,   cuentaCono);   ++llamadas; }
    if (cuentaHuella > 0) { glDrawArrays(GL_TRIANGLES, inicioHuella, cuentaHuella); ++llamadas; }
    if (cuentaAnillos > 0) { glDrawArrays(GL_LINES,    inicioAnillos, cuentaAnillos); ++llamadas; }
    glBindVertexArray(0);

    glDisable(GL_POLYGON_OFFSET_FILL);
    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);   // se restaura el estado global
    return llamadas;
}

void VistaEscaner::liberar() {
    vao.reiniciar();
    vbo.reiniciar();
    programa = 0;   // el programa lo posee el GestorRecursos
}
