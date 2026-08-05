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

    GLFWwindow*        ventana = nullptr;
    Escena             escena;        // MODELO
    Camara             camara;        // VISTA (parametros de encuadre)
    Renderizador       renderizador;  // VISTA
    ControladorEntrada controlador;   // CONTROLADOR
};
