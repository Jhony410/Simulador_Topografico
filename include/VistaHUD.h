#pragma once
#include "EstadoEntrada.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

class Escena;
class GestorRecursos;
class ContadorRendimiento;
class Camara;

// ============================================================================
//  VISTA: capa 2D en proyeccion ortografica.
//
//  Interfaz deliberadamente MINIMA durante el vuelo:
//    - Arriba a la izquierda: GEODRONE + subtitulo pequeno.
//    - Abajo a la izquierda:  minimapa (lo dibuja VistaMinimapa), porcentaje de
//                             exploracion y barra fina de progreso.
//    - Abajo a la derecha:    lista corta de controles.
//    - Centro:                solo la pista de arranque, unos segundos.
//
//  FPS, coordenadas, altitud, velocidad, draw calls y metricas de LOD NO se
//  dibujan por defecto: viven en el panel de depuracion que alterna F3.
//
//  Dibuja con un unico programa que solo recibe posiciones en pixeles y un
//  color plano; el texto sale de stb_easy_font convertido a triangulos.
// ============================================================================
class VistaHUD {
public:
    void inicializar(GestorRecursos& recursos);
    void liberar();

    // Devuelve el numero de draw calls emitidas.
    int dibujar(Escena& escena, const EstadoTeclas& teclas,
                float anchoPantalla, float altoPantalla,
                const ContadorRendimiento& metricas, const Camara& camara,
                int opcionMenu, int mapaSeleccionado);

private:
    void dibujarRectangulo(float x, float y, float ancho, float alto, const glm::vec4& color);

    // 'espaciado' abre el interletraje (en unidades de fuente, antes de escalar):
    // stb_easy_font no lo soporta, asi que se dibuja glifo a glifo acumulando
    // el avance a mano, pero todo acaba en UN solo draw call.
    void dibujarTexto(float x, float y, float escala, const char* texto,
                      const glm::vec4& color, float espaciado = 0.0f);
    float anchoTexto(const char* texto, float escala, float espaciado = 0.0f) const;

    // ---- Bloques de la interfaz minima ----
    void dibujarMarca(float anchoPantalla, float altoPantalla);
    void dibujarProgreso(const Escena& escena, float anchoPantalla, float altoPantalla);
    void dibujarControles(float anchoPantalla, float altoPantalla);
    void dibujarPistaInicial(const Escena& escena, float anchoPantalla, float altoPantalla);
    void dibujarAviso(const Escena& escena, float anchoPantalla, float altoPantalla);

    // ---- Solo con F3 ----
    void dibujarPanelDebug(const Escena& escena, const ContadorRendimiento& metricas,
                           const Camara& camara, float anchoPantalla, float altoPantalla);

    // ---- Menu de configuracion (ESC) ----
    void dibujarMenu(const Escena& escena, float anchoPantalla, float altoPantalla,
                     int opcionMenu, int mapaSeleccionado);

    GLuint programa = 0;
    GLint  locProyeccion = -1, locModelo = -1, locColor = -1;
    GLuint vao = 0, vbo = 0;

    // Buffer de trabajo de stb_easy_font. Se reserva una vez para no pedir
    // memoria en cada cadena de texto de cada frame.
    std::vector<char> bufferTexto;

    // Cada rectangulo y cada cadena es una llamada de dibujo: se cuentan aqui
    // para que la cifra del panel de debug refleje el coste real.
    int drawCalls = 0;
};
