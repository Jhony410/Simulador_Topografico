#include "VistaMinimapa.h"
#include "Configuracion.h"
#include "GestorRecursos.h"
#include "MapaExploracion.h"
#include "Terreno.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

namespace {
struct VerticeIcono { glm::vec2 posicion; glm::vec4 color; float tamano; };
constexpr int CAPACIDAD_ICONOS = 256;
}

glm::vec4 VistaMinimapa::rectangulo(float anchoPantalla, float altoPantalla) {
    (void)anchoPantalla;
    // Esquina inferior izquierda, discreto: 210x136 px. Debajo quedan libres
    // 40 px para el porcentaje y la barra fina de progreso.
    const float ancho = 210.0f, alto = 136.0f;
    return glm::vec4(24.0f, altoPantalla - alto - 62.0f, ancho, alto);
}

void VistaMinimapa::inicializar(GestorRecursos& recursos) {
    programaMapa   = recursos.obtenerPrograma("shaders/minimapa.vert", "shaders/minimapa.frag");
    programaIconos = recursos.obtenerPrograma("shaders/minimapa_iconos.vert", "shaders/minimapa_iconos.frag");
    locNiveles     = glGetUniformLocation(programaMapa, "uNiveles");

    glGenVertexArrays(1, &vaoMapa);
    glGenBuffers(1, &vboMapa);
    glBindVertexArray(vaoMapa);
    glBindBuffer(GL_ARRAY_BUFFER, vboMapa);
    glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glGenVertexArrays(1, &vaoIconos);
    glGenBuffers(1, &vboIconos);
    glBindVertexArray(vaoIconos);
    glBindBuffer(GL_ARRAY_BUFFER, vboIconos);
    // Capacidad fija: a partir de aqui solo glBufferSubData, nunca reasignacion.
    glBufferData(GL_ARRAY_BUFFER, CAPACIDAD_ICONOS * sizeof(VerticeIcono), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(VerticeIcono), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(VerticeIcono), reinterpret_cast<void*>(sizeof(glm::vec2)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(VerticeIcono), reinterpret_cast<void*>(sizeof(glm::vec2) + sizeof(glm::vec4)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    glGenTextures(1, &texturaAltura);
    glGenTextures(1, &texturaMascara);
    for (GLuint tex : {texturaAltura, texturaMascara}) {
        glBindTexture(GL_TEXTURE_2D, tex);
        // LINEAR en la mascara no es un detalle estetico: es lo que suaviza el
        // borde del revelado sin tener que dilatar la mascara en CPU.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
}

void VistaMinimapa::subirTerreno(const Terreno& terreno) {
    const auto& alturas = terreno.obtenerAlturas();
    if (alturas.empty()) return;
    const LimitesMundo& lim = terreno.obtenerLimites();
    std::vector<unsigned char> normalizadas(alturas.size());
    float rango = std::max(1e-5f, lim.maxY - lim.minY);
    for (std::size_t i = 0; i < alturas.size(); ++i)
        normalizadas[i] = static_cast<unsigned char>(std::clamp((alturas[i] - lim.minY) / rango, 0.0f, 1.0f) * 255.0f);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    int lado = terreno.obtenerAnchoGrilla();
    glBindTexture(GL_TEXTURE_2D, texturaAltura);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, lado, lado, 0, GL_RED, GL_UNSIGNED_BYTE, normalizadas.data());

    // La mascara se asigna aqui, a cero: el minimapa arranca OCULTO y solo se
    // revela con lo que el dron vaya explorando.
    std::vector<unsigned char> vacia((std::size_t)lado * lado, 0);
    glBindTexture(GL_TEXTURE_2D, texturaMascara);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, lado, lado, 0, GL_RED, GL_UNSIGNED_BYTE, vacia.data());
    anchoMascara = altoMascara = lado;
    revisionMascara = ~std::uint64_t(0);
}

void VistaMinimapa::actualizarExploracion(const MapaExploracion& mapa) {
    // Sin cambios en el Modelo no se toca la GPU: el guardia de revision es lo
    // que evita subir 64 KB de textura en cada uno de los 60 frames por segundo.
    if (revisionMascara == mapa.obtenerRevision() || mapa.obtenerMascara().empty()) return;
    if (mapa.obtenerAncho() != anchoMascara || mapa.obtenerAlto() != altoMascara) return;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBindTexture(GL_TEXTURE_2D, texturaMascara);
    // La textura ya existe con el tamano correcto: solo se reescribe contenido.
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, anchoMascara, altoMascara,
                    GL_RED, GL_UNSIGNED_BYTE, mapa.obtenerMascara().data());
    revisionMascara = mapa.obtenerRevision();
}

int VistaMinimapa::dibujar(const Terreno& terreno, const MapaExploracion& mapa,
                           const glm::vec3& posicionDron, float yawDron,
                           int anchoPantalla, int altoPantalla, bool visible) {
    if (!visible || !texturaAltura || anchoPantalla <= 0 || altoPantalla <= 0) return 0;
    actualizarExploracion(mapa);

    const glm::vec4 r = rectangulo((float)anchoPantalla, (float)altoPantalla);
    const float x = r.x, y = r.y, w = r.z, h = r.w;
    const float vertices[] = {
        x,y,0,0, x+w,y,1,0, x+w,y+h,1,1,
        x,y,0,0, x+w,y+h,1,1, x,y+h,0,1
    };
    glm::mat4 orto = glm::ortho(0.0f, static_cast<float>(anchoPantalla),
                                static_cast<float>(altoPantalla), 0.0f, -1.0f, 1.0f);
    glDisable(GL_DEPTH_TEST);
    glUseProgram(programaMapa);
    glUniformMatrix4fv(glGetUniformLocation(programaMapa, "uProj"), 1, GL_FALSE, glm::value_ptr(orto));
    glUniform1i(glGetUniformLocation(programaMapa, "uAltura"), 0);
    glUniform1i(glGetUniformLocation(programaMapa, "uMascara"), 1);
    if (locNiveles >= 0) glUniform1f(locNiveles, (float)Configuracion::NIVELES_CURVAS);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, texturaAltura);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, texturaMascara);
    glBindBuffer(GL_ARRAY_BUFFER, vboMapa);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBindVertexArray(vaoMapa);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // ---- Marco, dron y orientacion ----------------------------------------
    const LimitesMundo& lim = terreno.obtenerLimites();
    auto aPantalla = [&](const glm::vec3& p) {
        float u = (p.x - lim.minX) / lim.ancho();
        float v = (p.z - lim.minZ) / lim.profundidad();
        return glm::vec2(x + std::clamp(u, 0.0f, 1.0f) * w,
                         y + std::clamp(v, 0.0f, 1.0f) * h);
    };

    std::vector<VerticeIcono> lineas;
    glm::vec4 borde(Paleta::REJILLA_SEC, 0.75f);
    auto agregarLinea = [&](glm::vec2 a, glm::vec2 b, glm::vec4 c) {
        lineas.push_back({a,c,1}); lineas.push_back({b,c,1});
    };
    agregarLinea({x,y},{x+w,y},borde);       agregarLinea({x+w,y},{x+w,y+h},borde);
    agregarLinea({x+w,y+h},{x,y+h},borde);   agregarLinea({x,y+h},{x,y},borde);

    // Triangulo de orientacion del dron, en amarillo.
    glm::vec2 d = aPantalla(posicionDron);
    float ang = glm::radians(yawDron);
    glm::vec2 frente(std::sin(ang), std::cos(ang));
    glm::vec2 lateral(frente.y, -frente.x);
    glm::vec4 acento(Paleta::ACENTO, 1.0f);
    agregarLinea(d + frente * 7.0f, d - frente * 4.0f + lateral * 3.5f, acento);
    agregarLinea(d + frente * 7.0f, d - frente * 4.0f - lateral * 3.5f, acento);
    agregarLinea(d - frente * 4.0f + lateral * 3.5f, d - frente * 4.0f - lateral * 3.5f, acento);

    std::vector<VerticeIcono> puntos;
    puntos.push_back({d, acento, 5.0f});   // posicion del dron

    std::vector<VerticeIcono> datos = lineas;
    datos.insert(datos.end(), puntos.begin(), puntos.end());
    if (datos.size() > CAPACIDAD_ICONOS) datos.resize(CAPACIDAD_ICONOS);

    glBindBuffer(GL_ARRAY_BUFFER, vboIconos);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    static_cast<GLsizeiptr>(datos.size() * sizeof(VerticeIcono)), datos.data());
    glUseProgram(programaIconos);
    glUniformMatrix4fv(glGetUniformLocation(programaIconos, "uProj"), 1, GL_FALSE, glm::value_ptr(orto));
    glBindVertexArray(vaoIconos);
    int llamadas = 1;
    if (!lineas.empty()) { glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(lineas.size())); ++llamadas; }
    if (!puntos.empty()) {
        glEnable(GL_PROGRAM_POINT_SIZE);
        glDrawArrays(GL_POINTS, static_cast<GLint>(lineas.size()), static_cast<GLsizei>(puntos.size()));
        ++llamadas;
    }
    glBindVertexArray(0);
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_DEPTH_TEST);
    return llamadas;
}

void VistaMinimapa::liberar() {
    if (vaoMapa) glDeleteVertexArrays(1, &vaoMapa);
    if (vboMapa) glDeleteBuffers(1, &vboMapa);
    if (vaoIconos) glDeleteVertexArrays(1, &vaoIconos);
    if (vboIconos) glDeleteBuffers(1, &vboIconos);
    if (texturaAltura) glDeleteTextures(1, &texturaAltura);
    if (texturaMascara) glDeleteTextures(1, &texturaMascara);
    vaoMapa = vboMapa = vaoIconos = vboIconos = texturaAltura = texturaMascara = 0;
    programaMapa = programaIconos = 0;
}
