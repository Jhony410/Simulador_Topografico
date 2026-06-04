// ============================================================================
//  Simulador Topografico con Dron  -  FASE 1: Visor del terreno
//  OpenGL 3.3 Core Profile + C++17
//
//  Carga un .obj y lo dibuja como un terreno 3D estilo "nube de puntos +
//  wireframe" blanco azulado sobre fondo oscuro, inspirado en el simulador
//  de Orano Group. Camara orbital con elevacion limitada (20deg..60deg) y
//  zoom con limites.
// ============================================================================

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
#include <unordered_map>

// ---- Constantes de ventana ----
const unsigned int SCR_WIDTH  = 1280;
const unsigned int SCR_HEIGHT = 720;

// ---- Parametros de normalizacion del terreno ----
const float ANCHO_OBJETIVO = 100.0f; // ancho final deseado del terreno (X/Z)
const float EXAGERACION_Y  = 1.0f;   // factor extra para la altura (1.0 = uniforme)

// ============================================================================
//  CAMARA ORBITAL (variables globales usadas por los callbacks)
// ============================================================================
float camYaw    = 45.0f;   // rotacion horizontal libre (grados)
float camElev   = 35.0f;   // elevacion (grados), se limita a [20, 60]
float camRadius = 170.0f;  // distancia al centro del terreno (zoom)

// Limites de la camara (estilo Orano: no ver desde abajo ni cenital)
const float ELEV_MIN   = 20.0f;
const float ELEV_MAX   = 60.0f;
const float RADIO_MIN  = 70.0f;   // no acercarse tanto que salga del terreno
const float RADIO_MAX  = 300.0f;  // no alejarse tanto que el terreno desaparezca

// Estado del mouse para la rotacion con boton izquierdo arrastrando
bool   arrastrando = false;
bool   primerMouse = true;
double lastX = SCR_WIDTH  / 2.0;
double lastY = SCR_HEIGHT / 2.0;

// ============================================================================
//  SHADERS  -  convencion del curso: crearProgramaShader()
// ============================================================================
std::string cargarFuenteShader(const char* ruta) {
    std::ifstream archivo(ruta);
    if (!archivo.is_open()) {
        std::cerr << "ERROR: no se pudo abrir " << ruta << "\n";
        return "";
    }
    std::ostringstream ss;
    ss << archivo.rdbuf();
    return ss.str();
}

GLuint crearProgramaShader(const char* rutaVertex, const char* rutaFragment) {
    std::string vSrc = cargarFuenteShader(rutaVertex);
    std::string fSrc = cargarFuenteShader(rutaFragment);
    const char* vCode = vSrc.c_str();
    const char* fCode = fSrc.c_str();

    int  ok;
    char log[512];

    // -- Vertex shader --
    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &vCode, nullptr);
    glCompileShader(vert);
    glGetShaderiv(vert, GL_COMPILE_STATUS, &ok);
    if (!ok) { glGetShaderInfoLog(vert, 512, nullptr, log); std::cerr << "ERROR vertex:\n" << log << "\n"; }

    // -- Fragment shader --
    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &fCode, nullptr);
    glCompileShader(frag);
    glGetShaderiv(frag, GL_COMPILE_STATUS, &ok);
    if (!ok) { glGetShaderInfoLog(frag, 512, nullptr, log); std::cerr << "ERROR fragment:\n" << log << "\n"; }

    // -- Linkeo --
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) { glGetProgramInfoLog(prog, 512, nullptr, log); std::cerr << "ERROR link:\n" << log << "\n"; }

    glDeleteShader(vert);
    glDeleteShader(frag);
    return prog;
}

// ============================================================================
//  CARGADOR OBJ (parsing manual simple, sin Assimp)
//
//  - Lee posiciones "v" y caras "f" (formato v, v/vt, v//vn, v/vt/vn)
//  - Salta el objeto "Sphere" (cupula de cielo decorativa del .obj)
//  - Triangula caras con mas de 3 vertices mediante fan triangulation
//  - Normaliza: centra en el origen y escala a ANCHO_OBJETIVO unidades
//  - Devuelve posiciones compactas (solo vertices usados) + indices de
//    triangulos remapeados a esas posiciones
// ============================================================================

// Extrae solo el indice de posicion (parte antes del primer '/') de un token
int indicePosicion(const std::string& tok) {
    size_t s = tok.find('/');
    std::string num = (s == std::string::npos) ? tok : tok.substr(0, s);
    return std::stoi(num) - 1; // OBJ es 1-based
}

bool cargarTerrenoOBJ(const char* ruta,
                      std::vector<float>&        outPos,   // xyz compactos y normalizados
                      std::vector<unsigned int>& outIdx,   // indices de triangulos
                      float&                     outAncho) // ancho final del terreno
{
    std::ifstream archivo(ruta);
    if (!archivo.is_open()) {
        std::cerr << "ERROR: no se pudo abrir " << ruta << "\n";
        return false;
    }

    std::vector<glm::vec3>    tempPos;   // todas las posiciones del .obj (indices globales)
    std::vector<unsigned int> triGlobal; // indices de triangulos (globales, 0-based)

    bool objetoActivo = true; // si no hay linea "o", cargamos todo

    std::string linea;
    while (std::getline(archivo, linea)) {
        if (linea.empty() || linea[0] == '#') continue;
        std::istringstream iss(linea);
        std::string prefijo;
        iss >> prefijo;

        if (prefijo == "o") {
            std::string nombre;
            iss >> nombre;
            // El .obj incluye una "Sphere" (cupula del cielo): la omitimos
            objetoActivo = (nombre != "Sphere");
            std::cout << (objetoActivo ? "Cargando objeto: " : "Saltando objeto: ")
                      << nombre << "\n";

        } else if (prefijo == "v") {
            float x, y, z;
            iss >> x >> y >> z;
            // Acumulamos SIEMPRE para mantener correctos los indices globales
            tempPos.push_back({ x, y, z });

        } else if (prefijo == "f" && objetoActivo) {
            // Leemos todos los indices de la cara
            std::vector<int> cara;
            std::string tok;
            while (iss >> tok)
                cara.push_back(indicePosicion(tok));

            // Fan triangulation: (0,1,2), (0,2,3), ...
            for (size_t i = 1; i + 1 < cara.size(); ++i) {
                triGlobal.push_back((unsigned int)cara[0]);
                triGlobal.push_back((unsigned int)cara[i]);
                triGlobal.push_back((unsigned int)cara[i + 1]);
            }
        }
    }

    if (triGlobal.empty()) {
        std::cerr << "ERROR: el OBJ no contiene caras de terreno utilizables\n";
        return false;
    }

    // ---- Remapear a un arreglo compacto solo con los vertices usados ----
    std::unordered_map<unsigned int, unsigned int> remap;
    std::vector<glm::vec3> compact;
    compact.reserve(tempPos.size());

    for (unsigned int gi : triGlobal) {
        auto it = remap.find(gi);
        unsigned int nuevo;
        if (it == remap.end()) {
            nuevo = (unsigned int)compact.size();
            remap[gi] = nuevo;
            compact.push_back(tempPos[gi]);
        } else {
            nuevo = it->second;
        }
        outIdx.push_back(nuevo);
    }

    // ---- Normalizacion: bounding box, centrado y escalado ----
    glm::vec3 bMin( 1e9f), bMax(-1e9f);
    for (auto& p : compact) {
        bMin = glm::min(bMin, p);
        bMax = glm::max(bMax, p);
    }
    glm::vec3 centro = (bMin + bMax) * 0.5f;
    float spanX = bMax.x - bMin.x;
    float spanZ = bMax.z - bMin.z;
    float spanMax = std::max(spanX, spanZ);
    float escala  = (spanMax > 1e-6f) ? (ANCHO_OBJETIVO / spanMax) : 1.0f;

    outPos.reserve(compact.size() * 3);
    for (auto& p : compact) {
        glm::vec3 n = (p - centro) * escala;
        n.y *= EXAGERACION_Y; // exageracion opcional de la altura
        outPos.push_back(n.x);
        outPos.push_back(n.y);
        outPos.push_back(n.z);
    }

    outAncho = spanMax * escala;

    std::cout << "Vertices del terreno : " << compact.size() << "\n";
    std::cout << "Triangulos           : " << (outIdx.size() / 3) << "\n";
    std::cout << "Ancho normalizado    : " << outAncho << " unidades\n";
    std::cout << "Altura (Y) span      : " << (bMax.y - bMin.y) * escala * EXAGERACION_Y << " unidades\n";
    return true;
}

// ============================================================================
//  CALLBACKS GLFW
// ============================================================================
void framebuffer_size_callback(GLFWwindow*, int w, int h) {
    glViewport(0, 0, w, h);
}

// Activa/desactiva el arrastre con el boton izquierdo del mouse
void mouse_button_callback(GLFWwindow*, int button, int action, int) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            arrastrando = true;
            primerMouse = true; // evita el "salto" al iniciar el arrastre
        } else if (action == GLFW_RELEASE) {
            arrastrando = false;
        }
    }
}

// Rotacion orbital horizontal/vertical mientras se arrastra el boton izquierdo
void mouse_callback(GLFWwindow*, double xpos, double ypos) {
    if (!arrastrando) return;

    if (primerMouse) { lastX = xpos; lastY = ypos; primerMouse = false; }

    float xoff = (float)(xpos - lastX) * 0.3f;
    float yoff = (float)(lastY - ypos) * 0.3f; // invertido: arrastrar arriba sube la vista
    lastX = xpos; lastY = ypos;

    camYaw  += xoff;
    camElev += yoff;

    // Limites de elevacion (vista isometrica inclinada)
    if (camElev < ELEV_MIN) camElev = ELEV_MIN;
    if (camElev > ELEV_MAX) camElev = ELEV_MAX;
}

// Zoom con limites
void scroll_callback(GLFWwindow*, double, double yoffset) {
    camRadius -= (float)yoffset * 8.0f;
    if (camRadius < RADIO_MIN) camRadius = RADIO_MIN;
    if (camRadius > RADIO_MAX) camRadius = RADIO_MAX;
}

// ============================================================================
//  MAIN
// ============================================================================
int main() {
    // -- Inicializar GLFW (OpenGL 3.3 Core) --
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT,
        "Simulador Topografico - Visor de Terreno (Fase 1)", nullptr, nullptr);
    if (!window) {
        std::cerr << "ERROR: no se pudo crear la ventana GLFW\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window,     mouse_button_callback);
    glfwSetCursorPosCallback(window,       mouse_callback);
    glfwSetScrollCallback(window,          scroll_callback);

    // -- Inicializar GLAD --
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "ERROR: no se pudo inicializar GLAD\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    // Mezcla para el wireframe/puntos semi-transparentes
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Tamano de los puntos (GL_POINTS) ~2-3 px
    glPointSize(2.5f);

    // -- Shaders externos --
    GLuint shaderProgram = crearProgramaShader("shaders/terrain.vert", "shaders/terrain.frag");

    // -- Cargar y normalizar el terreno --
    std::vector<float>        posiciones;
    std::vector<unsigned int> indices;
    float anchoTerreno = ANCHO_OBJETIVO;
    if (!cargarTerrenoOBJ("assets/SnowTerrain.obj", posiciones, indices, anchoTerreno)) {
        std::cerr << "ERROR: fallo la carga del terreno\n";
        return -1;
    }
    int numVertices = (int)(posiciones.size() / 3);
    int numIndices  = (int)indices.size();

    // -- VAO / VBO / EBO --
    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
        (GLsizeiptr)(posiciones.size() * sizeof(float)),
        posiciones.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        (GLsizeiptr)(indices.size() * sizeof(unsigned int)),
        indices.data(), GL_STATIC_DRAW);

    // Posicion (location 0) -> 3 floats
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    // Localizaciones de uniforms (constantes durante la ejecucion)
    GLint locModel = glGetUniformLocation(shaderProgram, "model");
    GLint locView  = glGetUniformLocation(shaderProgram, "view");
    GLint locProj  = glGetUniformLocation(shaderProgram, "projection");

    std::cout << "\nControles:\n"
              << "  - Arrastrar boton izquierdo : rotar (orbital)\n"
              << "  - Scroll                    : zoom\n"
              << "  - ESC                       : salir\n\n";

    // ========================================================================
    //  BUCLE PRINCIPAL
    // ========================================================================
    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        // -- Fondo oscuro (#0a0d14) --
        glClearColor(0.039f, 0.051f, 0.078f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // -- Posicion de camara en coordenadas esfericas (orbita el origen) --
        float yawRad  = glm::radians(camYaw);
        float elevRad = glm::radians(camElev);
        glm::vec3 camPos(
            camRadius * cosf(elevRad) * sinf(yawRad),
            camRadius * sinf(elevRad),
            camRadius * cosf(elevRad) * cosf(yawRad)
        );
        glm::vec3 target(0.0f); // el terreno ya esta centrado en el origen

        // -- Matrices MVP --
        glm::mat4 model      = glm::mat4(1.0f);
        glm::mat4 view       = glm::lookAt(camPos, target, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f),
            (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);

        glUseProgram(shaderProgram);
        glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(locView,  1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(locProj,  1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAO);

        // -- Pasada 1: nube de puntos (GL_POINTS) --
        glDrawArrays(GL_POINTS, 0, numVertices);

        // -- Pasada 2: malla wireframe (triangulos en modo linea) --
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, 0);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // restaurar

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}
