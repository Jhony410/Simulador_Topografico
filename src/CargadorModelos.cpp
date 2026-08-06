// ============================================================================
//  Unica unidad de traduccion que instancia tinygltf y stb_image. Si estas
//  macros se definieran en dos .cpp el enlazador reportaria simbolos duplicados.
// ============================================================================
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"

#include "CargadorModelos.h"
#include "AristasCaracteristicas.h"
#include "Configuracion.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>

namespace fs = std::filesystem;

namespace {

// "12/3/4" -> indice de posicion 11 (los .obj indexan desde 1).
int indicePosicion(const std::string& token) {
    std::size_t barra = token.find('/');
    return std::stoi(barra == std::string::npos ? token : token.substr(0, barra)) - 1;
}

// Matriz local de un nodo glTF: o viene explicita, o se compone T * R * S.
glm::mat4 matrizNodo(const tinygltf::Node& n) {
    if (n.matrix.size() == 16) {
        float v[16];
        for (int i = 0; i < 16; i++) v[i] = (float)n.matrix[i];
        return glm::make_mat4(v);
    }
    glm::mat4 T(1.0f), R(1.0f), S(1.0f);
    if (n.translation.size() == 3)
        T = glm::translate(glm::mat4(1.0f), glm::vec3(n.translation[0], n.translation[1], n.translation[2]));
    if (n.rotation.size() == 4)
        R = glm::mat4_cast(glm::quat((float)n.rotation[3], (float)n.rotation[0],
                                     (float)n.rotation[1], (float)n.rotation[2]));
    if (n.scale.size() == 3)
        S = glm::scale(glm::mat4(1.0f), glm::vec3(n.scale[0], n.scale[1], n.scale[2]));
    return T * R * S;
}

// Acumula la matriz de mundo bajando por la jerarquia de nodos.
void calcularGlobales(const tinygltf::Model& m, int nodo, const glm::mat4& padre,
                      std::vector<glm::mat4>& globales, std::vector<glm::mat4>& locales) {
    globales[nodo] = padre * locales[nodo];
    for (int hijo : m.nodes[nodo].children)
        calcularGlobales(m, hijo, globales[nodo], globales, locales);
}

// Lee un indice del accessor de indices sea cual sea su tipo de componente.
bool leerIndice(const tinygltf::Accessor& ia, const uint8_t* base, std::size_t i, unsigned int& salida) {
    switch (ia.componentType) {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:  salida = base[i]; return true;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: salida = reinterpret_cast<const uint16_t*>(base)[i]; return true;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:   salida = reinterpret_cast<const uint32_t*>(base)[i]; return true;
        default: return false;
    }
}

} // namespace

namespace CargadorModelos {

// ---------------------------------------------------------------------------
bool cargarOBJ(const std::string& ruta,
               std::vector<glm::vec3>& posiciones,
               std::vector<unsigned int>& indices) {
    std::ifstream archivo(ruta);
    if (!archivo.is_open()) { std::cerr << "ERROR: no abre " << ruta << "\n"; return false; }

    std::vector<glm::vec3> temporales;
    std::vector<unsigned int> triangulos;
    bool objetoActivo = true;
    std::string linea;

    while (std::getline(archivo, linea)) {
        if (linea.empty() || linea[0] == '#') continue;
        std::istringstream flujo(linea);
        std::string prefijo;
        flujo >> prefijo;

        if (prefijo == "o") {
            std::string nombre; flujo >> nombre;
            objetoActivo = (nombre != "Sphere");   // la esfera del .obj no es terreno
        } else if (prefijo == "v") {
            float x, y, z; flujo >> x >> y >> z;
            temporales.push_back({x, y, z});
        } else if (prefijo == "f" && objetoActivo) {
            std::vector<int> esquinas; std::string token;
            while (flujo >> token) esquinas.push_back(indicePosicion(token));
            // Abanico de triangulos: soporta caras de N lados.
            for (std::size_t i = 1; i + 1 < esquinas.size(); ++i) {
                triangulos.push_back(esquinas[0]);
                triangulos.push_back(esquinas[i]);
                triangulos.push_back(esquinas[i + 1]);
            }
        }
    }
    if (triangulos.empty()) { std::cerr << "ERROR: OBJ sin caras\n"; return false; }

    // Remapeo: solo se conservan los vertices realmente referenciados.
    std::unordered_map<unsigned int, unsigned int> remapeo;
    for (unsigned int global : triangulos) {
        if (global >= temporales.size()) {
            std::cerr << "[ERROR] OBJ invalido: indice de cara fuera del rango en " << ruta << "\n";
            posiciones.clear();
            indices.clear();
            return false;
        }
        auto it = remapeo.find(global);
        unsigned int nuevo;
        if (it == remapeo.end()) {
            nuevo = (unsigned int)posiciones.size();
            remapeo[global] = nuevo;
            posiciones.push_back(temporales[global]);
        } else {
            nuevo = it->second;
        }
        indices.push_back(nuevo);
    }
    return true;
}

// ---------------------------------------------------------------------------
bool cargarGLBTerreno(const std::string& ruta,
                      std::vector<glm::vec3>& posiciones,
                      std::vector<unsigned int>& indices) {
    tinygltf::TinyGLTF lector;
    tinygltf::Model modelo;
    std::string error, aviso;

    std::string ext = fs::path(ruta).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    bool ok = (ext == ".glb") ? lector.LoadBinaryFromFile(&modelo, &error, &aviso, ruta)
                              : lector.LoadASCIIFromFile(&modelo, &error, &aviso, ruta);
    if (!ok) { std::cerr << "  ERROR GLB: " << error << "\n"; return false; }

    for (auto& malla : modelo.meshes) {
        for (auto& primitiva : malla.primitives) {
            auto it = primitiva.attributes.find("POSITION");
            if (it == primitiva.attributes.end()) continue;

            std::size_t base = posiciones.size();
            const tinygltf::Accessor&   ac = modelo.accessors[it->second];
            const tinygltf::BufferView& bv = modelo.bufferViews[ac.bufferView];
            const tinygltf::Buffer&     bf = modelo.buffers[bv.buffer];
            std::size_t paso = ac.ByteStride(bv);
            const uint8_t* p = bf.data.data() + bv.byteOffset + ac.byteOffset;
            for (std::size_t i = 0; i < ac.count; i++) {
                const float* f = reinterpret_cast<const float*>(p + i * paso);
                posiciones.push_back({f[0], f[1], f[2]});
            }

            if (primitiva.indices >= 0) {
                const tinygltf::Accessor&   ia  = modelo.accessors[primitiva.indices];
                const tinygltf::BufferView& ibv = modelo.bufferViews[ia.bufferView];
                const tinygltf::Buffer&     ibf = modelo.buffers[ibv.buffer];
                const uint8_t* ip = ibf.data.data() + ibv.byteOffset + ia.byteOffset;
                for (std::size_t i = 0; i < ia.count; i++) {
                    unsigned int id = 0;
                    if (!leerIndice(ia, ip, i, id)) continue;
                    indices.push_back((unsigned int)(base + id));
                }
            } else {
                for (std::size_t i = base; i < posiciones.size(); i++)
                    indices.push_back((unsigned int)i);
            }
        }
    }
    return !posiciones.empty();
}

// ---------------------------------------------------------------------------
bool cargarCSVCalles(const std::string& ruta,
                     std::vector<glm::vec3>& posiciones,
                     std::vector<unsigned int>& indices) {
    std::ifstream archivo(ruta);
    if (!archivo.is_open()) { std::cerr << "ERROR: no abre " << ruta << "\n"; return false; }

    const double PI = 3.14159265358979323846;
    std::string linea;
    std::getline(archivo, linea);   // cabecera

    double refLon = 0, refLat = 0;
    bool hayReferencia = false;

    while (std::getline(archivo, linea)) {
        std::size_t p = linea.find("LINESTRING");
        if (p == std::string::npos) continue;
        std::size_t a = linea.find('(', p), b = linea.find(')', a);
        if (a == std::string::npos || b == std::string::npos) continue;

        std::istringstream flujo(linea.substr(a + 1, b - a - 1));
        std::string token;
        bool primero = true;
        unsigned int anterior = 0;

        while (std::getline(flujo, token, ',')) {
            std::istringstream par(token);
            double lon, lat;
            if (!(par >> lon >> lat)) continue;
            if (!hayReferencia) { refLon = lon; refLat = lat; hayReferencia = true; }

            // Un grado de longitud se acorta por cos(latitud): sin esto el mapa
            // sale estirado en horizontal.
            float x = (float)((lon - refLon) * std::cos(refLat * PI / 180.0) * 1000.0);
            float z = (float)((lat - refLat) * 1000.0);

            unsigned int actual = (unsigned int)posiciones.size();
            posiciones.push_back(glm::vec3(x, 0.0f, z));
            if (!primero) { indices.push_back(anterior); indices.push_back(actual); }
            anterior = actual;
            primero = false;
        }
    }
    if (posiciones.empty()) { std::cerr << "ERROR: CSV sin geometria LINESTRING\n"; return false; }
    std::cout << "  Calles: vertices " << posiciones.size()
              << "  segmentos " << (indices.size() / 2) << "\n";
    return true;
}

// ---------------------------------------------------------------------------
bool cargarDronAnimado(const std::string& ruta,
                       MallaCruda& malla,
                       glm::vec3 pivotesHelices[4],
                       float tamanioObjetivo) {
    tinygltf::TinyGLTF lector;
    tinygltf::Model m;
    std::string error, aviso;
    if (!lector.LoadBinaryFromFile(&m, &error, &aviso, ruta)) {
        std::cerr << "ERROR dron: " << error << "\n"; return false;
    }
    if (m.skins.empty()) { std::cerr << "ERROR: el dron no tiene skin\n"; return false; }

    malla.limpiar();
    malla.floatsPorVertice = 7;   // [x, y, z, nx, ny, nz, idHelice]

    // 1) Matrices locales y globales de todos los nodos.
    int N = (int)m.nodes.size();
    std::vector<glm::mat4> locales(N), globales(N, glm::mat4(1.0f));
    for (int i = 0; i < N; i++) locales[i] = matrizNodo(m.nodes[i]);
    int escena = m.defaultScene >= 0 ? m.defaultScene : 0;
    for (int raiz : m.scenes[escena].nodes) calcularGlobales(m, raiz, glm::mat4(1.0f), globales, locales);

    // 2) Skin: joints + inverseBindMatrices.
    const tinygltf::Skin& skin = m.skins[0];
    int numJoints = (int)skin.joints.size();
    std::vector<glm::mat4> bindInverso(numJoints, glm::mat4(1.0f));
    if (skin.inverseBindMatrices >= 0) {
        const tinygltf::Accessor&   ac = m.accessors[skin.inverseBindMatrices];
        const tinygltf::BufferView& bv = m.bufferViews[ac.bufferView];
        const tinygltf::Buffer&     bf = m.buffers[bv.buffer];
        const uint8_t* p = bf.data.data() + bv.byteOffset + ac.byteOffset;
        for (int j = 0; j < numJoints; j++)
            bindInverso[j] = glm::make_mat4(reinterpret_cast<const float*>(p + (std::size_t)j * 64));
    }
    // Lleva vertices del espacio de bind al mundo, para hornear la pose de reposo.
    std::vector<glm::mat4> matrizSkin(numJoints);
    for (int j = 0; j < numJoints; j++)
        matrizSkin[j] = globales[skin.joints[j]] * bindInverso[j];

    // Joints de las 4 helices (prop_1..4_jnt).
    int jointHelice[4] = {-1, -1, -1, -1};
    for (int j = 0; j < numJoints; j++) {
        const std::string& nombre = m.nodes[skin.joints[j]].name;
        for (int k = 0; k < 4; k++) {
            std::string clave = "prop_" + std::to_string(k + 1) + "_jnt";
            if (nombre.find(clave) != std::string::npos) jointHelice[k] = j;
        }
    }
    for (int k = 0; k < 4; k++)
        pivotesHelices[k] = (jointHelice[k] >= 0)
                          ? glm::vec3(globales[skin.joints[jointHelice[k]]][3])
                          : glm::vec3(0.0f);

    // 3) Hornear cada vertice de las mallas skinned.
    std::vector<glm::vec3> posiciones;
    std::vector<float> idsHelice;

    for (auto& nodo : m.nodes) {
        if (nodo.mesh < 0) continue;
        const tinygltf::Mesh& mesh = m.meshes[nodo.mesh];
        for (auto& pr : mesh.primitives) {
            auto itP = pr.attributes.find("POSITION");
            auto itJ = pr.attributes.find("JOINTS_0");
            auto itW = pr.attributes.find("WEIGHTS_0");
            if (itP == pr.attributes.end() || itJ == pr.attributes.end() || itW == pr.attributes.end()) continue;

            const tinygltf::Accessor& aP = m.accessors[itP->second];
            const tinygltf::Accessor& aJ = m.accessors[itJ->second];
            const tinygltf::Accessor& aW = m.accessors[itW->second];
            const tinygltf::BufferView& vP = m.bufferViews[aP.bufferView];
            const tinygltf::BufferView& vJ = m.bufferViews[aJ.bufferView];
            const tinygltf::BufferView& vW = m.bufferViews[aW.bufferView];
            const uint8_t* pP = m.buffers[vP.buffer].data.data() + vP.byteOffset + aP.byteOffset;
            const uint8_t* pJ = m.buffers[vJ.buffer].data.data() + vJ.byteOffset + aJ.byteOffset;
            const uint8_t* pW = m.buffers[vW.buffer].data.data() + vW.byteOffset + aW.byteOffset;
            std::size_t sP = aP.ByteStride(vP), sJ = aJ.ByteStride(vJ), sW = aW.ByteStride(vW);
            std::size_t base = posiciones.size();

            for (std::size_t i = 0; i < aP.count; i++) {
                const float* fp = reinterpret_cast<const float*>(pP + i * sP);
                glm::vec4 vertice(fp[0], fp[1], fp[2], 1.0f);

                unsigned int joints[4];
                const uint8_t* jp = pJ + i * sJ;
                if (aJ.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
                    for (int k = 0; k < 4; k++) joints[k] = jp[k];
                else
                    for (int k = 0; k < 4; k++) joints[k] = reinterpret_cast<const uint16_t*>(jp)[k];

                const float* pesos = reinterpret_cast<const float*>(pW + i * sW);
                glm::mat4 combinada(0.0f);
                float sumaPesos = 0.0f;
                for (int k = 0; k < 4; k++) {
                    float w = pesos[k];
                    if (w <= 0) continue;
                    combinada += w * matrizSkin[joints[k]];
                    sumaPesos += w;
                }
                if (sumaPesos < 1e-6f) combinada = glm::mat4(1.0f);
                posiciones.push_back(glm::vec3(combinada * vertice));

                // idHelice = helice del joint con mayor peso (0 = cuerpo).
                int kMax = 0; float wMax = pesos[0];
                for (int k = 1; k < 4; k++) if (pesos[k] > wMax) { wMax = pesos[k]; kMax = k; }
                int dominante = (int)joints[kMax];
                float id = 0.0f;
                for (int k = 0; k < 4; k++) if (dominante == jointHelice[k]) id = (float)(k + 1);
                idsHelice.push_back(id);
            }

            if (pr.indices >= 0) {
                const tinygltf::Accessor&   ia  = m.accessors[pr.indices];
                const tinygltf::BufferView& ibv = m.bufferViews[ia.bufferView];
                const uint8_t* ip = m.buffers[ibv.buffer].data.data() + ibv.byteOffset + ia.byteOffset;
                for (std::size_t i = 0; i < ia.count; i++) {
                    unsigned int id = 0;
                    if (!leerIndice(ia, ip, i, id)) continue;
                    malla.indices.push_back((unsigned int)(base + id));
                }
            } else {
                for (std::size_t i = base; i < posiciones.size(); i++)
                    malla.indices.push_back((unsigned int)i);
            }
        }
    }
    if (posiciones.empty()) { std::cerr << "ERROR: dron sin vertices\n"; return false; }

    // 4) Centrar en el origen y escalar al tamaño de juego (pivotes incluidos).
    glm::vec3 bMin(1e9f), bMax(-1e9f);
    for (auto& p : posiciones) { bMin = glm::min(bMin, p); bMax = glm::max(bMax, p); }
    glm::vec3 centro = (bMin + bMax) * 0.5f;
    glm::vec3 extension = bMax - bMin;
    float extensionMax = std::max(extension.x, std::max(extension.y, extension.z));
    float escala = (extensionMax > 1e-6f) ? (tamanioObjetivo / extensionMax) : 1.0f;

    std::vector<glm::vec3> normales(posiciones.size(), glm::vec3(0.0f));
    for (std::size_t i = 0; i + 2 < malla.indices.size(); i += 3) {
        unsigned int ia = malla.indices[i], ib = malla.indices[i + 1], ic = malla.indices[i + 2];
        if (ia >= posiciones.size() || ib >= posiciones.size() || ic >= posiciones.size()) continue;
        glm::vec3 n = glm::cross(posiciones[ib] - posiciones[ia], posiciones[ic] - posiciones[ia]);
        if (glm::length(n) > 1e-8f) { normales[ia] += n; normales[ib] += n; normales[ic] += n; }
    }
    for (auto& n : normales) n = glm::length(n) > 1e-8f ? glm::normalize(n) : glm::vec3(0, 1, 0);

    malla.vertices.reserve(posiciones.size() * 7);
    for (std::size_t i = 0; i < posiciones.size(); i++) {
        glm::vec3 n = (posiciones[i] - centro) * escala;
        malla.vertices.push_back(n.x);
        malla.vertices.push_back(n.y);
        malla.vertices.push_back(n.z);
        malla.vertices.push_back(normales[i].x);
        malla.vertices.push_back(normales[i].y);
        malla.vertices.push_back(normales[i].z);
        malla.vertices.push_back(idsHelice[i]);
    }
    for (int k = 0; k < 4; k++) pivotesHelices[k] = (pivotesHelices[k] - centro) * escala;

    // El armazon se dibuja con estas aristas, no con los triangulos en modo
    // alambre: ver AristasCaracteristicas.h.
    malla.aristas = AristasCaracteristicas::extraer(
        malla.vertices, malla.floatsPorVertice, malla.indices,
        Configuracion::ANGULO_ARISTA_DRON, tamanioObjetivo * 1e-4f);

    std::cout << "Dron horneado: " << posiciones.size() << " vertices, "
              << (malla.indices.size() / 3) << " triangulos, "
              << (malla.aristas.size() / 2) << " aristas caracteristicas\n";
    return true;
}

// ---------------------------------------------------------------------------
std::vector<std::string> descubrirTerrenos(const std::string& carpeta) {
    std::vector<std::string> lista;
    if (!fs::exists(carpeta)) { std::cerr << "WARN: no existe " << carpeta << "\n"; return lista; }

    for (auto& entrada : fs::directory_iterator(carpeta)) {
        if (!entrada.is_regular_file()) continue;
        std::string ext    = entrada.path().extension().string();
        std::string nombre = entrada.path().filename().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        std::transform(nombre.begin(), nombre.end(), nombre.begin(), ::tolower);
        if (nombre.find("drone") != std::string::npos || nombre.find("dron") != std::string::npos) continue;
        if (ext == ".obj" || ext == ".glb" || ext == ".gltf" || ext == ".csv")
            lista.push_back(entrada.path().string());
    }
    std::sort(lista.begin(), lista.end());
    return lista;
}

} // namespace CargadorModelos
