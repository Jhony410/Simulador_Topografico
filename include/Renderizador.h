#pragma once
#include "Camara.h"
#include "ContadorRendimiento.h"
#include "EstadoEntrada.h"
#include "Frustum.h"
#include "GestorRecursos.h"
#include "PostProceso.h"
#include "VistaCurvas.h"
#include "VistaDron.h"
#include "VistaExploracion.h"
#include "VistaHUD.h"
#include "VistaMarcadores.h"
#include "VistaMinimapa.h"
#include "VistaTerreno.h"

#include <vector>

class Escena;

// ============================================================================
//  VISTA: orquestador de pasadas. Es el unico que decide el ORDEN en que se
//  dibuja (escena en FBO -> post-proceso -> HUD) y el estado global de OpenGL.
//  Tambien es quien resuelve la visibilidad: extrae el frustum de la camara y
//  consulta el Quadtree del terreno y el BVH de los marcadores.
// ============================================================================
class Renderizador {
public:
    bool inicializar();
    void liberar();

    // Sube a GPU la geometria que cambio en el Modelo (cambio de mapa).
    void sincronizarConEscena(Escena& escena);

    void renderizar(Escena& escena, const Camara& camara,
                    int anchoPantalla, int altoPantalla,
                    const EstadoTeclas& teclas, float dt,
                    int opcionMenu, int opcionConfiguracion, bool enConfiguracion);

    const ContadorRendimiento& obtenerMetricas() const { return metricas; }

private:
    GestorRecursos  recursos;
    VistaTerreno    vistaTerreno;
    VistaMarcadores vistaMarcadores;
    VistaMinimapa   vistaMinimapa;
    VistaDron       vistaDron;
    VistaExploracion vistaExploracion;
    VistaCurvas     vistaCurvas;
    VistaHUD        vistaHUD;
    PostProceso     postProceso;

    Frustum             frustum;
    ContadorRendimiento metricas;
    // Listas de visibilidad reutilizadas cada frame para no reservar memoria.
    std::vector<int> nodosTerrenoVisibles;
    std::vector<int> marcadoresVisibles;

    bool dronSubido = false;
    float tiempoTotal = 0.0f;
};
