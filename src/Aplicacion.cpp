#include "Aplicacion.h"
#include "Configuracion.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <algorithm>
#include <string>

bool Aplicacion::crearVentana() {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    // Tamano derivado del monitor real, no una constante 1280x720: en un panel
    // 1080p la ventana abre a 1824x993 y en uno 1440p a 2432x1325.
    int ancho = Configuracion::ANCHO_VENTANA_MIN;
    int alto  = Configuracion::ALTO_VENTANA_MIN;
    int origenX = 0, origenY = 0, anchoMonitor = ancho, altoMonitor = alto;

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* modo = monitor ? glfwGetVideoMode(monitor) : nullptr;
    if (modo) {
        // La zona de trabajo descuenta la barra de tareas: centrar sobre ella
        // evita que el borde inferior quede debajo del sistema.
        glfwGetMonitorWorkarea(monitor, &origenX, &origenY, &anchoMonitor, &altoMonitor);
        if (anchoMonitor <= 0 || altoMonitor <= 0) {
            anchoMonitor = modo->width;
            altoMonitor  = modo->height;
        }
        ancho = std::max(Configuracion::ANCHO_VENTANA_MIN,
                         (int)(anchoMonitor * Configuracion::FRACCION_VENTANA_X));
        alto  = std::max(Configuracion::ALTO_VENTANA_MIN,
                         (int)(altoMonitor  * Configuracion::FRACCION_VENTANA_Y));
    }

    ventana = glfwCreateWindow(ancho, alto, "GeoDrone - Exploracion Topografica", nullptr, nullptr);
    if (!ventana) return false;

    if (modo) {
        ventanaX = origenX + (anchoMonitor - ancho) / 2;
        ventanaY = origenY + (altoMonitor  - alto)  / 2;
        glfwSetWindowPos(ventana, ventanaX, ventanaY);
    }
    ventanaAncho = ancho;
    ventanaAlto  = alto;
    std::cout << "[INIT] Ventana " << ancho << "x" << alto << " centrada\n";
    return true;
}

void Aplicacion::alternarPantallaCompleta() {
    if (!ventana) return;
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (!monitor) return;
    const GLFWvidmode* modo = glfwGetVideoMode(monitor);
    if (!modo) return;

    if (!enPantallaCompleta) {
        // Se memoriza el estado antes de saltar, o al volver la ventana
        // aparecería con un tamano arbitrario en la esquina.
        glfwGetWindowPos(ventana, &ventanaX, &ventanaY);
        glfwGetWindowSize(ventana, &ventanaAncho, &ventanaAlto);
        glfwSetWindowMonitor(ventana, monitor, 0, 0, modo->width, modo->height, modo->refreshRate);
        enPantallaCompleta = true;
    } else {
        glfwSetWindowMonitor(ventana, nullptr, ventanaX, ventanaY, ventanaAncho, ventanaAlto, 0);
        enPantallaCompleta = false;
    }

    // El callback de framebuffer ya reajusta el viewport, pero se fuerza aqui
    // por si el gestor de ventanas no emite el evento en el mismo frame. La
    // proyeccion no hace falta tocarla: el aspecto se recalcula cada frame a
    // partir del tamano vigente del framebuffer.
    int fbAncho = 0, fbAlto = 0;
    glfwGetFramebufferSize(ventana, &fbAncho, &fbAlto);
    if (fbAncho > 0 && fbAlto > 0) glViewport(0, 0, fbAncho, fbAlto);
    controlador.inicializar(ventana, &escena, &camara);
}

bool Aplicacion::inicializar() {
    // 1) MODELO primero: si no hay terrenos no tiene sentido abrir ventana.
    if (!escena.inicializar()) return false;

    // 2) Ventana y contexto de OpenGL 3.3 Core.
    std::cout << "[INIT] Inicializando GLFW\n";
    if (!glfwInit()) { std::cerr << "[ERROR] GLFW no pudo inicializarse. Verifica el controlador grafico.\n"; return false; }

    if (!crearVentana()) {
        std::cerr << "[ERROR] No se pudo crear la ventana OpenGL 3.3.\n";
        glfwTerminate();
        return false;
    }
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

    // La camara adopta la escala del terreno activo: distancia, planos de
    // recorte y limites de zoom salen de ahi, no de constantes.
    camara.aplicarEscala(escena.obtenerEscala());
    camara.seguir(escena.obtenerDron().obtenerPosicion());
    camara.saltarAObjetivo();   // el primer frame no debe venir interpolado
    actualizarTitulo();

    std::cout << "[INIT] OpenGL " << glGetString(GL_VERSION) << " | GPU "
              << glGetString(GL_RENDERER) << "\n";
    std::cout << "[INIT] WASD mover | SPACE/SHIFT altura | MOUSE camara | TAB mapa | "
                 "F3 datos | F11 pantalla completa | ESC salir\n";
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
    camara.aplicarEscala(escena.obtenerEscala());
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
        if (controlador.consumirAlternarPantallaCompleta()) alternarPantallaCompleta();
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
        camara.confinarAlTerreno(escena.obtenerTerreno());
        renderizador.renderizar(escena, camara,
                                controlador.obtenerAncho(), controlador.obtenerAlto(),
                                controlador.obtenerEstadoTeclas(), dt,
                                controlador.obtenerOpcionMenu(),
                                controlador.obtenerMapaSeleccionado());

        glfwSwapBuffers(ventana);
        glfwPollEvents();
    }
}

void Aplicacion::liberar() {
    renderizador.liberar();
    if (ventana) glfwDestroyWindow(ventana);
    glfwTerminate();
}
