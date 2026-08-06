#pragma once
#include <glm/glm.hpp>
#include <string>

// ============================================================================
//  Configuracion global del simulador.
//
//  IMPORTANTE: aqui solo viven constantes que NO dependen del tamano del mapa
//  (resoluciones, semillas, colores, ritmos de animacion, rutas). Todo lo que
//  se mide en unidades de mundo -velocidades, alturas, radios, distancias de
//  camara, planos de recorte- se calcula por terreno en EscalaMundo, para que
//  la misma proporcion visual valga en cualquier DEM.
// ============================================================================
namespace Configuracion {

// ---- Terreno ----
inline constexpr float ANCHO_OBJETIVO     = 100.0f;  // ancho al que se normaliza cualquier mapa
inline constexpr float EXAGERACION_Y      = 1.0f;    // multiplicador del relieve
inline constexpr int   PUNTOS_MAX         = 35000;   // tope de la nube de puntos diluida
inline constexpr int   RESOLUCION_GRILLA  = 256;     // lado del mapa de alturas
// 257 VERTICES = 256 celdas = 2^8, divisible entre 2^5: es lo que permite que
// el Quadtree de profundidad 5 corte siempre sobre vertices existentes.
// Se subio de 129 a 257 al reducir la escala del dron: con la camara mucho mas
// cerca del relieve, la cuadricula anterior se leia gruesa. El quadtree sigue
// descartando lo que queda fuera del frustum, asi que el coste real apenas sube.
inline constexpr int   RESOLUCION_REJILLA = 257;

// ---- Proporciones adimensionales (las consume EscalaMundo) ----------------
// Diagonal del dron / diagonal del terreno. Con 0.0022 sobre un mapa de
// diagonal ~141 el dron mide ~0.31 unidades: se lee como un objeto pequeno
// sobre un relieve inmenso, que es la proporcion de referencia.
inline constexpr float RATIO_DRON_TERRENO     = 0.0022f;
// Altura de una estaca de sondeo / diagonal del terreno.
inline constexpr float RATIO_ALTURA_MARCADOR  = 0.0045f;

inline constexpr float ALPHA_REJILLA    = 1.0f;
inline constexpr float ALPHA_MARCADORES = 0.95f;

// ---- Marcadores de sondeo ----
inline constexpr int          NUM_MARCADORES     = 44;
inline constexpr unsigned int SEMILLA_MARCADORES = 20261u;  // fija = escena reproducible
// Variacion de altura entre estacas, como fraccion de la altura nominal.
inline constexpr float        MARCADOR_VARIACION = 0.45f;

// ---- Dron ----
// El modelo se normaliza a extension maxima 1.0 al cargarlo; la escala real la
// aplica EscalaMundo sobre la transformada, no sobre la malla, para que un
// mismo GLB sirva a mapas de tamanos distintos.
inline constexpr float TAMANIO_DRON       = 1.0f;
// Umbral del angulo diedro para considerar que una arista es un canto real.
inline constexpr float ANGULO_ARISTA_DRON = 26.0f;
inline constexpr float INCLINACION_MAX    = 11.0f;   // grados de pitch/roll
inline constexpr float GIRO_HELICES       = 18.0f;   // rad/seg (solo visual)
inline constexpr float OFFSET_YAW_DRON    = 0.0f;    // correccion si el modelo mira mal
// Constantes de tiempo de la interpolacion critica (1/s): mas alto = mas rapido
// en llegar, nunca sobrepasa porque es exponencial, no elastica.
inline constexpr float SUAVIZADO_YAW      = 4.5f;
inline constexpr float SUAVIZADO_INCLINACION = 4.0f;

// ---- Camara orbital ----
inline constexpr float CAM_YAW_INICIAL    = 45.0f;
// Ligeramente por encima del dron: es lo que deja ver a la vez el aparato y el
// terreno que esta barriendo debajo, sin caer en vista cenital.
inline constexpr float CAM_ELEV_INICIAL   = 24.0f;
inline constexpr float CAM_BASE_SUAVIZADO = 0.0025f; // fraccion pendiente tras 1 s
inline constexpr float CAM_ELEV_MIN       = 4.0f;
inline constexpr float CAM_ELEV_MAX       = 80.0f;
inline constexpr float CAM_FOV            = 58.0f;   // dentro del rango 50-65 pedido
inline constexpr float CAM_SENSIBILIDAD   = 0.10f;   // grados por pixel de arrastre

// ---- Ventana ----
inline constexpr float FRACCION_VENTANA_X = 0.95f;   // del ancho del monitor
inline constexpr float FRACCION_VENTANA_Y = 0.92f;   // del alto del monitor
inline constexpr int   ANCHO_VENTANA_MIN  = 1024;    // reserva si GLFW no da monitor
inline constexpr int   ALTO_VENTANA_MIN   = 640;

// ---- Escaneo / niebla de guerra ----
// Lote que desencola el sistema. Al doblar el radio del radar el area (y por
// tanto la cola inicial) se multiplica por cuatro: con un lote mayor el barrido
// se resuelve en pocos frames en vez de arrastrarse.
inline constexpr int   CELDAS_POR_FRAME        = 2048;
inline constexpr float UMBRAL_MISION_COMPLETA  = 0.995f;
inline constexpr float DURACION_FADE_PANEL     = 1.2f;   // segundos
inline constexpr int   NUM_PUNTOS_ESCANEO      = 10;
inline constexpr unsigned int SEMILLA_ESCANEO  = 42631u;

// ---- Luz de escaneo bajo el dron ----
inline constexpr int   SEGMENTOS_CONO   = 40;
inline constexpr int   ANILLOS_RADAR    = 3;
inline constexpr float PERIODO_ANILLO   = 2.4f;   // segundos que tarda en expandirse
inline constexpr float PERIODO_PULSO    = 1.6f;   // respiracion del cono
// Bajo a proposito: el cono se ve desde delante y desde detras a la vez y la
// mezcla aditiva suma ambas caras. Con mas alpha deja de leerse como luz.
inline constexpr float ALPHA_CONO       = 0.17f;  // en el vertice, bajo el dron
inline constexpr float ALPHA_HUELLA     = 0.60f;  // centro de la elipse proyectada

// ---- Curvas de nivel ----
inline constexpr int   NIVELES_CURVAS    = 12;
inline constexpr int   PASO_CURVAS       = 2;     // submuestreo de la grilla
inline constexpr float INTERVALO_CURVAS  = 0.2f;  // regeneracion como mucho cada 200 ms

// ---- Quadtree LOD + culling ----
inline constexpr int   PROFUNDIDAD_QUADTREE = 5;
inline constexpr float FACTOR_LOD           = 1.6f;
inline constexpr float INTERVALO_FPS        = 0.5f;   // ventana de promediado

// ---- Glow / post-proceso ----
inline constexpr int   PASADAS_DESENFOQUE          = 8;     // 4 horizontales + 4 verticales
inline constexpr int   DIVISOR_RESOLUCION_BRILLO   = 2;     // el halo se desenfoca a media res
inline constexpr float UMBRAL_BRILLO               = 0.20f;
inline constexpr float INTENSIDAD_GLOW             = 0.95f;

// ---- Textos fijos del HUD (sin tildes: stb_easy_font solo cubre ASCII) ----
inline const std::string TITULO_APP    = "GEODRONE";
inline const std::string SUBTITULO_APP = "EXPLORACION TOPOGRAFICA";
inline const std::string PIE_PROYECTO  = "UANCV / COMPUTACION GRAFICA SIS226";

// ---- Aviso inicial ----
inline constexpr float DURACION_PISTA_INICIAL = 5.0f;  // segundos visible
inline constexpr float FADE_PISTA_INICIAL     = 1.5f;  // segundos de desvanecido

// ---- Persistencia ----
inline constexpr float DURACION_AVISO = 2.6f;   // segundos que dura el mensaje

// ---- Rutas ----
inline const std::string RUTA_DRON        = "assets/animated_drone.glb";
inline const std::string CARPETA_ASSETS   = "assets";
inline const std::string RUTA_GUARDADO    = "guardado.json";

} // namespace Configuracion

// ============================================================================
//  Paleta propia del simulador GeoDrone.
// ============================================================================
namespace Paleta {
inline const glm::vec3 FONDO        {0.012f, 0.022f, 0.043f};
inline const glm::vec3 FONDO_ALTO   {0.025f, 0.054f, 0.094f};
inline const glm::vec3 REJILLA      {0.72f, 0.86f, 1.00f};
inline const glm::vec3 REJILLA_SEC  {0.25f, 0.43f, 0.60f};
inline const glm::vec3 ACENTO       {1.000f, 0.82f, 0.02f};
inline const glm::vec3 TEXTO_SEC    {0.55f, 0.64f, 0.73f};
inline const glm::vec3 BLANCO       {0.95f, 0.98f, 1.00f};
inline const glm::vec3 CIAN         {0.10f, 0.88f, 0.92f};
inline const glm::vec3 VERDE        {0.20f, 0.92f, 0.58f};
inline const glm::vec3 ALERTA       {1.00f, 0.34f, 0.20f};
// Luz del escaner: blanco calido, no amarillo saturado, para que se lea como
// luz proyectada y no como geometria solida.
inline const glm::vec3 LUZ_ESCANER  {1.00f, 0.95f, 0.72f};
} // namespace Paleta
