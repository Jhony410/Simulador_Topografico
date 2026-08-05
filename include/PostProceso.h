#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>

class GestorRecursos;

// ============================================================================
//  VISTA: post-proceso de glow (bloom) sobre la escena 3D.
//
//  CADENA DE PASADAS
//    1) La escena se dibuja en un FBO con DOS attachments de color (MRT):
//         attachment 0 -> imagen normal
//         attachment 1 -> solo lo que emite (las aristas del dron)
//       Usar MRT evita tener que redibujar la escena entera una segunda vez
//       solo para saber que brilla.
//    2) 'brillo.frag' baja el attachment 1 a media resolucion y le aplica un
//       corte suave por luminancia.
//    3) Desenfoque gaussiano separable en PING-PONG entre dos FBOs: se alterna
//       leer de uno y escribir en el otro, porque OpenGL no permite muestrear
//       de la misma textura en la que se esta escribiendo.
//    4) 'composicion.frag' suma escena + halo en un quad de pantalla completa,
//       ya sobre el framebuffer por defecto.
//
//  El HUD se dibuja DESPUES de componer: no debe brillar ni desenfocarse.
// ============================================================================
class PostProceso {
public:
    bool inicializar(GestorRecursos& recursos);
    void liberar();

    // Redirige el dibujado a la textura y limpia ambos attachments.
    void iniciarCaptura(int ancho, int alto, const glm::vec3& colorFondo);

    // Desenfoca el brillo y compone el resultado en el framebuffer por defecto.
    // Devuelve el numero de draw calls emitidas.
    int componer(int ancho, int alto);

    bool estaListo() const { return listo; }

private:
    void crearObjetivos(int ancho, int alto);
    void destruirObjetivos();
    void dibujarQuad();

    GLuint fboEscena = 0, texColor = 0, texBrillo = 0, rboProfundidad = 0;
    GLuint fboPing[2] = {0, 0}, texPing[2] = {0, 0};

    GLuint progBrillo = 0, progDesenfoque = 0, progComposicion = 0;
    GLuint vaoQuad = 0, vboQuad = 0;

    int ancho = 0, alto = 0;
    int anchoBrillo = 0, altoBrillo = 0;
    bool listo = false;
};
