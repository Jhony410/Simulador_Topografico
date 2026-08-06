#pragma once
#include "EstadoEntrada.h"

// Se declara GLFWwindow en lugar de incluir glfw3.h: ese encabezado arrastra
// GL/gl.h y, si entra antes que glad, glad aborta la compilacion. El .cpp
// incluye glad primero y GLFW despues.
struct GLFWwindow;

class Escena;
class Camara;

// ============================================================================
//  CONTROLADOR: traduce teclado y mouse a cambios en el MODELO.
//  No crea ni toca un solo buffer de GPU.
//
//  El vuelo se resuelve en procesarEntradaContinua(): cada frame se lee el
//  estado de las teclas y se entrega al Dron una DIRECCION deseada (y un eje
//  vertical), nunca un desplazamiento ya integrado. Si no hay tecla pulsada, la
//  direccion es el vector nulo y el Dron frena hasta detenerse por completo.
// ============================================================================
class ControladorEntrada {
public:
    void inicializar(GLFWwindow* ventana, Escena* escena, Camara* camara);

    // Entrada mantenida (vuelo del dron). Se llama una vez por frame.
    void procesarEntradaContinua(float dt);

    const EstadoTeclas& obtenerEstadoTeclas() const { return teclas; }

    // Peticion pendiente de cambio de mapa (la resuelve la Aplicacion, porque
    // implica volver a subir geometria a la GPU).
    bool hayCambioDeMapaPendiente() const { return cambioMapaPendiente; }
    int  consumirCambioDeMapa();

    // Peticion pendiente de alternar pantalla completa (la resuelve la
    // Aplicacion: es ella quien posee la ventana y el monitor).
    bool consumirAlternarPantallaCompleta() {
        bool s = alternarPantallaCompleta; alternarPantallaCompleta = false; return s;
    }

    // ---- Menu de configuracion (ESC) ----
    // La Vista necesita saber que fila esta resaltada y que mapa hay elegido
    // en la fila de seleccion de terreno.
    int obtenerOpcionMenu()     const { return opcionMenu; }
    int obtenerMapaSeleccionado() const { return mapaSeleccionado; }

    // Tamano de ventana vigente, actualizado por el callback de framebuffer.
    int obtenerAncho() const { return anchoPantalla; }
    int obtenerAlto()  const { return altoPantalla; }

private:
    // Callbacks de GLFW: recuperan la instancia via glfwGetWindowUserPointer.
    static void alRedimensionar(GLFWwindow* v, int ancho, int alto);
    static void alBotonMouse(GLFWwindow* v, int boton, int accion, int mods);
    static void alMoverMouse(GLFWwindow* v, double x, double y);
    static void alRuedaMouse(GLFWwindow* v, double dx, double dy);
    static void alTecla(GLFWwindow* v, int tecla, int scancode, int accion, int mods);

    void manejarBotonMouse(int boton, int accion);
    void manejarMovimientoMouse(double x, double y);
    void manejarTecla(int tecla, int accion);
    void seleccionarMedicion(double x, double y);

    // Navegacion del menu de configuracion. Devuelve true si consumio la tecla.
    bool manejarTeclaMenu(int tecla);
    void abrirMenu();
    void ejecutarOpcionMenu();

    GLFWwindow* ventana = nullptr;
    Escena*     escena  = nullptr;
    Camara*     camara  = nullptr;

    EstadoTeclas teclas;

    bool   arrastrando = false;
    bool   primerMovimiento = true;
    double ultimoX = 0.0, ultimoY = 0.0;

    bool cambioMapaPendiente = false;
    int  mapaSolicitado = 0;
    bool alternarPantallaCompleta = false;

    int  opcionMenu = 0;
    int  mapaSeleccionado = 0;   // fila "TERRENO": se aplica al confirmar

    int anchoPantalla = 1280, altoPantalla = 720;
};
