// ============================================================================
//  Simulador Topografico con Dron  -  FASE 1: Visor del terreno
//  OpenGL 3.3 Core Profile + C++17
//
//  Soporta .obj  y  .glb / .gltf   (via tinygltf, header-only)
//
//  CONTROLES
//    Flechas / WASD : mover la camara por el mapa (con colision, no atraviesa)
//    Arrastrar mouse: rotar la vista (orbital)
//    Scroll         : zoom
//    Tab            : siguiente modelo en assets/
//    Backspace      : modelo anterior
//    1..9           : saltar directo al modelo N
//    ESC            : salir
// ============================================================================

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE   // no necesitamos escribir imagenes
#include "tiny_gltf.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <unordered_map>

namespace fs = std::filesystem;

// ---- Constantes de ventana ----
const unsigned int SCR_WIDTH  = 1280;
const unsigned int SCR_HEIGHT = 720;

// ---- Parametros del terreno ----
const float ANCHO_OBJETIVO   = 100.0f; // ancho final del terreno (X/Z)
const float EXAGERACION_Y    = 1.0f;   // factor de altura
const int   PUNTOS_MAX       = 35000;  // tope de puntos a dibujar (se diluye)

// ---- Camara / movimiento ----
const float VEL_MOVIMIENTO   = 45.0f;  // unidades por segundo al moverse
const float ALTURA_CAMINANTE = 2.0f;   // cuanto se eleva el foco sobre el suelo
const float MARGEN_CAMARA    = 2.5f;   // colision: la camara no baja del suelo

// ============================================================================
//  CAMARA ORBITAL (orbita un "foco" que se mueve por el mapa)
// ============================================================================
float camYaw    = 45.0f;
float camElev   = 35.0f;
float camRadius = 70.0f;   // mas cerca que antes (era 170)
glm::vec3 camFoco(0.0f);   // punto al que mira / orbita la camara

const float ELEV_MIN  = 12.0f;
const float ELEV_MAX  = 75.0f;
const float RADIO_MIN = 18.0f;   // permite acercarse mas
const float RADIO_MAX = 260.0f;

bool   arrastrando = false;
bool   primerMouse = true;
double lastX = SCR_WIDTH  / 2.0;
double lastY = SCR_HEIGHT / 2.0;

// ============================================================================
//  SELECTOR DE MODELOS
// ============================================================================
std::vector<std::string> listaModelos;
int  modeloActual    = 0;
bool recargarModelo  = false;

// ============================================================================
//  MAPA DE ALTURAS (para movimiento y colision sobre el terreno)
// ============================================================================
std::vector<float> g_altura;          // grid gridN x gridN con la altura (Y)
int   g_gridN = 0;
float g_minX = -50, g_maxX = 50;
float g_minZ = -50, g_maxZ = 50;
float g_minY = 0,   g_maxY = 0;

// Devuelve la altura del terreno (Y) en una posicion (x,z) por interpolacion
float alturaTerreno(float x, float z) {
    if (g_gridN <= 0) return 0.0f;
    float fx = (x - g_minX) / (g_maxX - g_minX) * (g_gridN - 1);
    float fz = (z - g_minZ) / (g_maxZ - g_minZ) * (g_gridN - 1);
    fx = std::clamp(fx, 0.0f, (float)(g_gridN - 1));
    fz = std::clamp(fz, 0.0f, (float)(g_gridN - 1));
    int x0 = (int)fx, z0 = (int)fz;
    int x1 = std::min(x0 + 1, g_gridN - 1);
    int z1 = std::min(z0 + 1, g_gridN - 1);
    float tx = fx - x0, tz = fz - z0;
    float h00 = g_altura[z0 * g_gridN + x0];
    float h10 = g_altura[z0 * g_gridN + x1];
    float h01 = g_altura[z1 * g_gridN + x0];
    float h11 = g_altura[z1 * g_gridN + x1];
    float a = h00 * (1 - tx) + h10 * tx;
    float b = h01 * (1 - tx) + h11 * tx;
    return a * (1 - tz) + b * tz;
}

// ============================================================================
//  SHADERS
// ============================================================================
std::string cargarFuenteShader(const char* ruta) {
    std::ifstream archivo(ruta);
    if (!archivo.is_open()) { std::cerr << "ERROR: no se pudo abrir " << ruta << "\n"; return ""; }
    std::ostringstream ss; ss << archivo.rdbuf();
    return ss.str();
}

GLuint crearProgramaShader(const char* rutaVertex, const char* rutaFragment) {
    std::string vSrc = cargarFuenteShader(rutaVertex);
    std::string fSrc = cargarFuenteShader(rutaFragment);
    const char* vCode = vSrc.c_str();
    const char* fCode = fSrc.c_str();
    int ok; char log[512];

    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &vCode, nullptr); glCompileShader(vert);
    glGetShaderiv(vert, GL_COMPILE_STATUS, &ok);
    if (!ok) { glGetShaderInfoLog(vert, 512, nullptr, log); std::cerr << "ERROR vertex:\n" << log << "\n"; }

    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &fCode, nullptr); glCompileShader(frag);
    glGetShaderiv(frag, GL_COMPILE_STATUS, &ok);
    if (!ok) { glGetShaderInfoLog(frag, 512, nullptr, log); std::cerr << "ERROR fragment:\n" << log << "\n"; }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert); glAttachShader(prog, frag); glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) { glGetProgramInfoLog(prog, 512, nullptr, log); std::cerr << "ERROR link:\n" << log << "\n"; }

    glDeleteShader(vert); glDeleteShader(frag);
    return prog;
}

// ============================================================================
//  CARGADOR OBJ  (devuelve posiciones crudas + indices, SIN normalizar)
// ============================================================================
int indicePosicion(const std::string& tok) {
    size_t s = tok.find('/');
    std::string num = (s == std::string::npos) ? tok : tok.substr(0, s);
    return std::stoi(num) - 1;
}

bool cargarOBJ(const std::string& ruta,
               std::vector<glm::vec3>&    outPos,
               std::vector<unsigned int>& outIdx)
{
    std::ifstream archivo(ruta);
    if (!archivo.is_open()) { std::cerr << "ERROR: no se pudo abrir " << ruta << "\n"; return false; }

    std::vector<glm::vec3>    tempPos;
    std::vector<unsigned int> triGlobal;
    bool objetoActivo = true;

    std::string linea;
    while (std::getline(archivo, linea)) {
        if (linea.empty() || linea[0] == '#') continue;
        std::istringstream iss(linea);
        std::string prefijo; iss >> prefijo;

        if (prefijo == "o") {
            std::string nombre; iss >> nombre;
            objetoActivo = (nombre != "Sphere");
        } else if (prefijo == "v") {
            float x, y, z; iss >> x >> y >> z;
            tempPos.push_back({ x, y, z });
        } else if (prefijo == "f" && objetoActivo) {
            std::vector<int> cara; std::string tok;
            while (iss >> tok) cara.push_back(indicePosicion(tok));
            for (size_t i = 1; i + 1 < cara.size(); ++i) {
                triGlobal.push_back((unsigned int)cara[0]);
                triGlobal.push_back((unsigned int)cara[i]);
                triGlobal.push_back((unsigned int)cara[i + 1]);
            }
        }
    }
    if (triGlobal.empty()) { std::cerr << "ERROR: el OBJ no tiene caras\n"; return false; }

    // Compactar a vertices realmente usados
    std::unordered_map<unsigned int, unsigned int> remap;
    for (unsigned int gi : triGlobal) {
        auto it = remap.find(gi);
        unsigned int nuevo;
        if (it == remap.end()) { nuevo = (unsigned int)outPos.size(); remap[gi] = nuevo; outPos.push_back(tempPos[gi]); }
        else nuevo = it->second;
        outIdx.push_back(nuevo);
    }
    return true;
}

// ============================================================================
//  CARGADOR GLB / GLTF  (devuelve posiciones crudas + indices)
// ============================================================================
bool cargarGLB(const std::string& ruta,
               std::vector<glm::vec3>&    outPos,
               std::vector<unsigned int>& outIdx)
{
    tinygltf::TinyGLTF loader;
    tinygltf::Model    gltf;
    std::string err, warn;

    std::string ext = fs::path(ruta).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    bool ok = (ext == ".glb") ? loader.LoadBinaryFromFile(&gltf, &err, &warn, ruta)
                              : loader.LoadASCIIFromFile(&gltf, &err, &warn, ruta);
    if (!warn.empty()) std::cout << "  GLTF warn: " << warn << "\n";
    if (!ok) { std::cerr << "  ERROR GLB: " << err << "\n"; return false; }

    for (auto& mesh : gltf.meshes) {
        for (auto& prim : mesh.primitives) {
            auto posIt = prim.attributes.find("POSITION");
            if (posIt == prim.attributes.end()) continue;

            size_t baseVertex = outPos.size();
            auto& posAcc  = gltf.accessors[posIt->second];
            auto& posView = gltf.bufferViews[posAcc.bufferView];
            auto& posBuf  = gltf.buffers[posView.buffer];
            size_t stride = posView.byteStride == 0 ? 3 * sizeof(float) : posView.byteStride;
            const uint8_t* base = posBuf.data.data() + posView.byteOffset + posAcc.byteOffset;

            for (size_t i = 0; i < posAcc.count; i++) {
                const float* p = reinterpret_cast<const float*>(base + i * stride);
                outPos.push_back({ p[0], p[1], p[2] });
            }

            if (prim.indices >= 0) {
                auto& idxAcc  = gltf.accessors[prim.indices];
                auto& idxView = gltf.bufferViews[idxAcc.bufferView];
                auto& idxBuf  = gltf.buffers[idxView.buffer];
                const uint8_t* ib = idxBuf.data.data() + idxView.byteOffset + idxAcc.byteOffset;
                for (size_t i = 0; i < idxAcc.count; i++) {
                    unsigned int idx = 0;
                    switch (idxAcc.componentType) {
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:  idx = ib[i]; break;
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: idx = reinterpret_cast<const uint16_t*>(ib)[i]; break;
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:   idx = reinterpret_cast<const uint32_t*>(ib)[i]; break;
                        default: continue;
                    }
                    outIdx.push_back((unsigned int)(baseVertex + idx));
                }
            } else {
                for (size_t i = baseVertex; i < outPos.size(); i++)
                    outIdx.push_back((unsigned int)i);
            }
        }
    }
    if (outPos.empty()) { std::cerr << "  ERROR: GLB sin vertices\n"; return false; }
    return true;
}

// ============================================================================
//  REORIENTAR + NORMALIZAR + CONSTRUIR MAPA DE ALTURAS
//
//  - Reorienta automaticamente: el eje con menor rango pasa a ser la altura Y.
//    Esto corrige modelos "Z-up" (DEM) que aparecen volteados/de pie.
//  - Centra en el origen y escala a ANCHO_OBJETIVO.
//  - Llena outPos (floats xyz) y rellena el grid g_altura para colisiones.
// ============================================================================
void finalizarMalla(std::vector<glm::vec3>& raw,
                    std::vector<float>&     outPos)
{
    // --- 1) Detectar el eje vertical (el de menor extension) ---
    glm::vec3 bMin( 1e9f), bMax(-1e9f);
    for (auto& p : raw) { bMin = glm::min(bMin, p); bMax = glm::max(bMax, p); }
    glm::vec3 span = bMax - bMin;

    int up = 0;
    if (span.y <= span.x && span.y <= span.z) up = 1;
    else if (span.z <= span.x && span.z <= span.y) up = 2;
    else up = 0;
    int a = (up + 1) % 3; // primer eje horizontal
    int b = (up + 2) % 3; // segundo eje horizontal

    // Reorientar: nuevo (x = a, y = up, z = b)
    for (auto& p : raw) {
        glm::vec3 q(p[a], p[up], p[b]);
        p = q;
    }

    // --- 2) Normalizar (centrar + escalar) ---
    bMin = glm::vec3( 1e9f); bMax = glm::vec3(-1e9f);
    for (auto& p : raw) { bMin = glm::min(bMin, p); bMax = glm::max(bMax, p); }
    glm::vec3 centro = (bMin + bMax) * 0.5f;
    float spanMax = std::max(bMax.x - bMin.x, bMax.z - bMin.z);
    float escala  = (spanMax > 1e-6f) ? (ANCHO_OBJETIVO / spanMax) : 1.0f;

    outPos.clear();
    outPos.reserve(raw.size() * 3);
    for (auto& p : raw) {
        glm::vec3 n = (p - centro) * escala;
        n.y *= EXAGERACION_Y;
        outPos.push_back(n.x);
        outPos.push_back(n.y);
        outPos.push_back(n.z);
    }

    // --- 3) Construir mapa de alturas para colision/movimiento ---
    g_minX = (bMin.x - centro.x) * escala;  g_maxX = (bMax.x - centro.x) * escala;
    g_minZ = (bMin.z - centro.z) * escala;  g_maxZ = (bMax.z - centro.z) * escala;
    g_minY = (bMin.y - centro.y) * escala * EXAGERACION_Y;
    g_maxY = (bMax.y - centro.y) * escala * EXAGERACION_Y;
    if (g_maxX - g_minX < 1e-3f) g_maxX = g_minX + 1.0f;
    if (g_maxZ - g_minZ < 1e-3f) g_maxZ = g_minZ + 1.0f;

    g_gridN = 256;
    g_altura.assign((size_t)g_gridN * g_gridN, -1e9f);
    for (size_t i = 0; i + 2 < outPos.size(); i += 3) {
        float x = outPos[i], y = outPos[i + 1], z = outPos[i + 2];
        int gx = (int)((x - g_minX) / (g_maxX - g_minX) * (g_gridN - 1) + 0.5f);
        int gz = (int)((z - g_minZ) / (g_maxZ - g_minZ) * (g_gridN - 1) + 0.5f);
        gx = std::clamp(gx, 0, g_gridN - 1);
        gz = std::clamp(gz, 0, g_gridN - 1);
        float& cel = g_altura[(size_t)gz * g_gridN + gx];
        if (y > cel) cel = y; // guardamos la altura maxima de la celda
    }
    // Rellenar celdas vacias con la altura minima (evita "agujeros")
    for (auto& h : g_altura) if (h < -1e8f) h = g_minY;
}

// ============================================================================
//  DISPATCHER segun extension
// ============================================================================
bool cargarModelo(const std::string& ruta, std::vector<float>& outPos,
                  std::vector<unsigned int>& outIdx)
{
    std::string ext = fs::path(ruta).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    std::vector<glm::vec3> raw;
    bool ok = false;
    if (ext == ".obj") ok = cargarOBJ(ruta, raw, outIdx);
    else if (ext == ".glb" || ext == ".gltf") ok = cargarGLB(ruta, raw, outIdx);
    else { std::cerr << "  Formato no soportado: " << ext << "\n"; return false; }

    if (!ok) return false;
    finalizarMalla(raw, outPos);
    std::cout << "  Vertices: " << raw.size() << "  Triangulos: " << (outIdx.size() / 3) << "\n";
    return true;
}

// ============================================================================
//  DESCUBRIR MODELOS EN assets/
// ============================================================================
std::vector<std::string> descubrirModelos(const std::string& carpeta) {
    std::vector<std::string> lista;
    if (!fs::exists(carpeta)) { std::cerr << "WARN: carpeta '" << carpeta << "' no existe\n"; return lista; }
    for (auto& e : fs::directory_iterator(carpeta)) {
        if (!e.is_regular_file()) continue;
        std::string ext = e.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == ".obj" || ext == ".glb" || ext == ".gltf")
            lista.push_back(e.path().string());
    }
    std::sort(lista.begin(), lista.end());
    return lista;
}

// ============================================================================
//  SUBIR A GPU  (malla wireframe + nube de puntos diluida)
// ============================================================================
void subirAGPU(GLuint& vaoMalla, GLuint& vboMalla, GLuint& ebo,
               GLuint& vaoPuntos, GLuint& vboPuntos, int& numPuntos,
               const std::vector<float>& posiciones,
               const std::vector<unsigned int>& indices)
{
    if (vaoMalla)  { glDeleteVertexArrays(1, &vaoMalla);  glDeleteBuffers(1, &vboMalla);  glDeleteBuffers(1, &ebo); }
    if (vaoPuntos) { glDeleteVertexArrays(1, &vaoPuntos); glDeleteBuffers(1, &vboPuntos); }

    // ---- Malla (wireframe) ----
    glGenVertexArrays(1, &vaoMalla);
    glGenBuffers(1, &vboMalla);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vaoMalla);
    glBindBuffer(GL_ARRAY_BUFFER, vboMalla);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(posiciones.size() * sizeof(float)), posiciones.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(indices.size() * sizeof(unsigned int)), indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // ---- Nube de puntos DILUIDA (paso adaptativo) ----
    int numV = (int)(posiciones.size() / 3);
    int paso = std::max(1, numV / PUNTOS_MAX); // limita el total de puntos
    std::vector<float> puntos;
    puntos.reserve((numV / paso + 1) * 3);
    for (int v = 0; v < numV; v += paso) {
        puntos.push_back(posiciones[v * 3 + 0]);
        puntos.push_back(posiciones[v * 3 + 1]);
        puntos.push_back(posiciones[v * 3 + 2]);
    }
    numPuntos = (int)(puntos.size() / 3);

    glGenVertexArrays(1, &vaoPuntos);
    glGenBuffers(1, &vboPuntos);
    glBindVertexArray(vaoPuntos);
    glBindBuffer(GL_ARRAY_BUFFER, vboPuntos);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(puntos.size() * sizeof(float)), puntos.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
    std::cout << "  Puntos dibujados: " << numPuntos << " (paso " << paso << ")\n";
}

// ============================================================================
//  CALLBACKS GLFW
// ============================================================================
void framebuffer_size_callback(GLFWwindow*, int w, int h) { glViewport(0, 0, w, h); }

void mouse_button_callback(GLFWwindow*, int button, int action, int) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS)  { arrastrando = true; primerMouse = true; }
        if (action == GLFW_RELEASE)  arrastrando = false;
    }
}

void mouse_callback(GLFWwindow*, double xpos, double ypos) {
    if (!arrastrando) return;
    if (primerMouse) { lastX = xpos; lastY = ypos; primerMouse = false; }
    float xoff = (float)(xpos - lastX) * 0.3f;
    float yoff = (float)(lastY - ypos) * 0.3f;
    lastX = xpos; lastY = ypos;
    camYaw  += xoff;
    camElev += yoff;
    camElev = std::clamp(camElev, ELEV_MIN, ELEV_MAX);
}

void scroll_callback(GLFWwindow*, double, double yoffset) {
    camRadius -= (float)yoffset * 6.0f;
    camRadius = std::clamp(camRadius, RADIO_MIN, RADIO_MAX);
}

// Cambio de modelo (teclas discretas). El movimiento (flechas/WASD) se lee en el bucle.
void key_callback(GLFWwindow* window, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_ESCAPE) { glfwSetWindowShouldClose(window, true); return; }

    int n = (int)listaModelos.size();
    if (n == 0) return;

    if (key == GLFW_KEY_TAB)            { modeloActual = (modeloActual + 1) % n;       recargarModelo = true; }
    else if (key == GLFW_KEY_BACKSPACE) { modeloActual = (modeloActual - 1 + n) % n;   recargarModelo = true; }
    else if (key >= GLFW_KEY_1 && key <= GLFW_KEY_9) {
        int idx = key - GLFW_KEY_1;
        if (idx < n) { modeloActual = idx; recargarModelo = true; }
    }
}

void imprimirMenu() {
    std::cout << "\n========== MODELOS ==========\n";
    for (int i = 0; i < (int)listaModelos.size(); i++) {
        std::cout << "  [" << (i + 1) << "] " << fs::path(listaModelos[i]).filename().string();
        if (i == modeloActual) std::cout << "  <-- activo";
        std::cout << "\n";
    }
    std::cout << "=============================\n\n";
}

// ============================================================================
//  MAIN
// ============================================================================
int main() {
    listaModelos = descubrirModelos("assets");
    if (listaModelos.empty()) {
        std::cerr << "ERROR: no hay modelos (.obj/.glb/.gltf) en assets/\n";
        return -1;
    }

    std::cout << "Modelos encontrados:\n";
    for (int i = 0; i < (int)listaModelos.size(); i++)
        std::cout << "  [" << (i + 1) << "] " << fs::path(listaModelos[i]).filename().string() << "\n";
    std::cout << "\nControles:\n"
              << "  Flechas / WASD : moverse por el mapa (con colision)\n"
              << "  Arrastrar mouse: rotar vista\n"
              << "  Scroll         : zoom\n"
              << "  Tab / Backspace: siguiente / anterior modelo\n"
              << "  1..9           : modelo directo\n"
              << "  ESC            : salir\n\n";

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Simulador Topografico", nullptr, nullptr);
    if (!window) { std::cerr << "ERROR: ventana GLFW\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window,     mouse_button_callback);
    glfwSetCursorPosCallback(window,       mouse_callback);
    glfwSetScrollCallback(window,          scroll_callback);
    glfwSetKeyCallback(window,             key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { std::cerr << "ERROR: GLAD\n"; return -1; }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPointSize(1.6f); // puntos mas finos

    GLuint shaderProgram = crearProgramaShader("shaders/terrain.vert", "shaders/terrain.frag");

    GLuint vaoMalla = 0, vboMalla = 0, ebo = 0;
    GLuint vaoPuntos = 0, vboPuntos = 0;
    int numIndices = 0, numPuntos = 0;

    auto cargarActual = [&]() {
        const std::string& ruta = listaModelos[modeloActual];
        std::string nombre = fs::path(ruta).filename().string();
        std::cout << "\nCargando [" << (modeloActual + 1) << "/" << listaModelos.size() << "] " << nombre << " ...\n";

        std::vector<float>        posiciones;
        std::vector<unsigned int> indices;
        if (!cargarModelo(ruta, posiciones, indices)) {
            std::cerr << "  Fallo al cargar; se mantiene el modelo anterior.\n";
            return;
        }
        subirAGPU(vaoMalla, vboMalla, ebo, vaoPuntos, vboPuntos, numPuntos, posiciones, indices);
        numIndices = (int)indices.size();

        // Reubicar el foco en el centro, sobre la superficie
        camFoco = glm::vec3(0.0f, alturaTerreno(0.0f, 0.0f) + ALTURA_CAMINANTE, 0.0f);
        camRadius = 70.0f;

        std::string titulo = "Simulador Topografico  |  " + nombre + "  [" +
            std::to_string(modeloActual + 1) + "/" + std::to_string(listaModelos.size()) + "]";
        glfwSetWindowTitle(window, titulo.c_str());
        imprimirMenu();
    };

    cargarActual();

    GLint locModel = glGetUniformLocation(shaderProgram, "model");
    GLint locView  = glGetUniformLocation(shaderProgram, "view");
    GLint locProj  = glGetUniformLocation(shaderProgram, "projection");

    float tiempoPrev = (float)glfwGetTime();

    // ========================================================================
    //  BUCLE PRINCIPAL
    // ========================================================================
    while (!glfwWindowShouldClose(window)) {
        float ahora = (float)glfwGetTime();
        float dt = ahora - tiempoPrev;
        tiempoPrev = ahora;

        if (recargarModelo) { recargarModelo = false; cargarActual(); }

        // ---- Movimiento del foco por el mapa (flechas / WASD) ----
        float yawRad  = glm::radians(camYaw);
        float elevRad = glm::radians(camElev);

        // Direccion "hacia adelante" en el plano horizontal (de la camara al foco)
        glm::vec3 adelante = glm::normalize(glm::vec3(-sinf(yawRad), 0.0f, -cosf(yawRad)));
        glm::vec3 derecha  = glm::normalize(glm::vec3( cosf(yawRad), 0.0f, -sinf(yawRad)));

        glm::vec3 mov(0.0f);
        if (glfwGetKey(window, GLFW_KEY_UP)    == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) mov += adelante;
        if (glfwGetKey(window, GLFW_KEY_DOWN)  == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) mov -= adelante;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) mov += derecha;
        if (glfwGetKey(window, GLFW_KEY_LEFT)  == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) mov -= derecha;

        if (glm::length(mov) > 0.0001f) {
            mov = glm::normalize(mov) * VEL_MOVIMIENTO * dt;
            camFoco.x = std::clamp(camFoco.x + mov.x, g_minX, g_maxX);
            camFoco.z = std::clamp(camFoco.z + mov.z, g_minZ, g_maxZ);
        }
        // El foco "camina" sobre la superficie del terreno
        camFoco.y = alturaTerreno(camFoco.x, camFoco.z) + ALTURA_CAMINANTE;

        // ---- Posicion de la camara orbitando el foco ----
        glm::vec3 camPos = camFoco + glm::vec3(
            camRadius * cosf(elevRad) * sinf(yawRad),
            camRadius * sinf(elevRad),
            camRadius * cosf(elevRad) * cosf(yawRad)
        );
        // Colision: la camara nunca baja del terreno (no lo atraviesa)
        float hCam = alturaTerreno(camPos.x, camPos.z) + MARGEN_CAMARA;
        if (camPos.y < hCam) camPos.y = hCam;

        // ---- Render ----
        glClearColor(0.039f, 0.051f, 0.078f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 model      = glm::mat4(1.0f);
        glm::mat4 view       = glm::lookAt(camPos, camFoco, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f),
            (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1500.0f);

        glUseProgram(shaderProgram);
        glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(locView,  1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(locProj,  1, GL_FALSE, glm::value_ptr(projection));

        // Pasada 1: nube de puntos diluida
        glBindVertexArray(vaoPuntos);
        glDrawArrays(GL_POINTS, 0, numPuntos);

        // Pasada 2: malla wireframe
        glBindVertexArray(vaoMalla);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, 0);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &vaoMalla);  glDeleteBuffers(1, &vboMalla);  glDeleteBuffers(1, &ebo);
    glDeleteVertexArrays(1, &vaoPuntos); glDeleteBuffers(1, &vboPuntos);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}
