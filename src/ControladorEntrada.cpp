// glad SIEMPRE antes que GLFW: glfw3.h incluye GL/gl.h y glad se niega a
// compilar si ese encabezado ya entro.
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "ControladorEntrada.h"
#include "Camara.h"
#include "Configuracion.h"
#include "Escena.h"
#include "MenuConfiguracion.h"
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
    // Minimizar entrega 0x0: guardar ese tamano dejaria el aspecto en NaN y la
    // proyeccion degenerada al restaurar.
    if (ancho <= 0 || alto <= 0) return;
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
    if (self && self->camara) self->camara->acercar((float)dy);
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
        // Ya no hay botones en el HUD que interceptar: la interfaz en vuelo es
        // solo texto y minimapa, asi que el clic izquierdo va directo a la
        // camara (o a la herramienta de medicion si esta activa).
        if (escena && escena->obtenerSistemaMedicion().estaActivo() &&
            escena->obtenerEstadoAplicacion() == EstadoAplicacion::Playing) {
            double mx, my;
            glfwGetCursorPos(ventana, &mx, &my);
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

    // Sensibilidad baja a proposito: la camara debe acompanar, no latiguear.
    float sensibilidad = escena ? escena->obtenerAjustes().sensibilidadMouse
                                : Configuracion::CAM_SENSIBILIDAD;
    camara->orbitar((float)(x - ultimoX) * sensibilidad,
                    (float)(ultimoY - y) * sensibilidad);
    ultimoX = x;
    ultimoY = y;
}

void ControladorEntrada::abrirMenu() {
    opcionMenu = 0;
    mapaSeleccionado = escena->obtenerMapaActual();
    escena->establecerEstadoAplicacion(EstadoAplicacion::Paused);
}

bool ControladorEntrada::manejarTeclaMenu(int tecla) {
    if (escena->obtenerEstadoAplicacion() != EstadoAplicacion::Paused) return false;

    if (tecla == GLFW_KEY_ESCAPE) {
        escena->establecerEstadoAplicacion(EstadoAplicacion::Playing);
        return true;
    }
    if (tecla == GLFW_KEY_UP || tecla == GLFW_KEY_W)
        { opcionMenu = (opcionMenu - 1 + OPCIONES_MENU) % OPCIONES_MENU; return true; }
    if (tecla == GLFW_KEY_DOWN || tecla == GLFW_KEY_S)
        { opcionMenu = (opcionMenu + 1) % OPCIONES_MENU; return true; }

    // Izquierda/derecha solo tienen sentido en las filas con valor.
    int paso = (tecla == GLFW_KEY_LEFT || tecla == GLFW_KEY_A) ? -1
             : (tecla == GLFW_KEY_RIGHT || tecla == GLFW_KEY_D) ? 1 : 0;
    if (paso != 0) {
        int total = (int)escena->obtenerMapas().size();
        if (opcionMenu == FILA_MAPA && total > 0)
            mapaSeleccionado = (mapaSeleccionado + paso + total) % total;
        else if (opcionMenu == FILA_CONTROLES)
            escena->obtenerAjustes().controles = !escena->obtenerAjustes().controles;
        return true;
    }

    if (tecla == GLFW_KEY_ENTER || tecla == GLFW_KEY_KP_ENTER || tecla == GLFW_KEY_SPACE) {
        ejecutarOpcionMenu();
        return true;
    }
    // En el menu no se pilota: cualquier otra tecla se ignora.
    return true;
}

void ControladorEntrada::ejecutarOpcionMenu() {
    switch (opcionMenu) {
        case FILA_CONTINUAR:
            escena->establecerEstadoAplicacion(EstadoAplicacion::Playing);
            break;
        case FILA_MAPA:
            // Cargar un GLB de un millon de vertices es caro: solo se hace al
            // confirmar, no cada vez que el usuario mueve la seleccion.
            if (mapaSeleccionado != escena->obtenerMapaActual()) {
                mapaSolicitado = mapaSeleccionado;
                cambioMapaPendiente = true;
            }
            escena->establecerEstadoAplicacion(EstadoAplicacion::Playing);
            break;
        case FILA_REINICIAR:
            escena->reiniciarEscaneo();
            escena->mostrarAviso("ESCANEO REINICIADO");
            escena->establecerEstadoAplicacion(EstadoAplicacion::Playing);
            break;
        case FILA_CONTROLES:
            escena->obtenerAjustes().controles = !escena->obtenerAjustes().controles;
            break;
        case FILA_SALIR:
            glfwSetWindowShouldClose(ventana, true);
            break;
        default: break;
    }
}

void ControladorEntrada::manejarTecla(int tecla, int accion) {
    if (accion != GLFW_PRESS) return;
    if (!escena) return;

    // F11 funciona tambien con el menu abierto: es de ventana, no de juego.
    if (tecla == GLFW_KEY_F11) { alternarPantallaCompleta = true; return; }

    // Con el menu abierto, el teclado es suyo.
    if (manejarTeclaMenu(tecla)) return;

    // ESC abre el menu de configuracion (continuar, cambiar mapa, reiniciar
    // escaneo, controles, salir). Salir sigue estando a un paso.
    if (tecla == GLFW_KEY_ESCAPE) { abrirMenu(); return; }

    // F3 alterna TODA la informacion tecnica, que por defecto esta oculta.
    if (tecla == GLFW_KEY_F3) {
        Ajustes& a = escena->obtenerAjustes();
        a.modoDebug = !a.modoDebug;
        escena->mostrarAviso(a.modoDebug ? "DATOS TECNICOS ACTIVOS" : "DATOS TECNICOS OCULTOS");
        return;
    }

    if (tecla == GLFW_KEY_P) { abrirMenu(); return; }

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
    if (tecla == GLFW_KEY_X) { escena->obtenerSistemaMedicion().alternar(); escena->mostrarAviso(escena->obtenerSistemaMedicion().estaActivo() ? "MODO MEDICION ACTIVO" : "MODO MEDICION CERRADO"); return; }
    if (tecla == GLFW_KEY_R) { escena->reiniciarEscaneo(); escena->mostrarAviso("ESCANEO REINICIADO"); return; }

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
    (void)dt;
    if (!ventana || !escena || !camara) return;

    Dron& dron = escena->obtenerDron();
    if (escena->obtenerEstadoAplicacion() != EstadoAplicacion::Playing) {
        // En pausa se sigue entregando "sin entrada" para que el dron frene y
        // se quede exactamente donde estaba, en vez de congelar su velocidad.
        teclas = EstadoTeclas{};
        dron.mover(glm::vec3(0.0f));
        dron.ajustarAltura(0.0f);
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

    // Se entrega una DIRECCION, no un desplazamiento: la velocidad, el limite
    // de aceleracion y la integracion por deltaTime son cosa del Modelo.
    dron.mover(direccion);
    dron.ajustarAltura((teclas.espacio ? 1.0f : 0.0f) - (teclas.shift ? 1.0f : 0.0f));
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

int ControladorEntrada::consumirCambioDeMapa() {
    cambioMapaPendiente = false;
    return mapaSolicitado;
}
