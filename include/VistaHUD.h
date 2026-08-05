#pragma once
#include "EstadoEntrada.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

class Escena;
class EstadoMision;
class GestorRecursos;
class ContadorRendimiento;

// ============================================================================
//  VISTA: capa 2D en proyeccion ortografica (botones de mapa, teclas, textos).
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
                const ContadorRendimiento& metricas);

private:
    void dibujarRectangulo(float x, float y, float ancho, float alto, const glm::vec4& color);

    // 'espaciado' abre el interletraje (en unidades de fuente, antes de escalar):
    // stb_easy_font no lo soporta, asi que se dibuja glifo a glifo acumulando
    // el avance a mano, pero todo acaba en UN solo draw call.
    void dibujarTexto(float x, float y, float escala, const char* texto,
                      const glm::vec4& color, float espaciado = 0.0f);
    float anchoTexto(const char* texto, float escala, float espaciado = 0.0f) const;

    // Primitivas derivadas: el shader del HUD solo sabe pintar triangulos
    // planos, asi que lineas gruesas y anillos se teselan aqui a mano.
    void dibujarLineaGruesa(float x1, float y1, float x2, float y2,
                            float grosor, const glm::vec4& color);
    void dibujarAnillo(float cx, float cy, float radio, float grosor,
                       const glm::vec4& color, int segmentos = 40);

    void dibujarBoton(const glm::vec4& rect, const char* etiqueta, bool activo, bool resaltado);
    void dibujarTecla(const glm::vec4& rect, const char* etiqueta, bool presionada);

    // Bloques de la FASE 3
    void dibujarBarraProgreso(const EstadoMision& mision, float anchoPantalla, float altoPantalla);
    void dibujarTextoMision(const EstadoMision& mision, float anchoPantalla, float altoPantalla);
    void dibujarPanelLateral(const EstadoMision& mision, float anchoPantalla, float altoPantalla);
    void dibujarMetricas(const ContadorRendimiento& metricas, float anchoPantalla, float altoPantalla);
    void dibujarAviso(const Escena& escena, float anchoPantalla, float altoPantalla);

    // Bloques de la FASE 7
    void dibujarEsquinas(float anchoPantalla, float altoPantalla);
    void dibujarClusterTeclas(const EstadoTeclas& teclas, float anchoPantalla, float altoPantalla);

    GLuint programa = 0;
    GLint  locProyeccion = -1, locModelo = -1, locColor = -1;
    GLuint vao = 0, vbo = 0;

    // Buffer de trabajo de stb_easy_font. Se reserva una vez para no pedir
    // memoria en cada cadena de texto de cada frame.
    std::vector<char> bufferTexto;

    // Cada rectangulo y cada cadena es una llamada de dibujo: se cuentan aqui
    // para que la cifra del HUD refleje el coste real y no solo el de la escena.
    int drawCalls = 0;
};
