#include "VistaCurvas.h"
#include "Configuracion.h"
#include "DisenoHUD.h"
#include "GestorRecursos.h"

#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

namespace {

// Rampa topografica clasica: azul (bajo) -> cian -> verde -> amarillo ->
// naranja -> rojo (alto). Se interpola linealmente entre las seis paradas.
glm::vec3 rampaAltura(float t) {
    static const glm::vec3 paradas[6] = {
        {0.13f, 0.35f, 0.95f},   // azul
        {0.10f, 0.85f, 0.95f},   // cian
        {0.22f, 0.90f, 0.35f},   // verde
        {0.98f, 0.90f, 0.15f},   // amarillo
        {1.00f, 0.55f, 0.10f},   // naranja
        {0.95f, 0.18f, 0.15f}    // rojo
    };
    t = std::clamp(t, 0.0f, 1.0f) * 5.0f;
    int i = std::min((int)t, 4);
    return glm::mix(paradas[i], paradas[i + 1], t - (float)i);
}

// Inclinacion del plano: ~60 grados en X para verlo en escorzo y un giro suave
// en Y que rompe la simetria y lo hace parecer una maqueta.
constexpr float GRADOS_INCLINACION = 52.0f;
constexpr float GRADOS_GIRO_Y      = -14.0f;
constexpr float DISTANCIA_CAMARA   = 3.0f;
constexpr float ESCALA_PLANO       = 1.00f;
constexpr float FOV_PANEL          = 38.0f;

} // namespace

void VistaCurvas::inicializar(GestorRecursos& recursos) {
    programa      = recursos.obtenerPrograma("shaders/curvas.vert", "shaders/curvas.frag");
    locModelo     = glGetUniformLocation(programa, "model");
    locVista      = glGetUniformLocation(programa, "view");
    locProyeccion = glGetUniformLocation(programa, "projection");
    locAlpha      = glGetUniformLocation(programa, "uAlpha");

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    const GLsizei paso = 5 * sizeof(float);   // [x, y, r, g, b]
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, paso, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, paso, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

void VistaCurvas::actualizarCurvas(const CurvasNivel& curvas, const LimitesMundo& limites) {
    verticesCurvas = verticesBorde = 0;

    std::vector<float> datos;
    auto agregar = [&datos](float x, float y, const glm::vec3& c) {
        datos.push_back(x); datos.push_back(y);
        datos.push_back(c.r); datos.push_back(c.g); datos.push_back(c.b);
    };
    // Mundo -> cuadrado [-1, 1] del plano. La Z del mundo se invierte para que
    // el norte del mapa quede arriba en el panel.
    auto aPlano = [&limites](const glm::vec2& p) {
        float u = (p.x - limites.minX) / limites.ancho()       * 2.0f - 1.0f;
        float v = (p.y - limites.minZ) / limites.profundidad() * 2.0f - 1.0f;
        return glm::vec2(u, -v);
    };

    // ---- Curvas: cada polilinea se trocea en segmentos para GL_LINES ----
    for (const auto& curva : curvas.obtenerCurvas()) {
        if (curva.puntos.size() < 2) continue;
        glm::vec3 color = rampaAltura(curvas.normalizar(curva.altura));
        for (std::size_t i = 0; i + 1 < curva.puntos.size(); ++i) {
            glm::vec2 a = aPlano(curva.puntos[i]);
            glm::vec2 b = aPlano(curva.puntos[i + 1]);
            agregar(a.x, a.y, color);
            agregar(b.x, b.y, color);
        }
    }
    verticesCurvas = (int)(datos.size() / 5);

    // ---- Marco del plano: cuadrilatero de lineas muy tenues ----
    const glm::vec3 gris = Paleta::TEXTO_SEC * 0.9f;
    const glm::vec2 esquinas[4] = {{-1,-1}, {1,-1}, {1,1}, {-1,1}};
    for (int i = 0; i < 4; ++i) {
        const glm::vec2& a = esquinas[i];
        const glm::vec2& b = esquinas[(i + 1) % 4];
        agregar(a.x, a.y, gris);
        agregar(b.x, b.y, gris);
    }
    verticesBorde = (int)(datos.size() / 5) - verticesCurvas;

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(datos.size() * sizeof(float)),
                 datos.data(), GL_DYNAMIC_DRAW);
}

int VistaCurvas::dibujar(int anchoPantalla, int altoPantalla) {
    if (!vao) return 0;

    glm::vec4 panel = DisenoHUD::rectPanelCurvas((float)anchoPantalla, (float)altoPantalla);
    // El HUD mide Y desde arriba y OpenGL desde abajo: hay que voltearla.
    GLint px = (GLint)panel.x;
    GLint py = (GLint)(altoPantalla - panel.y - panel.w);
    GLsizei pw = (GLsizei)panel.z;
    GLsizei ph = (GLsizei)panel.w;
    if (pw <= 0 || ph <= 0) return 0;

    glViewport(px, py, pw, ph);
    glEnable(GL_SCISSOR_TEST);
    glScissor(px, py, pw, ph);
    // El panel flota por encima de la escena: no debe competir con el z-buffer
    // del terreno ni escribir profundidad propia.
    glDisable(GL_DEPTH_TEST);

    glUseProgram(programa);
    glm::mat4 proyeccion = glm::perspective(glm::radians(FOV_PANEL),
                                            (float)pw / (float)ph, 0.1f, 50.0f);
    glm::mat4 vista = glm::lookAt(glm::vec3(0.0f, 0.0f, DISTANCIA_CAMARA),
                                  glm::vec3(0.0f), glm::vec3(0, 1, 0));
    glm::mat4 modelo = glm::rotate(glm::mat4(1.0f), glm::radians(GRADOS_INCLINACION), glm::vec3(1, 0, 0));
    modelo = glm::rotate(modelo, glm::radians(GRADOS_GIRO_Y), glm::vec3(0, 1, 0));
    modelo = glm::scale(modelo, glm::vec3(ESCALA_PLANO));

    glUniformMatrix4fv(locProyeccion, 1, GL_FALSE, glm::value_ptr(proyeccion));
    glUniformMatrix4fv(locVista,      1, GL_FALSE, glm::value_ptr(vista));
    glUniformMatrix4fv(locModelo,     1, GL_FALSE, glm::value_ptr(modelo));

    glBindVertexArray(vao);
    int drawCalls = 0;
    if (verticesBorde > 0) {
        glUniform1f(locAlpha, 0.35f);   // marco apenas insinuado
        glDrawArrays(GL_LINES, verticesCurvas, verticesBorde);
        ++drawCalls;
    }
    if (verticesCurvas > 0) {
        glUniform1f(locAlpha, 0.95f);
        glDrawArrays(GL_LINES, 0, verticesCurvas);
        ++drawCalls;
    }
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, anchoPantalla, altoPantalla);   // restaurar para el HUD
    return drawCalls;
}

void VistaCurvas::liberar() {
    if (vao) { glDeleteVertexArrays(1, &vao); glDeleteBuffers(1, &vbo); }
    vao = vbo = 0;
    programa = 0;   // el programa lo posee el GestorRecursos
}
