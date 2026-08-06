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

#include <algorithm>
#include <glm/gtc/matrix_inverse.hpp>

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
    if (boton == GLFW_MOUSE_BUTTON_RIGHT && accion == GLFW_PRESS && escena &&
        escena->obtenerSistemaMedicion().estaActivo()) {
        escena->obtenerSistemaMedicion().limpiar();
        escena->mostrarAviso("MEDICIONES LIMPIADAS");
        return;
    }
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
        if (escena && escena->obtenerSistemaMedicion().estaActivo() &&
            escena->obtenerEstadoAplicacion() == EstadoAplicacion::Playing) {
            seleccionarMedicion(mx, my);
            return;
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

    float sensibilidad = escena ? escena->obtenerAjustes().sensibilidadMouse : 0.3f;
    camara->orbitar((float)(x - ultimoX) * sensibilidad,
                    (float)(ultimoY - y) * sensibilidad);
    ultimoX = x;
    ultimoY = y;
}

void ControladorEntrada::manejarTecla(int tecla, int accion) {
    if (accion != GLFW_PRESS) return;
    if (!escena) return;

    EstadoAplicacion estado = escena->obtenerEstadoAplicacion();
    if (tecla == GLFW_KEY_ESCAPE) {
        if (estado == EstadoAplicacion::Playing) {
            escena->establecerEstadoAplicacion(EstadoAplicacion::Paused);
            opcionMenu = 0;
        } else if (estado == EstadoAplicacion::Paused) {
            if (enConfiguracion) enConfiguracion = false;
            else escena->establecerEstadoAplicacion(EstadoAplicacion::Playing);
        } else if (estado == EstadoAplicacion::Intro) {
            glfwSetWindowShouldClose(ventana, true);
        }
        return;
    }

    if (estado == EstadoAplicacion::Intro || estado == EstadoAplicacion::Paused) {
        int maximo = enConfiguracion ? 7 : (estado == EstadoAplicacion::Intro ? 3 : 4);
        if (tecla == GLFW_KEY_UP || tecla == GLFW_KEY_W)
            (enConfiguracion ? opcionConfiguracion : opcionMenu) =
                ((enConfiguracion ? opcionConfiguracion : opcionMenu) - 1 + maximo + 1) % (maximo + 1);
        else if (tecla == GLFW_KEY_DOWN || tecla == GLFW_KEY_S)
            (enConfiguracion ? opcionConfiguracion : opcionMenu) =
                ((enConfiguracion ? opcionConfiguracion : opcionMenu) + 1) % (maximo + 1);
        else if (enConfiguracion && (tecla == GLFW_KEY_LEFT || tecla == GLFW_KEY_A)) ajustarConfiguracion(-1);
        else if (enConfiguracion && (tecla == GLFW_KEY_RIGHT || tecla == GLFW_KEY_D)) ajustarConfiguracion(1);
        else if (tecla == GLFW_KEY_ENTER) ejecutarOpcionMenu();
        return;
    }

    if (estado == EstadoAplicacion::MissionComplete) {
        if (tecla == GLFW_KEY_R) escena->reiniciarMision();
        else if (tecla == GLFW_KEY_TAB) {
            int total = static_cast<int>(escena->obtenerMapas().size());
            mapaSolicitado = (escena->obtenerMapaActual() + 1) % total;
            cambioMapaPendiente = true;
        } else if (tecla == GLFW_KEY_ENTER) escena->establecerEstadoAplicacion(EstadoAplicacion::Playing);
        return;
    }

    // F5 guarda, F9 recupera. El resultado se anuncia en el HUD tanto si sale
    // bien como si falla: un guardado silencioso que no funciona es peor que
    // ninguno.
    if (tecla == GLFW_KEY_F5 || tecla == GLFW_KEY_F9) {
        std::string mensaje;
        bool ok = (tecla == GLFW_KEY_F5)
                    ? Persistencia::guardar(*escena, Configuracion::RUTA_GUARDADO, mensaje)
                    : Persistencia::cargar(*escena, Configuracion::RUTA_GUARDADO, mensaje);
        escena->mostrarAviso(ok ? mensaje : ("ERROR: " + mensaje));
        return;
    }

    if (tecla == GLFW_KEY_C) { camara->siguienteModo(); escena->mostrarAviso(nombreModoCamara(camara->obtenerModo())); return; }
    if (tecla == GLFW_KEY_F) { camara->recentrar(); escena->mostrarAviso("CAMARA RECENTRADA"); return; }
    if (tecla == GLFW_KEY_V) { escena->obtenerAjustes().siguienteVisualizacion(); escena->mostrarAviso(nombreModoVisualizacion(escena->obtenerAjustes().visualizacion)); return; }
    if (tecla == GLFW_KEY_M) { escena->obtenerAjustes().minimapa = !escena->obtenerAjustes().minimapa; return; }
    if (tecla == GLFW_KEY_H) { escena->obtenerAjustes().curvasNivel = !escena->obtenerAjustes().curvasNivel; return; }
    if (tecla == GLFW_KEY_L) { escena->obtenerAjustes().rutaVuelo = !escena->obtenerAjustes().rutaVuelo; return; }
    if (tecla == GLFW_KEY_F1) { escena->obtenerAjustes().controles = !escena->obtenerAjustes().controles; return; }
    if (tecla == GLFW_KEY_F3) { escena->obtenerAjustes().estadisticas = !escena->obtenerAjustes().estadisticas; return; }
    if (tecla == GLFW_KEY_X) { escena->obtenerSistemaMedicion().alternar(); escena->mostrarAviso(escena->obtenerSistemaMedicion().estaActivo() ? "MODO MEDICION ACTIVO" : "MODO MEDICION CERRADO"); return; }
    if (tecla == GLFW_KEY_R) { escena->reiniciarMision(); escena->mostrarAviso("MISION REINICIADA"); return; }
    if (tecla == GLFW_KEY_P) { escena->establecerEstadoAplicacion(EstadoAplicacion::Paused); opcionMenu = 0; return; }

    int total = (int)escena->obtenerMapas().size();
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
    if (escena->obtenerEstadoAplicacion() != EstadoAplicacion::Playing) {
        teclas = EstadoTeclas{};
        escena->obtenerDron().mover(glm::vec3(0.0f), 0.0f, dt);
        return;
    }

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
    dron.mover(direccion, Configuracion::VEL_DRON * escena->obtenerAjustes().multiplicadorVelocidadDron, dt);
    if (teclas.espacio) dron.ajustarAltura( Configuracion::VEL_ALTURA * dt);
    if (teclas.shift)   dron.ajustarAltura(-Configuracion::VEL_ALTURA * dt);
}

void ControladorEntrada::seleccionarMedicion(double x, double y) {
    if (!camara || !escena || anchoPantalla <= 0 || altoPantalla <= 0) return;
    float ndcX = 2.0f * static_cast<float>(x) / anchoPantalla - 1.0f;
    float ndcY = 1.0f - 2.0f * static_cast<float>(y) / altoPantalla;
    float aspecto = static_cast<float>(anchoPantalla) / altoPantalla;
    glm::mat4 inversa = glm::inverse(camara->matrizProyeccion(aspecto) * camara->matrizVista());
    glm::vec4 cerca = inversa * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
    glm::vec4 lejos = inversa * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
    cerca /= cerca.w;
    lejos /= lejos.w;
    glm::vec3 direccion = glm::normalize(glm::vec3(lejos - cerca));
    if (escena->obtenerSistemaMedicion().seleccionarTerrenoPorRayo(
            camara->obtenerPosicion(), direccion, escena->obtenerTerreno()))
        escena->mostrarAviso("PUNTO TOPOGRAFICO REGISTRADO");
    else
        escena->mostrarAviso("SIN INTERSECCION CON EL TERRENO");
}

void ControladorEntrada::ejecutarOpcionMenu() {
    EstadoAplicacion estado = escena->obtenerEstadoAplicacion();
    if (enConfiguracion) { enConfiguracion = false; return; }
    if (estado == EstadoAplicacion::Intro) {
        if (opcionMenu == 0) escena->establecerEstadoAplicacion(EstadoAplicacion::Playing);
        else if (opcionMenu == 1) {
            int total = static_cast<int>(escena->obtenerMapas().size());
            mapaSolicitado = (escena->obtenerMapaActual() + 1) % total;
            cambioMapaPendiente = true;
        } else if (opcionMenu == 2) escena->obtenerAjustes().controles = !escena->obtenerAjustes().controles;
        else glfwSetWindowShouldClose(ventana, true);
    } else if (estado == EstadoAplicacion::Paused) {
        if (opcionMenu == 0) escena->establecerEstadoAplicacion(EstadoAplicacion::Playing);
        else if (opcionMenu == 1) escena->reiniciarMision();
        else if (opcionMenu == 2) {
            int total = static_cast<int>(escena->obtenerMapas().size());
            mapaSolicitado = (escena->obtenerMapaActual() + 1) % total;
            cambioMapaPendiente = true;
        } else if (opcionMenu == 3) { enConfiguracion = true; opcionConfiguracion = 0; }
        else glfwSetWindowShouldClose(ventana, true);
    }
}

void ControladorEntrada::ajustarConfiguracion(int direccion) {
    Ajustes& a = escena->obtenerAjustes();
    switch (opcionConfiguracion) {
        case 0: a.sensibilidadMouse = std::clamp(a.sensibilidadMouse + direccion * 0.05f, 0.10f, 0.80f); break;
        case 1: a.multiplicadorVelocidadDron = std::clamp(a.multiplicadorVelocidadDron + direccion * 0.15f, 0.50f, 2.0f); break;
        case 2: a.intensidadPuntos = std::clamp(a.intensidadPuntos + direccion * 0.10f, 0.10f, 1.0f); break;
        case 3: a.densidadWireframe = std::clamp(a.densidadWireframe + direccion, 0, 2); break;
        case 4: a.curvasNivel = !a.curvasNivel; break;
        case 5: a.particulas = !a.particulas; break;
        case 6: a.minimapa = !a.minimapa; break;
        case 7: a.estadisticas = !a.estadisticas; break;
    }
}

int ControladorEntrada::consumirCambioDeMapa() {
    cambioMapaPendiente = false;
    return mapaSolicitado;
}
