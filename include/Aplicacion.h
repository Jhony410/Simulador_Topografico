#pragma once
#include "Camara.h"
#include "ControladorEntrada.h"
#include "Escena.h"
#include "Renderizador.h"

struct GLFWwindow;

// ============================================================================
//  Punto de union del patron MVC: crea la ventana, instancia el Modelo
//  (Escena), la Vista (Renderizador) y el Controlador, y corre el bucle
//  principal en el orden entrada -> modelo -> vista.
// ============================================================================
class Aplicacion {
public:
    bool inicializar();
    void ejecutar();
    void liberar();

private:
    void aplicarCambioDeMapa(int indice);
    void actualizarTitulo();
    // Crea la ventana al 95x92% del monitor principal y la centra.
    bool crearVentana();
    // F11: alterna entre ventana y pantalla completa restaurando el tamano y la
    // posicion previos. Recalcula viewport; la proyeccion se rehace sola porque
    // el aspecto se lee del framebuffer cada frame.
    void alternarPantallaCompleta();

    GLFWwindow*        ventana = nullptr;
    // Estado que hay que guardar para poder volver de pantalla completa.
    bool enPantallaCompleta = false;
    int  ventanaX = 0, ventanaY = 0;
    int  ventanaAncho = 0, ventanaAlto = 0;

    Escena             escena;        // MODELO
    Camara             camara;        // VISTA (parametros de encuadre)
    Renderizador       renderizador;  // VISTA
    ControladorEntrada controlador;   // CONTROLADOR
};
