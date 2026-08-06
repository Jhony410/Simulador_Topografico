#include "VistaHUD.h"
#include "Configuracion.h"
#include "DisenoHUD.h"
#include "ContadorRendimiento.h"
#include "Camara.h"
#include "Escena.h"
#include "GestorRecursos.h"
#include "MenuConfiguracion.h"
#include "VistaMinimapa.h"

#include <filesystem>

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
// van en mayusculas sin tilde.
constexpr float ESPACIADO_TITULO   = 2.4f;
constexpr float ESPACIADO_ETIQUETA = 1.2f;
constexpr float ALTO_GLIFO         = 7.0f;   // alto nominal de stb_easy_font

// Ayuda en pantalla. Dos columnas explicitas (tecla / accion) para que alguien
// que abre el programa por primera vez sepa pilotar sin documentacion.
struct FilaControl { const char* tecla; const char* accion; };
const FilaControl CONTROLES[] = {
    {"W A S D", "MOVER EL DRON"},
    {"SPACE",   "SUBIR"},
    {"SHIFT",   "BAJAR"},
    {"MOUSE",   "GIRAR CAMARA (ARRASTRAR)"},
    {"RUEDA",   "ACERCAR / ALEJAR"},
    {"F",       "RECENTRAR CAMARA"},
    {"R",       "REINICIAR ESCANEO"},
    {"TAB",     "CAMBIAR DE TERRENO"},
    {"F3",      "DATOS TECNICOS"},
    {"F11",     "PANTALLA COMPLETA"},
    {"ESC",     "MENU"}
};
constexpr int NUM_CONTROLES = (int)(sizeof(CONTROLES) / sizeof(CONTROLES[0]));

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

// ---------------------------------------------------------------------------
//  Arriba a la izquierda: la marca. Nada mas.
// ---------------------------------------------------------------------------
void VistaHUD::dibujarMarca(float anchoPantalla, float altoPantalla) {
    (void)anchoPantalla; (void)altoPantalla;
    dibujarRectangulo(24.0f, 26.0f, 3.0f, 14.0f, glm::vec4(Paleta::ACENTO, 1.0f));
    dibujarTexto(36.0f, 28.0f, 1.35f, Configuracion::TITULO_APP.c_str(),
                 glm::vec4(0.92f, 0.95f, 0.99f, 0.95f), ESPACIADO_TITULO);
    dibujarTexto(36.0f, 45.0f, 0.95f, Configuracion::SUBTITULO_APP.c_str(),
                 glm::vec4(Paleta::TEXTO_SEC, 0.72f), 0.9f);
}

// ---------------------------------------------------------------------------
//  Abajo a la izquierda, bajo el minimapa: porcentaje + barra fina.
// ---------------------------------------------------------------------------
void VistaHUD::dibujarProgreso(const Escena& escena, float anchoPantalla, float altoPantalla) {
    // El porcentaje sale de la MASCARA real, no de los puntos de sondeo: es
    // exactamente la misma cifra que se ve revelada en el minimapa.
    const float fraccion = escena.obtenerMapaExploracion().porcentajeExplorado();
    glm::vec4 carril = DisenoHUD::rectBarraProgreso(anchoPantalla, altoPantalla);

    char etiqueta[32];
    std::snprintf(etiqueta, sizeof(etiqueta), "EXPLORADO  %d %%",
                  (int)(fraccion * 100.0f + 0.5f));
    dibujarRectangulo(carril.x - 10.0f, carril.y - 20.0f, carril.z + 20.0f, 34.0f,
                      glm::vec4(0.01f, 0.02f, 0.04f, 0.40f));
    dibujarTexto(carril.x, carril.y - 15.0f, 1.15f, etiqueta,
                 glm::vec4(Paleta::BLANCO, 0.88f), ESPACIADO_ETIQUETA);

    // Carril apagado de fondo, para que se lea cuanto falta.
    dibujarRectangulo(carril.x, carril.y, carril.z, carril.w,
                      glm::vec4(Paleta::TEXTO_SEC, 0.22f));
    if (fraccion > 0.0f)
        dibujarRectangulo(carril.x, carril.y, carril.z * fraccion, carril.w,
                          glm::vec4(Paleta::ACENTO, 0.95f));
}

// ---------------------------------------------------------------------------
//  Abajo a la derecha: controles, en gris y sin recuadro.
// ---------------------------------------------------------------------------
void VistaHUD::dibujarControles(float anchoPantalla, float altoPantalla) {
    // Se reserva una fila extra para el rotulo "CONTROLES".
    glm::vec4 bloque = DisenoHUD::rectControles(anchoPantalla, altoPantalla, NUM_CONTROLES + 1);

    // Fondo semitransparente: sobre la rejilla clara el texto gris se perdia y
    // la ayuda dejaba de cumplir su funcion. Sigue siendo un velo, no un panel
    // opaco, y no tapa el terreno.
    dibujarRectangulo(bloque.x - 14.0f, bloque.y - 12.0f, bloque.z + 26.0f, bloque.w + 20.0f,
                      glm::vec4(0.015f, 0.028f, 0.050f, 0.62f));
    // Filete amarillo a la izquierda, igual que la marca: identifica el bloque.
    dibujarRectangulo(bloque.x - 14.0f, bloque.y - 12.0f, 2.0f, bloque.w + 20.0f,
                      glm::vec4(Paleta::ACENTO, 0.75f));

    float y = bloque.y;
    dibujarTexto(bloque.x, y, 1.15f, "CONTROLES",
                 glm::vec4(Paleta::ACENTO, 0.95f), 1.6f);
    y += 18.0f;

    // Columna de teclas alineada a la izquierda y columna de acciones a un
    // tabulador fijo: se lee como una tabla, no como una lista corrida.
    const float escala = 1.05f;
    const float columnaAccion = 62.0f;
    for (int i = 0; i < NUM_CONTROLES; ++i) {
        dibujarTexto(bloque.x, y, escala, CONTROLES[i].tecla,
                     glm::vec4(Paleta::BLANCO, 0.92f), 0.6f);
        dibujarTexto(bloque.x + columnaAccion, y, escala, CONTROLES[i].accion,
                     glm::vec4(Paleta::TEXTO_SEC, 0.88f), 0.6f);
        y += 15.0f;
    }
}

// ---------------------------------------------------------------------------
//  Pista de arranque: unos segundos y se apaga sola.
// ---------------------------------------------------------------------------
void VistaHUD::dibujarPistaInicial(const Escena& escena, float anchoPantalla, float altoPantalla) {
    float alpha = escena.obtenerAlphaPistaInicial();
    if (alpha < 0.004f) return;

    const char* linea = "WASD PARA INICIAR LA EXPLORACION";
    const float escala = 1.5f;
    float ancho = anchoTexto(linea, escala, ESPACIADO_TITULO);
    dibujarTexto((anchoPantalla - ancho) * 0.5f, altoPantalla * 0.52f, escala, linea,
                 glm::vec4(Paleta::BLANCO, 0.80f * alpha), ESPACIADO_TITULO);
}

void VistaHUD::dibujarAviso(const Escena& escena, float anchoPantalla, float altoPantalla) {
    const AvisoHUD& aviso = escena.obtenerAviso();
    if (!aviso.visible()) return;

    float alpha = aviso.obtenerAlpha();
    const char* texto = aviso.obtenerTexto().c_str();
    const float escala = 1.3f;
    float ancho = anchoTexto(texto, escala, ESPACIADO_ETIQUETA);

    // Centrado en el tercio inferior: no tapa el terreno ni el minimapa.
    float x = (anchoPantalla - ancho) * 0.5f;
    float y = altoPantalla - 70.0f;
    dibujarTexto(x, y, escala, texto, glm::vec4(Paleta::ACENTO, alpha), ESPACIADO_ETIQUETA);
}

// ---------------------------------------------------------------------------
//  F3: todo lo tecnico, oculto por defecto.
// ---------------------------------------------------------------------------
void VistaHUD::dibujarPanelDebug(const Escena& escena, const ContadorRendimiento& metricas,
                                 const Camara& camara, float anchoPantalla, float altoPantalla) {
    const Dron& dron = escena.obtenerDron();
    const EscalaMundo& escala = escena.obtenerEscala();
    const LimitesMundo& lim = escena.obtenerTerreno().obtenerLimites();
    const MapaExploracion& mapa = escena.obtenerMapaExploracion();

    char lineas[11][72];
    int n = 0;
    std::snprintf(lineas[n++], 72, "FPS %.0f   DRAW CALLS %d", metricas.obtenerFPS(), metricas.obtenerDrawCalls());
    std::snprintf(lineas[n++], 72, "NODOS LOD %d   LINEAS %d/%d", metricas.obtenerNodosTerreno(),
                  metricas.obtenerSegmentosDibujados(), metricas.obtenerSegmentosTotales());
    // El nombre se recorta: los DEM traen ficheros larguisimos que se saldrian
    // del panel y se solaparian con la escena.
    std::string nombre = escena.obtenerTerreno().obtenerNombreArchivo();
    if (nombre.size() > 26) nombre = nombre.substr(0, 25) + ".";
    std::snprintf(lineas[n++], 72, "MAPA %s", nombre.c_str());
    std::snprintf(lineas[n++], 72, "POS %.2f %.2f %.2f", dron.obtenerPosicion().x,
                  dron.obtenerPosicion().y, dron.obtenerPosicion().z);
    std::snprintf(lineas[n++], 72, "ALTITUD %.2f   VELOCIDAD %.3f",
                  dron.obtenerAlturaSobreTerreno(), dron.obtenerRapidez());
    std::snprintf(lineas[n++], 72, "YAW %.1f  PITCH %.1f  ROLL %.1f",
                  dron.obtenerYaw(), dron.obtenerPitch(), dron.obtenerRoll());
    std::snprintf(lineas[n++], 72, "DIAGONAL %.1f   ESCALA DRON %.5f",
                  escala.diagonalTerreno, escala.escalaDron);
    std::snprintf(lineas[n++], 72, "VMAX %.2f   RADAR %.2f   CAM %.2f",
                  escala.velocidadMaxima, escala.radioEscaneo, camara.obtenerRadio());
    std::snprintf(lineas[n++], 72, "NEAR %.3f   FAR %.1f   FOV %.0f",
                  escala.planoCercano, escala.planoLejano, Configuracion::CAM_FOV);
    std::snprintf(lineas[n++], 72, "CELDAS %zu / %zu   COBERTURA %.2f %%",
                  mapa.celdasExploradas(), mapa.celdasValidas(),
                  mapa.porcentajeExplorado() * 100.0f);
    std::snprintf(lineas[n++], 72, "TERRENO Y %.2f .. %.2f   CAM %s",
                  lim.minY, lim.maxY, nombreModoCamara(camara.obtenerModo()));

    glm::vec4 panel = DisenoHUD::rectPanelDebug(anchoPantalla, altoPantalla, n);
    dibujarRectangulo(panel.x, panel.y, panel.z, panel.w, glm::vec4(0.02f, 0.03f, 0.05f, 0.72f));
    dibujarRectangulo(panel.x, panel.y, 2.0f, panel.w, glm::vec4(Paleta::CIAN, 0.9f));

    float y = panel.y + 10.0f;
    for (int i = 0; i < n; ++i) {
        dibujarTexto(panel.x + 10.0f, y, 1.05f, lineas[i],
                     glm::vec4(Paleta::TEXTO_SEC, 0.95f), 0.5f);
        y += 15.0f;
    }
}

// ---------------------------------------------------------------------------
//  Menu de configuracion (ESC): continuar, terreno, reiniciar escaneo, ayuda,
//  salir. Solo aparece con el simulador detenido, asi que aqui si cabe un panel.
// ---------------------------------------------------------------------------
void VistaHUD::dibujarMenu(const Escena& escena, float anchoPantalla, float altoPantalla,
                           int opcionMenu, int mapaSeleccionado) {
    if (escena.obtenerEstadoAplicacion() != EstadoAplicacion::Paused) return;

    // Velo sobre toda la pantalla: deja entrever el terreno detras.
    dibujarRectangulo(0, 0, anchoPantalla, altoPantalla, glm::vec4(0.005f, 0.012f, 0.025f, 0.78f));

    const float panelW = 460.0f;
    const float altoFila = 34.0f;
    const float panelH = 132.0f + OPCIONES_MENU * altoFila + 44.0f;
    const float px = (anchoPantalla - panelW) * 0.5f;
    const float py = (altoPantalla - panelH) * 0.5f;

    dibujarRectangulo(px, py, panelW, panelH, glm::vec4(0.018f, 0.036f, 0.062f, 0.96f));
    dibujarRectangulo(px, py, 4.0f, panelH, glm::vec4(Paleta::ACENTO, 1.0f));

    dibujarTexto(px + 32.0f, py + 30.0f, 2.4f, Configuracion::TITULO_APP.c_str(),
                 glm::vec4(Paleta::BLANCO, 1.0f), ESPACIADO_TITULO);
    dibujarTexto(px + 32.0f, py + 60.0f, 1.05f, "CONFIGURACION",
                 glm::vec4(Paleta::TEXTO_SEC, 0.9f), 1.4f);

    const auto& mapas = escena.obtenerMapas();
    float y = py + 110.0f;
    for (int i = 0; i < OPCIONES_MENU; ++i) {
        const bool sel = (i == opcionMenu);
        if (sel)
            dibujarRectangulo(px + 22.0f, y - 9.0f, panelW - 44.0f, 27.0f,
                              glm::vec4(Paleta::ACENTO, 0.16f));

        glm::vec4 color = sel ? glm::vec4(Paleta::ACENTO, 1.0f)
                              : glm::vec4(Paleta::BLANCO, 0.86f);
        dibujarTexto(px + 34.0f, y, 1.5f, etiquetaFilaMenu(i), color, 1.2f);

        // Filas con valor: se muestra el valor a la derecha entre flechas, para
        // que se vea que izquierda/derecha lo cambian.
        if (filaTieneValor(i)) {
            char valor[64];
            if (i == FILA_MAPA) {
                std::string nombre = (mapaSeleccionado >= 0 && mapaSeleccionado < (int)mapas.size())
                    ? std::filesystem::path(mapas[mapaSeleccionado]).stem().string()
                    : "-";
                if (nombre.size() > 16) nombre = nombre.substr(0, 15) + ".";
                std::snprintf(valor, sizeof(valor), "< %s %d/%d >", nombre.c_str(),
                              mapaSeleccionado + 1, (int)mapas.size());
            } else {
                std::snprintf(valor, sizeof(valor), "< %s >",
                              escena.obtenerAjustes().controles ? "SI" : "NO");
            }
            float ancho = anchoTexto(valor, 1.25f, 0.8f);
            dibujarTexto(px + panelW - 34.0f - ancho, y + 2.0f, 1.25f, valor,
                         sel ? glm::vec4(Paleta::ACENTO, 1.0f)
                             : glm::vec4(Paleta::TEXTO_SEC, 0.95f), 0.8f);
        }
        y += altoFila;
    }

    const char* pie = "W/S MOVER   FLECHAS CAMBIAR VALOR   ENTER CONFIRMAR   ESC VOLVER";
    dibujarTexto(px + 32.0f, py + panelH - 26.0f, 1.0f, pie,
                 glm::vec4(Paleta::TEXTO_SEC, 0.85f), 0.4f);
}

// ---------------------------------------------------------------------------
int VistaHUD::dibujar(Escena& escena, const EstadoTeclas& teclas,
                      float anchoPantalla, float altoPantalla,
                      const ContadorRendimiento& metricas, const Camara& camara,
                      int opcionMenu, int mapaSeleccionado) {
    (void)teclas;
    drawCalls = 0;
    glDisable(GL_DEPTH_TEST);
    glUseProgram(programa);

    // Ortografica con Y hacia abajo: coordenadas de pantalla directas.
    glm::mat4 orto = glm::ortho(0.0f, anchoPantalla, altoPantalla, 0.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(locProyeccion, 1, GL_FALSE, glm::value_ptr(orto));
    glBindVertexArray(vao);

    dibujarMarca(anchoPantalla, altoPantalla);
    if (escena.obtenerAjustes().minimapa)
        dibujarProgreso(escena, anchoPantalla, altoPantalla);
    if (escena.obtenerAjustes().controles)
        dibujarControles(anchoPantalla, altoPantalla);
    dibujarPistaInicial(escena, anchoPantalla, altoPantalla);
    dibujarAviso(escena, anchoPantalla, altoPantalla);

    // Toda la informacion tecnica queda oculta hasta que el usuario pulsa F3.
    if (escena.obtenerAjustes().modoDebug)
        dibujarPanelDebug(escena, metricas, camara, anchoPantalla, altoPantalla);

    dibujarMenu(escena, anchoPantalla, altoPantalla, opcionMenu, mapaSeleccionado);

    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
    return drawCalls;
}

void VistaHUD::liberar() {
    if (vao) { glDeleteVertexArrays(1, &vao); glDeleteBuffers(1, &vbo); }
    vao = vbo = 0;
    programa = 0;   // el programa lo posee el GestorRecursos
}
