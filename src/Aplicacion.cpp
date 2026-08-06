#include "Aplicacion.h"
#include "Configuracion.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <algorithm>
#include <string>

bool Aplicacion::inicializar() {
    // 1) MODELO primero: si no hay terrenos no tiene sentido abrir ventana.
    if (!escena.inicializar()) return false;

    // 2) Ventana y contexto de OpenGL 3.3 Core.
    std::cout << "[INIT] Inicializando GLFW\n";
    if (!glfwInit()) { std::cerr << "[ERROR] GLFW no pudo inicializarse. Verifica el controlador grafico.\n"; return false; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    ventana = glfwCreateWindow(Configuracion::ANCHO_VENTANA, Configuracion::ALTO_VENTANA,
                               "GeoDrone - Exploracion Topografica", nullptr, nullptr);
    if (!ventana) { std::cerr << "[ERROR] No se pudo crear la ventana OpenGL 3.3.\n"; glfwTerminate(); return false; }
    glfwMakeContextCurrent(ventana);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "[ERROR] GLAD no pudo resolver las funciones OpenGL.\n";
        glfwDestroyWindow(ventana);
        ventana = nullptr;
        glfwTerminate();
        return false;
    }

    // 3) CONTROLADOR: registra los callbacks sobre Modelo y Camara.
    controlador.inicializar(ventana, &escena, &camara);

    // 4) VISTA: compila shaders y sube la geometria inicial.
    if (!renderizador.inicializar()) {
        renderizador.liberar();
        glfwDestroyWindow(ventana);
        ventana = nullptr;
        glfwTerminate();
        return false;
    }
    renderizador.sincronizarConEscena(escena);

    camara.establecerRadio(Configuracion::CAM_RADIO_INICIAL);
    camara.seguir(escena.obtenerDron().obtenerPosicion());
    camara.saltarAObjetivo();   // el primer frame no debe venir interpolado
    actualizarTitulo();

    std::cout << "[INIT] OpenGL " << glGetString(GL_VERSION) << " | GPU "
              << glGetString(GL_RENDERER) << "\n";
    std::cout << "[INIT] Enter iniciar | WASD mover | C camara | V visualizacion | "
                 "X medicion | ESC pausa\n";
    return true;
}

void Aplicacion::actualizarTitulo() {
    std::string titulo = "GeoDrone  |  " + escena.obtenerTerreno().obtenerNombreArchivo() +
                         "  [" + std::to_string(escena.obtenerMapaActual() + 1) + "/" +
                         std::to_string(escena.obtenerMapas().size()) + "]";
    glfwSetWindowTitle(ventana, titulo.c_str());
}

void Aplicacion::aplicarCambioDeMapa(int indice) {
    if (!escena.cargarMapa(indice)) return;
    // No hace falta sincronizar aqui: el bucle lo hace cada frame y solo actua
    // sobre lo que el Modelo haya marcado como sucio.
    camara.establecerRadio(Configuracion::CAM_RADIO_INICIAL);
    // Cambiar de mapa teletransporta el dron: la camara debe ir con el de golpe,
    // no atravesar el terreno interpolando desde la posicion anterior.
    camara.seguir(escena.obtenerDron().obtenerPosicion());
    camara.saltarAObjetivo();
    actualizarTitulo();
}

void Aplicacion::ejecutar() {
    float tiempoAnterior = (float)glfwGetTime();

    while (!glfwWindowShouldClose(ventana)) {
        float ahora = (float)glfwGetTime();
        float dt = std::clamp(ahora - tiempoAnterior, 0.0f, 0.05f);
        tiempoAnterior = ahora;

        // ---- CONTROLADOR ----
        if (controlador.hayCambioDeMapaPendiente())
            aplicarCambioDeMapa(controlador.consumirCambioDeMapa());
        controlador.procesarEntradaContinua(dt);

        // ---- MODELO ----
        escena.actualizar(dt);

        // ---- VISTA ----
        // Sube a GPU solo lo que el Modelo marco como sucio este frame.
        renderizador.sincronizarConEscena(escena);
        camara.seguir(escena.obtenerDron().obtenerPosicion());
        camara.establecerDatosDron(escena.obtenerDron().obtenerYaw(),
                                   escena.obtenerDron().obtenerVelocidad());
        camara.actualizar(dt);
        camara.evitarTerreno(escena.obtenerTerreno());
        renderizador.renderizar(escena, camara,
                                controlador.obtenerAncho(), controlador.obtenerAlto(),
                                controlador.obtenerEstadoTeclas(), dt,
                                controlador.obtenerOpcionMenu(),
                                controlador.obtenerOpcionConfiguracion(),
                                controlador.estaEnConfiguracion());

        glfwSwapBuffers(ventana);
        glfwPollEvents();
    }
}

void Aplicacion::liberar() {
    renderizador.liberar();
    if (ventana) glfwDestroyWindow(ventana);
    glfwTerminate();
}
