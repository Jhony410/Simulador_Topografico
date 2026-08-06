#pragma once
#include "Componente.h"
#include "MallaCruda.h"
#include "NodoEscena.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// ============================================================================
//  Componentes concretos del sistema COP.
//  Ninguno hace llamadas a OpenGL: son datos que la Vista consulta para saber
//  QUE dibujar y COMO, sin que el Modelo sepa que existe una GPU.
// ============================================================================

// ---- Posicion / orientacion / escala, y enlace al nodo del grafo ----------
class ComponenteTransformada : public Componente {
public:
    glm::vec3 posicion{0.0f};
    glm::vec3 rotacionEuler{0.0f};   // grados: pitch (x), yaw (y), roll (z)
    glm::vec3 escala{1.0f};

    // Nodo del scene graph al que esta entidad alimenta su matriz local.
    // Es observador: el arbol es el dueño de la memoria.
    NodoEscena* nodo = nullptr;

    glm::mat4 componerMatriz() const {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), posicion);
        m = glm::rotate(m, glm::radians(rotacionEuler.y), glm::vec3(0, 1, 0));
        m = glm::rotate(m, glm::radians(rotacionEuler.x), glm::vec3(1, 0, 0));
        m = glm::rotate(m, glm::radians(rotacionEuler.z), glm::vec3(0, 0, 1));
        m = glm::scale(m, escala);
        return m;
    }

    // Vuelca la transformada al grafo; luego la raiz propaga en cascada.
    void volcarAlNodo() const {
        if (nodo) nodo->establecerTransformacionLocal(componerMatriz());
    }
};

// ---- Referencia (no propietaria) a la geometria en CPU --------------------
class ComponenteMalla : public Componente {
public:
    const MallaCruda* malla = nullptr;  // vive en Terreno o en la Escena
    bool topologiaLineas = false;       // true = GL_LINES (mapas de calles CSV)
    bool dirty = true;                  // la Vista debe volver a subirla a GPU
};

// ---- Como se colorea la entidad ------------------------------------------
class ComponenteMaterial : public Componente {
public:
    glm::vec3 color{0.85f, 0.88f, 0.92f};   // color de las aristas
    float     alpha = 0.7f;

    bool      dibujarRelleno = false;       // pasada previa solida (silueta)
    glm::vec3 colorRelleno{0.0f};
    float     alphaRelleno = 1.0f;

    // 0 = no aporta al buffer de brillo; 1 = emite y por tanto genera glow.
    float     emision = 0.0f;
};

// ---- Parametros de animacion (giro de helices) ---------------------------
class ComponenteAnimacion : public Componente {
public:
    float velocidadGiro = 0.0f;  // rad/seg
    float tiempo = 0.0f;         // acumulador propio de la entidad

    void avanzar(float dt) { tiempo += dt; }
};

// ---- Sensor de escaneo continuo del dron ----------------------------------
class ComponenteEscaner : public Componente {
public:
    float radioEscaneo   = 14.0f;  // radio en unidades de mundo
    int   celdasPorFrame = 256;    // lote que se desencola cada frame
    bool  activo         = true;
};
