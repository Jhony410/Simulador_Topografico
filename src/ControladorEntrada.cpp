// glad SIEMPRE antes que GLFW: glfw3.h incluye GL/gl.h y glad se niega a
// compilar si ese encabezado ya entro.
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "ControladorEntrada.h"
#include "Camara.h"
#include "Configuracion.h"
#include "DisenoHUD.h"
#include "Escena.h"
#include "Persistencia.h"

void ControladorEntrada::inicializar(GLFWwindow* v, Escena* e, Camara* c) {
    ventana = v;
    escena  = e;
    camara  = c;

    glfwGetFramebufferSize(v, &anchoPantalla, &altoPantalla);

    // El puntero de usuario es lo que permite volver de un callback estatico de
    // C a esta instancia sin recurrir a variables globales.
    glfwSetWindowUserPointer(v, this);
    glfwSetFramebufferSizeCallback(v, alRedimensionar);
    glfwSetMouseButtonCallback(v, alBotonMouse);
    glfwSetCursorPosCallback(v, alMoverMouse);
    glfwSetScrollCallback(v, alRuedaMouse);
    glfwSetKeyCallback(v, alTecla);
}

// ---- Puentes estaticos -----------------------------------------------------
void ControladorEntrada::alRedimensionar(GLFWwindow* v, int ancho, int alto) {
    auto* self = static_cast<ControladorEntrada*>(glfwGetWindowUserPointer(v));
    if (!self) return;
    self->anchoPantalla = ancho;
    self->altoPantalla  = alto;
    glViewport(0, 0, ancho, alto);
}
void ControladorEntrada::alBotonMouse(GLFWwindow* v, int boton, int accion, int) {
    if (auto* self = static_cast<ControladorEntrada*>(glfwGetWindowUserPointer(v)))
        self->manejarBotonMouse(boton, accion);
}
void ControladorEntrada::alMoverMouse(GLFWwindow* v, double x, double y) {
    if (auto* self = static_cast<ControladorEntrada*>(glfwGetWindowUserPointer(v)))
        self->manejarMovimientoMouse(x, y);
}
void ControladorEntrada::alRuedaMouse(GLFWwindow* v, double, double dy) {
    auto* self = static_cast<ControladorEntrada*>(glfwGetWindowUserPointer(v));
    if (self && self->camara) self->camara->acercar((float)dy * 5.0f);
}
void ControladorEntrada::alTecla(GLFWwindow* v, int tecla, int, int accion, int) {
    if (auto* self = static_cast<ControladorEntrada*>(glfwGetWindowUserPointer(v)))
        self->manejarTecla(tecla, accion);
}

// ---- Logica ----------------------------------------------------------------
void ControladorEntrada::manejarBotonMouse(int boton, int accion) {
    if (boton != GLFW_MOUSE_BUTTON_LEFT) return;

    if (accion == GLFW_PRESS) {
        double mx, my;
        glfwGetCursorPos(ventana, &mx, &my);

        // El circulo amarillo de arriba a la derecha cierra la aplicacion.
        if (DisenoHUD::puntoDentro(mx, my,
                DisenoHUD::rectBotonSalir((float)anchoPantalla, (float)altoPantalla))) {
            glfwSetWindowShouldClose(ventana, true);
            return;
        }

        // Despues el resto del HUD: si el clic cae en un boton de mapa no debe
        // llegar a rotar la camara.
        int total = escena ? (int)escena->obtenerMapas().size() : 0;
        for (int i = 0; i < total && i < 9; i++) {
            glm::vec4 r = DisenoHUD::rectBotonMapa(i, (float)anchoPantalla, (float)altoPantalla);
            if (DisenoHUD::puntoDentro(mx, my, r)) {
                if (i != escena->obtenerMapaActual()) { mapaSolicitado = i; cambioMapaPendiente = true; }
                return;
            }
        }
        arrastrando = true;
        primerMovimiento = true;
    } else if (accion == GLFW_RELEASE) {
        arrastrando = false;
    }
}

void ControladorEntrada::manejarMovimientoMouse(double x, double y) {
    if (!arrastrando || !camara) return;
    // El primer frame de arrastre solo fija el origen, o la vista pegaria un salto.
    if (primerMovimiento) { ultimoX = x; ultimoY = y; primerMovimiento = false; }

    camara->orbitar((float)(x - ultimoX) * 0.3f, (float)(ultimoY - y) * 0.3f);
    ultimoX = x;
    ultimoY = y;
}

void ControladorEntrada::manejarTecla(int tecla, int accion) {
    if (accion != GLFW_PRESS) return;
    if (tecla == GLFW_KEY_ESCAPE) { glfwSetWindowShouldClose(ventana, true); return; }

    // F5 guarda, F9 recupera. El resultado se anuncia en el HUD tanto si sale
    // bien como si falla: un guardado silencioso que no funciona es peor que
    // ninguno.
    if (tecla == GLFW_KEY_F5 || tecla == GLFW_KEY_F9) {
        if (!escena) return;
        std::string mensaje;
        bool ok = (tecla == GLFW_KEY_F5)
                    ? Persistencia::guardar(*escena, Configuracion::RUTA_GUARDADO, mensaje)
                    : Persistencia::cargar(*escena, Configuracion::RUTA_GUARDADO, mensaje);
        escena->mostrarAviso(ok ? mensaje : ("ERROR: " + mensaje));
        return;
    }

    int total = escena ? (int)escena->obtenerMapas().size() : 0;
    if (total == 0) return;
    int actual = escena->obtenerMapaActual();

    if (tecla == GLFW_KEY_TAB) {
        mapaSolicitado = (actual + 1) % total;
        cambioMapaPendiente = true;
    } else if (tecla == GLFW_KEY_BACKSPACE) {
        mapaSolicitado = (actual - 1 + total) % total;
        cambioMapaPendiente = true;
    } else if (tecla >= GLFW_KEY_1 && tecla <= GLFW_KEY_9) {
        int i = tecla - GLFW_KEY_1;
        if (i < total) { mapaSolicitado = i; cambioMapaPendiente = true; }
    }
}

void ControladorEntrada::procesarEntradaContinua(float dt) {
    if (!ventana || !escena || !camara) return;

    auto presionada = [&](int t) { return glfwGetKey(ventana, t) == GLFW_PRESS; };

    teclas.arriba    = presionada(GLFW_KEY_UP)    || presionada(GLFW_KEY_W);
    teclas.abajo     = presionada(GLFW_KEY_DOWN)  || presionada(GLFW_KEY_S);
    teclas.derecha   = presionada(GLFW_KEY_RIGHT) || presionada(GLFW_KEY_D);
    teclas.izquierda = presionada(GLFW_KEY_LEFT)  || presionada(GLFW_KEY_A);
    teclas.espacio   = presionada(GLFW_KEY_SPACE);
    teclas.shift     = presionada(GLFW_KEY_LEFT_SHIFT) || presionada(GLFW_KEY_RIGHT_SHIFT);

    // Los ejes se toman de la camara: "adelante" es siempre hacia el fondo de
    // la pantalla, aunque el jugador haya rotado la vista.
    glm::vec3 direccion(0.0f);
    if (teclas.arriba)    direccion += camara->adelante();
    if (teclas.abajo)     direccion -= camara->adelante();
    if (teclas.derecha)   direccion += camara->derecha();
    if (teclas.izquierda) direccion -= camara->derecha();

    Dron& dron = escena->obtenerDron();
    dron.mover(direccion, Configuracion::VEL_DRON, dt);
    if (teclas.espacio) dron.ajustarAltura( Configuracion::VEL_ALTURA * dt);
    if (teclas.shift)   dron.ajustarAltura(-Configuracion::VEL_ALTURA * dt);
}

int ControladorEntrada::consumirCambioDeMapa() {
    cambioMapaPendiente = false;
    return mapaSolicitado;
}
