#include "Escena.h"
#include "CargadorModelos.h"
#include "Configuracion.h"

#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace fs = std::filesystem;

Escena::Escena() = default;

bool Escena::inicializar() {
    estadoAplicacion = EstadoAplicacion::Loading;
    std::cout << "[INIT] Descubriendo mapas en assets/\n";
    mapas = CargadorModelos::descubrirTerrenos(Configuracion::CARPETA_ASSETS);
    if (mapas.empty()) {
        mensajeError = "No se encontraron terrenos compatibles en assets/.";
        estadoAplicacion = EstadoAplicacion::Error;
        std::cerr << "[ERROR] " << mensajeError << "\n";
        return false;
    }
    std::cout << "Terrenos:\n";
    for (int i = 0; i < (int)mapas.size(); i++)
        std::cout << "  [" << (i + 1) << "] " << fs::path(mapas[i]).filename().string() << "\n";

    // El dron se carga una sola vez: su malla no depende del mapa activo.
    if (fs::exists(Configuracion::RUTA_DRON)) {
        dronCargado = CargadorModelos::cargarDronAnimado(
            Configuracion::RUTA_DRON, mallaDron, pivotesHelices, Configuracion::TAMANIO_DRON);
    }
    if (!dronCargado) std::cerr << "AVISO: no se cargo el dron\n";

    construirGrafo();
    construirEntidades();
    if (!cargarMapa(0)) {
        mensajeError = "No fue posible cargar el primer terreno disponible.";
        estadoAplicacion = EstadoAplicacion::Error;
        return false;
    }
    estadoAplicacion = EstadoAplicacion::Intro;
    return true;
}

void Escena::construirGrafo() {
    // raiz -> { terreno, dron -> { helice1..4, sensor }, marcadores, luces }
    nodoTerreno = raiz.agregarHijo(std::make_unique<NodoEscena>("terreno"));
    nodoDron    = raiz.agregarHijo(std::make_unique<NodoEscena>("dron"));

    for (int i = 0; i < 4; i++) {
        nodosHelice[i] = nodoDron->agregarHijo(
            std::make_unique<NodoEscena>("helice" + std::to_string(i + 1)));
        // La local de cada helice es la traslacion a su pivote: asi el nodo ya
        // representa el eje de giro real dentro del espacio del dron.
        nodosHelice[i]->establecerTransformacionLocal(
            glm::translate(glm::mat4(1.0f), pivotesHelices[i]));
    }
    nodoSensor     = nodoDron->agregarHijo(std::make_unique<NodoEscena>("sensor"));
    nodoMarcadores = raiz.agregarHijo(std::make_unique<NodoEscena>("marcadores"));
    nodoLuces      = raiz.agregarHijo(std::make_unique<NodoEscena>("luces"));

    std::cout << "Grafo de escena: " << raiz.contarDescendientes() << " nodos bajo la raiz\n";
}

void Escena::construirEntidades() {
    // ---- Terreno ----
    auto* transTerreno = entidadTerreno.agregarComponente<ComponenteTransformada>();
    transTerreno->nodo = nodoTerreno;

    auto* mallaTerreno = entidadTerreno.agregarComponente<ComponenteMalla>();
    mallaTerreno->malla = &terreno.obtenerMalla();

    auto* matTerreno = entidadTerreno.agregarComponente<ComponenteMaterial>();
    matTerreno->color = Paleta::REJILLA;
    matTerreno->alpha = Configuracion::ALPHA_REJILLA;
    matTerreno->dibujarRelleno = false;

    // ---- Dron ----
    auto* transDron = entidadDron.agregarComponente<ComponenteTransformada>();
    transDron->nodo = nodoDron;

    auto* mallaDronComp = entidadDron.agregarComponente<ComponenteMalla>();
    mallaDronComp->malla = &mallaDron;

    auto* matDron = entidadDron.agregarComponente<ComponenteMaterial>();
    matDron->dibujarRelleno = true;
    matDron->colorRelleno   = glm::vec3(0.10f, 0.09f, 0.02f);  // silueta: tapa el interior
    matDron->alphaRelleno   = 1.0f;
    matDron->color          = Paleta::ACENTO;                  // aristas amarillas
    matDron->alpha          = 1.0f;
    matDron->emision        = 1.0f;                            // es lo unico que genera glow

    auto* animDron = entidadDron.agregarComponente<ComponenteAnimacion>();
    animDron->velocidadGiro = Configuracion::GIRO_HELICES;

    auto* escaner = entidadDron.agregarComponente<ComponenteEscaner>();
    escaner->radioEscaneo   = Configuracion::RADIO_ESCANEO;
    escaner->celdasPorFrame = Configuracion::CELDAS_POR_FRAME;
}

bool Escena::cargarMapa(int indice) {
    if (indice < 0 || indice >= (int)mapas.size()) return false;
    mapaActual = indice;

    const std::string& ruta = mapas[mapaActual];
    std::cout << "\nCargando [" << (mapaActual + 1) << "/" << mapas.size() << "] "
              << fs::path(ruta).filename().string() << " ...\n";

    if (!terreno.cargarDesdeArchivo(ruta)) {
        mensajeError = "Fallo al cargar " + fs::path(ruta).filename().string();
        std::cerr << "[ERROR] " << mensajeError << "\n";
        return false;
    }

    // La malla cambio de contenido: la Vista debe volver a subirla a la GPU.
    if (auto* comp = entidadTerreno.obtenerComponente<ComponenteMalla>()) {
        comp->malla = &terreno.obtenerMalla();
        comp->topologiaLineas = terreno.esMallaDeLineas();
        comp->dirty = true;
    }

    // Niebla de guerra limpia para el nuevo mapa.
    mapaExploracion.reiniciar(terreno.obtenerAnchoGrilla(), terreno.obtenerAnchoGrilla(),
                              terreno.obtenerLimites());
    sistemaEscaneo.reiniciar(terreno.obtenerAnchoGrilla(), terreno.obtenerAnchoGrilla());
    estadoMision.reiniciar();
    generadorCurvas.reiniciar();
    curvas.limpiar();
    curvas.establecerRango(terreno.obtenerLimites().minY, terreno.obtenerLimites().maxY);
    curvasSucias = true;   // el panel debe vaciarse al cambiar de mapa

    // Estacas resembradas con la misma semilla: cada mapa tiene su propio
    // patron, pero siempre el mismo entre ejecuciones.
    marcadores.generar(terreno, Configuracion::NUM_MARCADORES, Configuracion::SEMILLA_MARCADORES);
    sistemaExploracion.generar(terreno, Configuracion::NUM_PUNTOS_ESCANEO,
                               Configuracion::SEMILLA_ESCANEO + static_cast<unsigned int>(indice));
    sistemaMedicion.limpiar();

    // Dron al centro del mapa, a una altura segura sobre el relieve.
    dron.establecerPosicion(glm::vec3(0.0f, terreno.alturaEn(0.0f, 0.0f) + Configuracion::ALTURA_INICIAL, 0.0f));
    dron.establecerYaw(0.0f);
    if (estadoAplicacion != EstadoAplicacion::Loading)
        estadoAplicacion = EstadoAplicacion::Playing;
    return true;
}

bool Escena::reiniciarMision() {
    return cargarMapa(mapaActual);
}

void Escena::mostrarAviso(const std::string& texto) {
    aviso.mostrar(texto, Configuracion::DURACION_AVISO);
}

int Escena::buscarMapaPorNombre(const std::string& nombreArchivo) const {
    for (int i = 0; i < (int)mapas.size(); ++i)
        if (fs::path(mapas[i]).filename().string() == nombreArchivo) return i;
    return -1;
}

bool Escena::aplicarGuardado(const std::string& nombreMapa,
                             const std::vector<uint32_t>& tiradasRLE,
                             int ancho, int alto,
                             const glm::vec3& posicionDron, float yawDron) {
    int indice = buscarMapaPorNombre(nombreMapa);
    if (indice < 0) return false;

    // cargarMapa deja la mascara a cero y con los limites correctos; solo
    // despues tiene sentido volcar encima la mascara guardada.
    if (indice != mapaActual || !terreno.estaCargado())
        if (!cargarMapa(indice)) return false;

    if (!mapaExploracion.descomprimirRLE(tiradasRLE, ancho, alto)) return false;

    // Todo lo derivado de la mascara queda obsoleto y hay que rehacerlo.
    sistemaEscaneo.reiniciar(terreno.obtenerAnchoGrilla(), terreno.obtenerAnchoGrilla());
    generadorCurvas.reiniciar();
    estadoMision.reiniciar();
    curvas.limpiar();
    curvasSucias = true;

    dron.establecerPosicion(posicionDron);
    dron.establecerYaw(yawDron);
    return true;
}

void Escena::actualizar(float dt) {
    aviso.actualizar(dt);
    const bool jugando = estadoAplicacion == EstadoAplicacion::Playing;
    if (jugando) dron.actualizar(terreno, dt);

    // Modelo -> componente transformada -> nodo del grafo.
    if (auto* trans = entidadDron.obtenerComponente<ComponenteTransformada>()) {
        trans->posicion      = dron.obtenerPosicion() + glm::vec3(0.0f, dron.obtenerFlotacion(), 0.0f);
        trans->rotacionEuler = glm::vec3(dron.obtenerPitch(), dron.obtenerYaw(), dron.obtenerRoll());
        trans->volcarAlNodo();
    }
    if (auto* trans = entidadTerreno.obtenerComponente<ComponenteTransformada>())
        trans->volcarAlNodo();

    if (jugando) if (auto* anim = entidadDron.obtenerComponente<ComponenteAnimacion>())
        anim->avanzar(dt);

    // ---- Escaneo: detectar encola, procesarLote desencola un lote acotado ----
    if (jugando) if (auto* escaner = entidadDron.obtenerComponente<ComponenteEscaner>()) {
        if (escaner->activo) {
            const glm::vec3& p = dron.obtenerPosicion();
            sistemaEscaneo.detectar(mapaExploracion, p.x, p.z, escaner->radioEscaneo);
            sistemaEscaneo.procesarLote(mapaExploracion, escaner->celdasPorFrame);
        }
    }
    if (jugando) {
        sistemaExploracion.actualizar(dron.obtenerPosicion(), dron.obtenerRapidez(),
                                      dt, mapaExploracion);
        estadoMision.actualizar(sistemaExploracion.obtenerProgresoPuntos(), dt);
        if (sistemaExploracion.estaCompleta()) {
            estadoAplicacion = EstadoAplicacion::MissionComplete;
            std::cout << "[MISSION] Exploracion completada en "
                      << sistemaExploracion.obtenerTiempoMision() << " segundos\n";
        }
    }

    // Marching Squares + grafo, con su propio freno de 200 ms.
    if (jugando && ajustes.curvasNivel &&
        generadorCurvas.actualizar(terreno, mapaExploracion, curvas, dt))
        curvasSucias = true;

    // Una sola pasada en preorden actualiza dron, sus 4 helices y el sensor.
    raiz.actualizarTransformaciones(glm::mat4(1.0f));
}
