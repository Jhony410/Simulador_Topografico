#include "VistaMinimapa.h"
#include "Configuracion.h"
#include "GestorRecursos.h"
#include "MapaExploracion.h"
#include "SistemaExploracion.h"
#include "Terreno.h"

#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

namespace {
struct VerticeIcono { glm::vec2 posicion; glm::vec4 color; float tamano; };
}

void VistaMinimapa::inicializar(GestorRecursos& recursos) {
    programaMapa = recursos.obtenerPrograma("shaders/minimapa.vert", "shaders/minimapa.frag");
    programaIconos = recursos.obtenerPrograma("shaders/minimapa_iconos.vert", "shaders/minimapa_iconos.frag");
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
    glBufferData(GL_ARRAY_BUFFER, 256 * sizeof(VerticeIcono), nullptr, GL_DYNAMIC_DRAW);
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
    glBindTexture(GL_TEXTURE_2D, texturaAltura);
    int lado = terreno.obtenerAnchoGrilla();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, lado, lado, 0, GL_RED, GL_UNSIGNED_BYTE, normalizadas.data());
    revisionMascara = ~std::uint64_t(0);
}

void VistaMinimapa::actualizarExploracion(const MapaExploracion& mapa) {
    if (revisionMascara == mapa.obtenerRevision() || mapa.obtenerMascara().empty()) return;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBindTexture(GL_TEXTURE_2D, texturaMascara);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, mapa.obtenerAncho(), mapa.obtenerAlto(),
                 0, GL_RED, GL_UNSIGNED_BYTE, mapa.obtenerMascara().data());
    revisionMascara = mapa.obtenerRevision();
}

int VistaMinimapa::dibujar(const Terreno& terreno, const MapaExploracion& mapa,
                           const SistemaExploracion& exploracion,
                           const glm::vec3& posicionDron, float yawDron,
                           int anchoPantalla, int altoPantalla, bool visible) {
    if (!visible || !texturaAltura || anchoPantalla <= 0 || altoPantalla <= 0) return 0;
    actualizarExploracion(mapa);
    const float x = 24.0f, y = altoPantalla - 302.0f, w = 270.0f, h = 174.0f;
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
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, texturaAltura);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, texturaMascara);
    glBindBuffer(GL_ARRAY_BUFFER, vboMapa);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBindVertexArray(vaoMapa);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    const LimitesMundo& lim = terreno.obtenerLimites();
    auto aPantalla = [&](const glm::vec3& p) {
        float u = (p.x - lim.minX) / lim.ancho();
        float v = (p.z - lim.minZ) / lim.profundidad();
        return glm::vec2(x + std::clamp(u, 0.0f, 1.0f) * w,
                         y + std::clamp(v, 0.0f, 1.0f) * h);
    };
    std::vector<VerticeIcono> iconos;
    for (int i = 0; i < static_cast<int>(exploracion.obtenerPuntos().size()); ++i) {
        const auto& p = exploracion.obtenerPuntos()[i];
        glm::vec4 color = p.estado == EstadoPuntoEscaneo::Completado
                        ? glm::vec4(Paleta::VERDE, 1.0f)
                        : (i == exploracion.obtenerObjetivoActual()
                           ? glm::vec4(Paleta::ACENTO, 1.0f)
                           : glm::vec4(Paleta::TEXTO_SEC, 0.82f));
        iconos.push_back({aPantalla(p.posicion), color, i == exploracion.obtenerObjetivoActual() ? 8.0f : 5.0f});
    }
    glm::vec2 d = aPantalla(posicionDron);
    iconos.push_back({d, glm::vec4(Paleta::BLANCO, 1.0f), 9.0f});

    std::vector<VerticeIcono> lineas;
    glm::vec4 borde(Paleta::REJILLA_SEC, 0.8f);
    auto agregarLinea = [&](glm::vec2 a, glm::vec2 b, glm::vec4 c) {
        lineas.push_back({a,c,1}); lineas.push_back({b,c,1});
    };
    agregarLinea({x,y},{x+w,y},borde); agregarLinea({x+w,y},{x+w,y+h},borde);
    agregarLinea({x+w,y+h},{x,y+h},borde); agregarLinea({x,y+h},{x,y},borde);
    float ang = glm::radians(yawDron);
    glm::vec2 frente(std::sin(ang), std::cos(ang));
    glm::vec2 lateral(frente.y, -frente.x);
    agregarLinea(d + frente * 10.0f, d - frente * 6.0f + lateral * 5.0f, glm::vec4(Paleta::ACENTO,1));
    agregarLinea(d + frente * 10.0f, d - frente * 6.0f - lateral * 5.0f, glm::vec4(Paleta::ACENTO,1));
    agregarLinea(d - frente * 6.0f + lateral * 5.0f, d - frente * 6.0f - lateral * 5.0f, glm::vec4(Paleta::ACENTO,1));

    std::vector<VerticeIcono> datos = lineas;
    datos.insert(datos.end(), iconos.begin(), iconos.end());
    glBindBuffer(GL_ARRAY_BUFFER, vboIconos);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(datos.size() * sizeof(VerticeIcono)), datos.data(), GL_DYNAMIC_DRAW);
    glUseProgram(programaIconos);
    glUniformMatrix4fv(glGetUniformLocation(programaIconos, "uProj"), 1, GL_FALSE, glm::value_ptr(orto));
    glBindVertexArray(vaoIconos);
    int llamadas = 1;
    if (!lineas.empty()) { glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(lineas.size())); ++llamadas; }
    if (!iconos.empty()) {
        glEnable(GL_PROGRAM_POINT_SIZE);
        glDrawArrays(GL_POINTS, static_cast<GLint>(lineas.size()), static_cast<GLsizei>(iconos.size()));
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
