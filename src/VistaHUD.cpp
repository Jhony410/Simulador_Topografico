#include "VistaHUD.h"
#include "Configuracion.h"
#include "DisenoHUD.h"
#include "ContadorRendimiento.h"
#include "Escena.h"
#include "GestorRecursos.h"

#include "stb_easy_font.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

namespace {

// stb_easy_font solo conoce ASCII 32..126: cualquier vocal acentuada ocuparia
// dos bytes UTF-8 fuera de ese rango y saldria como basura. Por eso los textos
// van en mayusculas sin tilde hasta que se sustituya por un atlas de fuente.
const char* MISION_LINEA1 = "EXPLORA EL AREA PARA DETERMINAR LAS DIMENSIONES";
const char* MISION_LINEA2 = "Y LA TOPOGRAFIA DEL SITIO";
const char* COMPLETA_LINEA1 = "HAS MAPEADO EL AREA COMPLETA";
const char* COMPLETA_LINEA2 = "TOPOGRAFIA RECONSTRUIDA CON EXITO";

// Lineas cortadas a mano a <= 32 caracteres: es lo que cabe en el ancho del
// panel a escala 1.35, y stb_easy_font no sabe hacer saltos de linea.
const char* PARRAFO_PANEL[] = {
    "EL BARRIDO DEL DRON CUBRIO LA",
    "TOTALIDAD DE LA SUPERFICIE.",
    "",
    "EL MAPA DE ALTURAS ESTA COMPLETO",
    "Y LAS CURVAS DE NIVEL DESCRIBEN",
    "TODO EL RELIEVE DEL SITIO."
};
constexpr int LINEAS_PARRAFO = (int)(sizeof(PARRAFO_PANEL) / sizeof(PARRAFO_PANEL[0]));

constexpr float ESPACIADO_TITULO = 3.2f;   // interletraje amplio del texto de mision
constexpr float ESPACIADO_ETIQUETA = 1.6f; // el del resto de rotulos del HUD
constexpr float ALTO_GLIFO       = 7.0f;   // alto nominal de stb_easy_font

} // namespace

void VistaHUD::inicializar(GestorRecursos& recursos) {
    programa      = recursos.obtenerPrograma("shaders/hud.vert", "shaders/hud.frag");
    locProyeccion = glGetUniformLocation(programa, "uProj");
    locModelo     = glGetUniformLocation(programa, "uModel");
    locColor      = glGetUniformLocation(programa, "uColor");

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    bufferTexto.resize(80000);
}

void VistaHUD::dibujarRectangulo(float x, float y, float ancho, float alto, const glm::vec4& color) {
    float v[12] = { x, y,  x + ancho, y,  x + ancho, y + alto,
                    x, y,  x + ancho, y + alto,  x, y + alto };
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_DYNAMIC_DRAW);

    glm::mat4 identidad(1.0f);
    glUniformMatrix4fv(locModelo, 1, GL_FALSE, glm::value_ptr(identidad));
    glUniform4fv(locColor, 1, glm::value_ptr(color));
    glDrawArrays(GL_TRIANGLES, 0, 6);
    ++drawCalls;
}

float VistaHUD::anchoTexto(const char* texto, float escala, float espaciado) const {
    float total = 0.0f;
    char glifo[2] = {0, 0};
    for (const char* c = texto; *c; ++c) {
        glifo[0] = *c;
        total += stb_easy_font_width(glifo) + espaciado;
    }
    if (total > 0.0f) total -= espaciado;   // el ultimo glifo no arrastra hueco
    return total * escala;
}

void VistaHUD::dibujarTexto(float x, float y, float escala, const char* texto,
                            const glm::vec4& color, float espaciado) {
    unsigned char blanco[4] = {255, 255, 255, 255};
    std::vector<float> triangulos;
    const int orden[6] = {0, 1, 2, 0, 2, 3};

    // Un glifo por llamada para poder inyectar el interletraje en el avance.
    // stb_easy_font_print ya acepta el origen, asi que el desplazamiento queda
    // horneado en los vertices y todo se envia junto al final.
    float cursor = 0.0f;
    char glifo[2] = {0, 0};
    for (const char* c = texto; *c; ++c) {
        glifo[0] = *c;
        int numQuads = stb_easy_font_print(cursor, 0.0f, glifo, blanco,
                                           bufferTexto.data(), (int)bufferTexto.size());
        const char* base = bufferTexto.data();
        for (int q = 0; q < numQuads; q++) {
            float esquina[4][2];
            for (int k = 0; k < 4; k++) {
                const float* f = reinterpret_cast<const float*>(base + (q * 4 + k) * 16);
                esquina[k][0] = f[0];
                esquina[k][1] = f[1];
            }
            for (int k = 0; k < 6; k++) {
                triangulos.push_back(esquina[orden[k]][0]);
                triangulos.push_back(esquina[orden[k]][1]);
            }
        }
        cursor += stb_easy_font_width(glifo) + espaciado;
    }
    if (triangulos.empty()) return;

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(triangulos.size() * sizeof(float)),
                 triangulos.data(), GL_DYNAMIC_DRAW);

    glm::mat4 modelo = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f)) *
                       glm::scale(glm::mat4(1.0f), glm::vec3(escala, escala, 1.0f));
    glUniformMatrix4fv(locModelo, 1, GL_FALSE, glm::value_ptr(modelo));
    glUniform4fv(locColor, 1, glm::value_ptr(color));
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(triangulos.size() / 2));
    ++drawCalls;
}

void VistaHUD::dibujarLineaGruesa(float x1, float y1, float x2, float y2,
                                  float grosor, const glm::vec4& color) {
    // Un segmento grueso es un rectangulo girado: se construye desplazando los
    // extremos a lo largo de la normal del propio segmento.
    float dx = x2 - x1, dy = y2 - y1;
    float longitud = std::sqrt(dx * dx + dy * dy);
    if (longitud < 1e-4f) return;
    float nx = -dy / longitud * grosor * 0.5f;
    float ny =  dx / longitud * grosor * 0.5f;

    float v[12] = {
        x1 + nx, y1 + ny,  x2 + nx, y2 + ny,  x2 - nx, y2 - ny,
        x1 + nx, y1 + ny,  x2 - nx, y2 - ny,  x1 - nx, y1 - ny
    };
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_DYNAMIC_DRAW);
    glm::mat4 identidad(1.0f);
    glUniformMatrix4fv(locModelo, 1, GL_FALSE, glm::value_ptr(identidad));
    glUniform4fv(locColor, 1, glm::value_ptr(color));
    glDrawArrays(GL_TRIANGLES, 0, 6);
    ++drawCalls;
}

void VistaHUD::dibujarAnillo(float cx, float cy, float radio, float grosor,
                             const glm::vec4& color, int segmentos) {
    // Corona teselada en tiras de dos triangulos. Se manda entera en un solo
    // draw call en vez de dibujar N segmentos sueltos.
    std::vector<float> v;
    v.reserve((std::size_t)segmentos * 12);
    const float DOS_PI = 6.28318530718f;
    float interior = radio - grosor;
    for (int i = 0; i < segmentos; ++i) {
        float a0 = DOS_PI * (float)i / segmentos;
        float a1 = DOS_PI * (float)(i + 1) / segmentos;
        float c0 = std::cos(a0), s0 = std::sin(a0);
        float c1 = std::cos(a1), s1 = std::sin(a1);

        float ex0 = cx + c0 * radio,    ey0 = cy + s0 * radio;
        float ex1 = cx + c1 * radio,    ey1 = cy + s1 * radio;
        float ix0 = cx + c0 * interior, iy0 = cy + s0 * interior;
        float ix1 = cx + c1 * interior, iy1 = cy + s1 * interior;

        float tri[12] = { ix0, iy0, ex0, ey0, ex1, ey1,
                          ix0, iy0, ex1, ey1, ix1, iy1 };
        v.insert(v.end(), tri, tri + 12);
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(v.size() * sizeof(float)), v.data(), GL_DYNAMIC_DRAW);
    glm::mat4 identidad(1.0f);
    glUniformMatrix4fv(locModelo, 1, GL_FALSE, glm::value_ptr(identidad));
    glUniform4fv(locColor, 1, glm::value_ptr(color));
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(v.size() / 2));
    ++drawCalls;
}

void VistaHUD::dibujarTecla(const glm::vec4& r, const char* etiqueta, bool presionada) {
    // Al pulsar, borde y relleno suben de luminosidad a la vez: el salto se lee
    // aunque la tecla sea pequena.
    glm::vec4 borde = presionada ? glm::vec4(0.85f, 0.89f, 0.95f, 1.0f)
                                 : glm::vec4(Paleta::TEXTO_SEC, 0.55f);
    glm::vec4 fondo = presionada ? glm::vec4(0.26f, 0.30f, 0.38f, 0.9f)
                                 : glm::vec4(0.05f, 0.07f, 0.11f, 0.55f);
    dibujarRectangulo(r.x, r.y, r.z, r.w, borde);
    dibujarRectangulo(r.x + 1.5f, r.y + 1.5f, r.z - 3.0f, r.w - 3.0f, fondo);

    const float escala = 1.6f;
    glm::vec4 colorTexto = presionada ? glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)
                                      : glm::vec4(Paleta::TEXTO_SEC, 1.0f);
    dibujarTexto(r.x + (r.z - anchoTexto(etiqueta, escala, ESPACIADO_ETIQUETA)) * 0.5f,
                 r.y + (r.w - ALTO_GLIFO * escala) * 0.5f,
                 escala, etiqueta, colorTexto, ESPACIADO_ETIQUETA);
}

void VistaHUD::dibujarBoton(const glm::vec4& r, const char* etiqueta, bool activo, bool resaltado) {
    glm::vec4 borde = activo ? glm::vec4(Paleta::ACENTO, 1.0f)
                             : glm::vec4(0.5f, 0.55f, 0.6f, 0.9f);
    glm::vec4 fondo = activo ? glm::vec4(Paleta::ACENTO, 0.25f)
                             : (resaltado ? glm::vec4(0.3f, 0.34f, 0.4f, 0.85f)
                                          : glm::vec4(0.07f, 0.09f, 0.13f, 0.8f));
    // El "borde" es el rectangulo de atras; el fondo va 2 px adentro.
    dibujarRectangulo(r.x, r.y, r.z, r.w, borde);
    dibujarRectangulo(r.x + 2, r.y + 2, r.z - 4, r.w - 4, fondo);

    const float escala = 2.0f;
    glm::vec4 colorTexto = activo ? glm::vec4(1.0f, 0.9f, 0.3f, 1.0f)
                                  : glm::vec4(0.85f, 0.9f, 0.95f, 1.0f);
    dibujarTexto(r.x + (r.z - anchoTexto(etiqueta, escala)) * 0.5f,
                 r.y + (r.w - ALTO_GLIFO * escala) * 0.5f,
                 escala, etiqueta, colorTexto);
}

// ---------------------------------------------------------------------------
void VistaHUD::dibujarBarraProgreso(const EstadoMision& mision,
                                    float anchoPantalla, float altoPantalla) {
    glm::vec4 carril = DisenoHUD::rectBarraProgreso(anchoPantalla, altoPantalla);
    float progreso = mision.obtenerProgreso();

    // Carril apagado de fondo, para que se lea cuanto falta.
    dibujarRectangulo(carril.x, carril.y, carril.z, carril.w,
                      glm::vec4(Paleta::TEXTO_SEC, 0.25f));
    // Tramo recorrido.
    if (progreso > 0.0f)
        dibujarRectangulo(carril.x, carril.y, carril.z * progreso, carril.w,
                          glm::vec4(Paleta::ACENTO, 1.0f));

    // El porcentaje VIAJA con el extremo de la barra, no esta fijo.
    char etiqueta[16];
    std::snprintf(etiqueta, sizeof(etiqueta), "%d%%", mision.obtenerPorcentaje());

    const float escala = 2.6f;
    float x = carril.x + carril.z * progreso + 10.0f;
    // Al 100% el texto se saldria del carril: se frena contra el borde util.
    x = std::min(x, anchoPantalla - anchoTexto(etiqueta, escala) - 12.0f);
    dibujarTexto(x, carril.y - ALTO_GLIFO * escala - 4.0f, escala, etiqueta,
                 glm::vec4(Paleta::ACENTO, 1.0f));
}

void VistaHUD::dibujarTextoMision(const EstadoMision& mision,
                                  float anchoPantalla, float altoPantalla) {
    glm::vec4 panel = DisenoHUD::rectPanelMision(anchoPantalla, altoPantalla);
    dibujarRectangulo(panel.x, panel.y, panel.z, panel.w, glm::vec4(0.02f, 0.03f, 0.05f, 0.55f));

    const char* linea1 = mision.estaCompletada() ? COMPLETA_LINEA1 : MISION_LINEA1;
    const char* linea2 = mision.estaCompletada() ? COMPLETA_LINEA2 : MISION_LINEA2;
    glm::vec4 color = mision.estaCompletada() ? glm::vec4(Paleta::ACENTO, 1.0f)
                                              : glm::vec4(0.90f, 0.93f, 0.97f, 1.0f);

    const float escala = 1.6f;
    const float altoLinea = ALTO_GLIFO * escala + 12.0f;
    float y = panel.y + (panel.w - 2.0f * altoLinea + 12.0f) * 0.5f;

    dibujarTexto(panel.x + (panel.z - anchoTexto(linea1, escala, ESPACIADO_TITULO)) * 0.5f,
                 y, escala, linea1, color, ESPACIADO_TITULO);
    dibujarTexto(panel.x + (panel.z - anchoTexto(linea2, escala, ESPACIADO_TITULO)) * 0.5f,
                 y + altoLinea, escala, linea2, color, ESPACIADO_TITULO);
}

void VistaHUD::dibujarPanelLateral(const EstadoMision& mision,
                                   float anchoPantalla, float altoPantalla) {
    float alpha = mision.obtenerAlphaPanel();
    if (alpha < 0.004f) return;

    glm::vec4 panel = DisenoHUD::rectPanelLateral(anchoPantalla, altoPantalla);
    // Entra deslizandose desde la derecha mientras sube la opacidad.
    float x = panel.x + (1.0f - alpha) * 46.0f;

    dibujarRectangulo(x, panel.y, panel.z, panel.w, glm::vec4(0.03f, 0.04f, 0.07f, 0.82f * alpha));

    // Linea amarilla corta encima del bloque de texto.
    dibujarRectangulo(x + 22.0f, panel.y + 24.0f, 44.0f, 2.0f, glm::vec4(Paleta::ACENTO, alpha));

    const float escalaTitulo = 2.0f;
    dibujarTexto(x + 22.0f, panel.y + 42.0f, escalaTitulo, "INFORME DE SONDEO",
                 glm::vec4(0.92f, 0.95f, 0.99f, alpha), 2.4f);

    const float escala = 1.35f;
    const float altoLinea = ALTO_GLIFO * escala + 9.0f;
    float y = panel.y + 78.0f;
    for (int i = 0; i < LINEAS_PARRAFO; ++i) {
        if (PARRAFO_PANEL[i][0] != '\0')
            dibujarTexto(x + 22.0f, y, escala, PARRAFO_PANEL[i],
                         glm::vec4(Paleta::TEXTO_SEC, alpha), 0.6f);
        y += altoLinea;
    }
}

void VistaHUD::dibujarEsquinas(float anchoPantalla, float altoPantalla) {
    // ---- Arriba a la izquierda: marca amarilla vertical + titulo ----
    dibujarRectangulo(28.0f, 30.0f, 4.0f, 17.0f, glm::vec4(Paleta::ACENTO, 1.0f));
    // Escala contenida a proposito: el texto de mision empieza en el centro de
    // la pantalla y con interletraje amplio este rotulo llegaria a solaparlo.
    dibujarTexto(44.0f, 34.0f, 1.4f, Configuracion::TITULO_APP.c_str(),
                 glm::vec4(0.92f, 0.95f, 0.99f, 1.0f), 2.0f);

    // ---- Arriba a la derecha: circulo amarillo con X (cierra la aplicacion) ----
    glm::vec4 salir = DisenoHUD::rectBotonSalir(anchoPantalla, altoPantalla);
    float cx = salir.x + salir.z * 0.5f, cy = salir.y + salir.w * 0.5f;
    float radio = salir.z * 0.5f;
    dibujarAnillo(cx, cy, radio, 1.6f, glm::vec4(Paleta::ACENTO, 1.0f));
    const float brazo = radio * 0.36f;
    dibujarLineaGruesa(cx - brazo, cy - brazo, cx + brazo, cy + brazo, 1.8f,
                       glm::vec4(Paleta::ACENTO, 1.0f));
    dibujarLineaGruesa(cx - brazo, cy + brazo, cx + brazo, cy - brazo, 1.8f,
                       glm::vec4(Paleta::ACENTO, 1.0f));

    // ---- Icono de menu: tres lineas ----
    glm::vec4 menu = DisenoHUD::rectIconoMenu(anchoPantalla, altoPantalla);
    for (int i = 0; i < 3; ++i)
        dibujarRectangulo(menu.x, menu.y + i * (menu.w * 0.5f), menu.z, 2.0f,
                          glm::vec4(0.85f, 0.89f, 0.95f, 0.9f));

    // ---- Abajo a la izquierda: pie del proyecto ----
    dibujarRectangulo(24.0f, altoPantalla - 48.0f, 3.0f, 14.0f, glm::vec4(Paleta::ACENTO, 1.0f));
    dibujarTexto(36.0f, altoPantalla - 45.0f, 1.5f, Configuracion::PIE_PROYECTO.c_str(),
                 glm::vec4(Paleta::TEXTO_SEC, 1.0f), ESPACIADO_ETIQUETA);
}

void VistaHUD::dibujarClusterTeclas(const EstadoTeclas& teclas,
                                    float anchoPantalla, float altoPantalla) {
    auto rect = [&](int i) { return DisenoHUD::rectTecla(i, anchoPantalla, altoPantalla); };

    dibujarTecla(rect(0), "^",     teclas.arriba);
    dibujarTecla(rect(1), "<",     teclas.izquierda);
    dibujarTecla(rect(2), "v",     teclas.abajo);
    dibujarTecla(rect(3), ">",     teclas.derecha);
    dibujarTecla(rect(4), "SHIFT", teclas.shift);
    dibujarTecla(rect(5), "SPACE", teclas.espacio);

    // Rotulo encima del cluster, alineado con su borde izquierdo.
    glm::vec4 arriba = rect(0);
    glm::vec4 izquierda = rect(1);
    dibujarTexto(izquierda.x, arriba.y - 17.0f, 1.4f, "CONTROLES",
                 glm::vec4(Paleta::TEXTO_SEC, 0.9f), ESPACIADO_ETIQUETA);
}

void VistaHUD::dibujarAviso(const Escena& escena, float anchoPantalla, float altoPantalla) {
    const AvisoHUD& aviso = escena.obtenerAviso();
    if (!aviso.visible()) return;
    (void)altoPantalla;

    float alpha = aviso.obtenerAlpha();
    const char* texto = aviso.obtenerTexto().c_str();
    const float escala = 1.7f;
    float ancho = anchoTexto(texto, escala, ESPACIADO_ETIQUETA);

    // Justo debajo del panel de mision, centrado: es donde el ojo ya esta.
    float x = (anchoPantalla - ancho) * 0.5f;
    float y = 112.0f;
    dibujarRectangulo(x - 16.0f, y - 8.0f, ancho + 32.0f, ALTO_GLIFO * escala + 16.0f,
                      glm::vec4(0.02f, 0.03f, 0.05f, 0.75f * alpha));
    dibujarTexto(x, y, escala, texto, glm::vec4(Paleta::ACENTO, alpha), ESPACIADO_ETIQUETA);
}

void VistaHUD::dibujarMetricas(const ContadorRendimiento& metricas,
                               float anchoPantalla, float altoPantalla) {
    (void)altoPantalla;
    // Arriba a la derecha, en gris: son datos de diagnostico, no de la mision.
    char linea[64];
    // Bajo el icono de menu: arriba al centro manda el texto de mision, que no
    // debe competir con cifras de diagnostico.
    const float escala = 1.3f;
    const float x = anchoPantalla - 178.0f;
    float y = 122.0f;
    const float salto = 15.0f;
    const glm::vec4 gris(Paleta::TEXTO_SEC, 0.85f);

    std::snprintf(linea, sizeof(linea), "FPS %.0f", metricas.obtenerFPS());
    dibujarTexto(x, y, escala, linea, gris, 1.0f);           y += salto;

    std::snprintf(linea, sizeof(linea), "DRAW CALLS %d", metricas.obtenerDrawCalls());
    dibujarTexto(x, y, escala, linea, gris, 1.0f);           y += salto;

    std::snprintf(linea, sizeof(linea), "NODOS LOD %d", metricas.obtenerNodosTerreno());
    dibujarTexto(x, y, escala, linea, gris, 1.0f);           y += salto;

    std::snprintf(linea, sizeof(linea), "LINEAS %d/%d",
                  metricas.obtenerSegmentosDibujados(), metricas.obtenerSegmentosTotales());
    dibujarTexto(x, y, escala, linea, gris, 1.0f);
}

// ---------------------------------------------------------------------------
int VistaHUD::dibujar(Escena& escena, const EstadoTeclas& teclas,
                      float anchoPantalla, float altoPantalla,
                      const ContadorRendimiento& metricas) {
    drawCalls = 0;
    glDisable(GL_DEPTH_TEST);
    glUseProgram(programa);

    // Ortografica con Y hacia abajo: coordenadas de pantalla directas.
    glm::mat4 orto = glm::ortho(0.0f, anchoPantalla, altoPantalla, 0.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(locProyeccion, 1, GL_FALSE, glm::value_ptr(orto));
    glBindVertexArray(vao);

    const EstadoMision& mision = escena.obtenerEstadoMision();
    dibujarTextoMision(mision, anchoPantalla, altoPantalla);
    dibujarBarraProgreso(mision, anchoPantalla, altoPantalla);
    dibujarPanelLateral(mision, anchoPantalla, altoPantalla);

    // ---- Rotulos de esquina ----
    dibujarEsquinas(anchoPantalla, altoPantalla);

    // ---- Botones de mapa (centrados abajo) ----
    const auto& mapas = escena.obtenerMapas();
    for (int i = 0; i < (int)mapas.size() && i < 9; i++) {
        dibujarBoton(DisenoHUD::rectBotonMapa(i, anchoPantalla, altoPantalla),
                     std::to_string(i + 1).c_str(), i == escena.obtenerMapaActual(), false);
    }
    {
        glm::vec4 primero = DisenoHUD::rectBotonMapa(0, anchoPantalla, altoPantalla);
        dibujarTexto(primero.x, primero.y - 21.0f, 1.4f, "MAPAS",
                     glm::vec4(Paleta::TEXTO_SEC, 0.9f), ESPACIADO_ETIQUETA);
    }

    // ---- Cluster de teclas de vuelo ----
    dibujarClusterTeclas(teclas, anchoPantalla, altoPantalla);

    dibujarMetricas(metricas, anchoPantalla, altoPantalla);
    dibujarAviso(escena, anchoPantalla, altoPantalla);

    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
    return drawCalls;
}

void VistaHUD::liberar() {
    if (vao) { glDeleteVertexArrays(1, &vao); glDeleteBuffers(1, &vbo); }
    vao = vbo = 0;
    programa = 0;   // el programa lo posee el GestorRecursos
}
