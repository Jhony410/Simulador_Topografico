// ============================================================================
//  Simulador Topografico con Dron  -  FASE 1
//  OpenGL 3.3 Core Profile + C++17
//
//  - Terrenos .obj / .glb / .gltf (wireframe + nube de puntos diluida)
//  - Dron .glb animado (animated_drone.glb): se hornea la pose de reposo con
//    skinning en CPU (queda derecho) y las 4 helices giran en el shader.
//  - HUD 2D: controles de movimiento en pantalla + 4 botones de mapa clicables.
//
//  CONTROLES
//    Flechas / WASD : mover el dron
//    Espacio / Shift: subir / bajar
//    Arrastrar mouse: rotar vista       Scroll: zoom
//    Botones 1..4 (clic) o teclas 1..4 : cambiar de mapa
//    ESC            : salir
// ============================================================================

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"
#include "stb_easy_font.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

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

// ---- Ventana ----
unsigned int SCR_WIDTH  = 1280;
unsigned int SCR_HEIGHT = 720;

// ---- Terreno ----
const float ANCHO_OBJETIVO = 100.0f;
const float EXAGERACION_Y  = 1.0f;
const int   PUNTOS_MAX      = 35000;

// ---- Dron ----
const std::string RUTA_DRON = "assets/animated_drone.glb";
const float TAMANIO_DRON   = 8.0f;
const float VEL_DRON       = 50.0f;
const float VEL_ALTURA     = 35.0f;
const float ALTURA_INICIAL = 18.0f;
const float GIRO_HELICES    = 18.0f;   // rad/seg
float DRON_OFFSET_YAW = 0.0f;          // ajuste de orientacion si mira mal

// ---- Camara ----
float camYaw    = 45.0f;
float camElev   = 30.0f;
float camRadius = 55.0f;
const float ELEV_MIN  = 8.0f,  ELEV_MAX  = 80.0f;
const float RADIO_MIN = 22.0f, RADIO_MAX = 260.0f;

bool   arrastrando = false, primerMouse = true;
double lastX = 640, lastY = 360;

// ---- Dron: estado ----
glm::vec3 dronPos(0.0f);
float     dronYaw = 0.0f;
glm::vec3 dronPivots[4];

// ---- Terrenos ----
std::vector<std::string> listaModelos;
int  modeloActual = 0;
bool recargarModelo = false;
bool g_esLineas = false;   // true cuando el "terreno" es un mapa de calles (CSV)

// ---- Mapa de alturas (solo para colocar el dron) ----
std::vector<float> g_altura;
int g_gridN = 0;
float g_minX=-50,g_maxX=50,g_minZ=-50,g_maxZ=50,g_minY=0,g_maxY=0;

float alturaTerreno(float x, float z) {
    if (g_gridN <= 0) return 0.0f;
    float fx = std::clamp((x-g_minX)/(g_maxX-g_minX)*(g_gridN-1),0.0f,(float)(g_gridN-1));
    float fz = std::clamp((z-g_minZ)/(g_maxZ-g_minZ)*(g_gridN-1),0.0f,(float)(g_gridN-1));
    int x0=(int)fx,z0=(int)fz,x1=std::min(x0+1,g_gridN-1),z1=std::min(z0+1,g_gridN-1);
    float tx=fx-x0,tz=fz-z0;
    float a=g_altura[z0*g_gridN+x0]*(1-tx)+g_altura[z0*g_gridN+x1]*tx;
    float b=g_altura[z1*g_gridN+x0]*(1-tx)+g_altura[z1*g_gridN+x1]*tx;
    return a*(1-tz)+b*tz;
}

// ============================================================================
//  SHADERS
// ============================================================================
std::string leerArchivo(const char* ruta) {
    std::ifstream f(ruta);
    if (!f.is_open()) { std::cerr << "ERROR: no abre " << ruta << "\n"; return ""; }
    std::ostringstream ss; ss << f.rdbuf(); return ss.str();
}
GLuint crearShader(const char* rv, const char* rf) {
    std::string vs=leerArchivo(rv), fsr=leerArchivo(rf);
    const char* vc=vs.c_str(); const char* fc=fsr.c_str();
    int ok; char log[512];
    GLuint v=glCreateShader(GL_VERTEX_SHADER); glShaderSource(v,1,&vc,nullptr); glCompileShader(v);
    glGetShaderiv(v,GL_COMPILE_STATUS,&ok); if(!ok){glGetShaderInfoLog(v,512,nullptr,log);std::cerr<<"VERT "<<rv<<":\n"<<log<<"\n";}
    GLuint f=glCreateShader(GL_FRAGMENT_SHADER); glShaderSource(f,1,&fc,nullptr); glCompileShader(f);
    glGetShaderiv(f,GL_COMPILE_STATUS,&ok); if(!ok){glGetShaderInfoLog(f,512,nullptr,log);std::cerr<<"FRAG "<<rf<<":\n"<<log<<"\n";}
    GLuint p=glCreateProgram(); glAttachShader(p,v); glAttachShader(p,f); glLinkProgram(p);
    glGetProgramiv(p,GL_LINK_STATUS,&ok); if(!ok){glGetProgramInfoLog(p,512,nullptr,log);std::cerr<<"LINK:\n"<<log<<"\n";}
    glDeleteShader(v); glDeleteShader(f); return p;
}

// ============================================================================
//  CARGADOR OBJ (terreno)
// ============================================================================
int idxPos(const std::string& t){ size_t s=t.find('/'); return std::stoi(s==std::string::npos?t:t.substr(0,s))-1; }
bool cargarOBJ(const std::string& ruta, std::vector<glm::vec3>& outPos, std::vector<unsigned int>& outIdx) {
    std::ifstream a(ruta); if(!a.is_open()){std::cerr<<"ERROR: no abre "<<ruta<<"\n";return false;}
    std::vector<glm::vec3> tmp; std::vector<unsigned int> tri; bool act=true; std::string ln;
    while(std::getline(a,ln)){
        if(ln.empty()||ln[0]=='#')continue;
        std::istringstream is(ln); std::string pf; is>>pf;
        if(pf=="o"){std::string n;is>>n;act=(n!="Sphere");}
        else if(pf=="v"){float x,y,z;is>>x>>y>>z;tmp.push_back({x,y,z});}
        else if(pf=="f"&&act){std::vector<int> c;std::string t;while(is>>t)c.push_back(idxPos(t));
            for(size_t i=1;i+1<c.size();++i){tri.push_back(c[0]);tri.push_back(c[i]);tri.push_back(c[i+1]);}}
    }
    if(tri.empty()){std::cerr<<"ERROR: OBJ sin caras\n";return false;}
    std::unordered_map<unsigned int,unsigned int> rm;
    for(unsigned int gi:tri){auto it=rm.find(gi);unsigned int nv;
        if(it==rm.end()){nv=(unsigned int)outPos.size();rm[gi]=nv;outPos.push_back(tmp[gi]);}else nv=it->second;
        outIdx.push_back(nv);}
    return true;
}

// ============================================================================
//  CARGADOR GLB (terreno: solo POSITION + indices)
// ============================================================================
bool cargarGLBterreno(const std::string& ruta, std::vector<glm::vec3>& outPos, std::vector<unsigned int>& outIdx) {
    tinygltf::TinyGLTF loader; tinygltf::Model m; std::string e,w;
    std::string ext=fs::path(ruta).extension().string();
    std::transform(ext.begin(),ext.end(),ext.begin(),::tolower);
    bool ok=(ext==".glb")?loader.LoadBinaryFromFile(&m,&e,&w,ruta):loader.LoadASCIIFromFile(&m,&e,&w,ruta);
    if(!ok){std::cerr<<"  ERROR GLB: "<<e<<"\n";return false;}
    for(auto& mesh:m.meshes) for(auto& pr:mesh.primitives){
        auto it=pr.attributes.find("POSITION"); if(it==pr.attributes.end())continue;
        size_t base=outPos.size();
        const tinygltf::Accessor& ac=m.accessors[it->second];
        const tinygltf::BufferView& bv=m.bufferViews[ac.bufferView];
        const tinygltf::Buffer& bf=m.buffers[bv.buffer];
        size_t st=ac.ByteStride(bv); const uint8_t* p=bf.data.data()+bv.byteOffset+ac.byteOffset;
        for(size_t i=0;i<ac.count;i++){const float* f=reinterpret_cast<const float*>(p+i*st);outPos.push_back({f[0],f[1],f[2]});}
        if(pr.indices>=0){
            const tinygltf::Accessor& ia=m.accessors[pr.indices];
            const tinygltf::BufferView& ibv=m.bufferViews[ia.bufferView];
            const tinygltf::Buffer& ibf=m.buffers[ibv.buffer];
            const uint8_t* ip=ibf.data.data()+ibv.byteOffset+ia.byteOffset;
            for(size_t i=0;i<ia.count;i++){unsigned int id=0;
                switch(ia.componentType){
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: id=ip[i];break;
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: id=reinterpret_cast<const uint16_t*>(ip)[i];break;
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: id=reinterpret_cast<const uint32_t*>(ip)[i];break;
                    default:continue;}
                outIdx.push_back((unsigned int)(base+id));}
        } else for(size_t i=base;i<outPos.size();i++) outIdx.push_back((unsigned int)i);
    }
    return !outPos.empty();
}

// ============================================================================
//  CARGADOR CSV: red de calles (OpenStreetMap) con geometria LINESTRING
//  Cada fila trae ... "LINESTRING (lon lat, lon lat, ...)". Se dibuja como
//  segmentos de linea (GL_LINES) en un plano (Y = 0).
// ============================================================================
bool cargarCSV(const std::string& ruta, std::vector<glm::vec3>& raw, std::vector<unsigned int>& idx) {
    std::ifstream f(ruta);
    if(!f.is_open()){std::cerr<<"ERROR: no abre "<<ruta<<"\n";return false;}
    const double PI=3.14159265358979323846;
    std::string line; std::getline(f,line); // cabecera
    double refLon=0,refLat=0; bool haveRef=false;
    while(std::getline(f,line)){
        size_t p=line.find("LINESTRING"); if(p==std::string::npos)continue;
        size_t a=line.find('(',p), b=line.find(')',a);
        if(a==std::string::npos||b==std::string::npos)continue;
        std::string inside=line.substr(a+1,b-a-1);
        std::istringstream ss(inside); std::string tok;
        bool first=true; unsigned int prev=0;
        while(std::getline(ss,tok,',')){
            std::istringstream ps(tok); double lon,lat;
            if(!(ps>>lon>>lat))continue;
            if(!haveRef){refLon=lon;refLat=lat;haveRef=true;}
            // Correccion de aspecto: 1 grado de longitud se acorta por cos(latitud)
            float x=(float)((lon-refLon)*std::cos(refLat*PI/180.0)*1000.0);
            float z=(float)((lat-refLat)*1000.0);
            unsigned int cur=(unsigned int)raw.size();
            raw.push_back(glm::vec3(x,0.0f,z));
            if(!first){idx.push_back(prev);idx.push_back(cur);}
            prev=cur; first=false;
        }
    }
    if(raw.empty()){std::cerr<<"ERROR: CSV sin geometria LINESTRING\n";return false;}
    std::cout<<"  Calles: vertices "<<raw.size()<<"  segmentos "<<(idx.size()/2)<<"\n";
    return true;
}

// ============================================================================
//  TERRENO: reorientar (auto Y-up) + normalizar + mapa de alturas
// ============================================================================
void finalizarTerreno(std::vector<glm::vec3>& raw, std::vector<float>& outPos) {
    glm::vec3 bMin(1e9f),bMax(-1e9f);
    for(auto&p:raw){bMin=glm::min(bMin,p);bMax=glm::max(bMax,p);}
    glm::vec3 span=bMax-bMin; int up=0;
    if(span.y<=span.x&&span.y<=span.z)up=1; else if(span.z<=span.x&&span.z<=span.y)up=2;
    int a=(up+1)%3,b=(up+2)%3;
    for(auto&p:raw){glm::vec3 q(p[a],p[up],p[b]);p=q;}
    bMin=glm::vec3(1e9f);bMax=glm::vec3(-1e9f);
    for(auto&p:raw){bMin=glm::min(bMin,p);bMax=glm::max(bMax,p);}
    glm::vec3 c=(bMin+bMax)*0.5f;
    float spanMax=std::max(bMax.x-bMin.x,bMax.z-bMin.z);
    float esc=(spanMax>1e-6f)?(ANCHO_OBJETIVO/spanMax):1.0f;
    outPos.clear();outPos.reserve(raw.size()*3);
    for(auto&p:raw){glm::vec3 n=(p-c)*esc;n.y*=EXAGERACION_Y;outPos.push_back(n.x);outPos.push_back(n.y);outPos.push_back(n.z);}
    g_minX=(bMin.x-c.x)*esc;g_maxX=(bMax.x-c.x)*esc;g_minZ=(bMin.z-c.z)*esc;g_maxZ=(bMax.z-c.z)*esc;
    g_minY=(bMin.y-c.y)*esc*EXAGERACION_Y;g_maxY=(bMax.y-c.y)*esc*EXAGERACION_Y;
    if(g_maxX-g_minX<1e-3f)g_maxX=g_minX+1; if(g_maxZ-g_minZ<1e-3f)g_maxZ=g_minZ+1;
    g_gridN=256; g_altura.assign((size_t)g_gridN*g_gridN,-1e9f);
    for(size_t i=0;i+2<outPos.size();i+=3){
        float x=outPos[i],y=outPos[i+1],z=outPos[i+2];
        int gx=std::clamp((int)((x-g_minX)/(g_maxX-g_minX)*(g_gridN-1)+0.5f),0,g_gridN-1);
        int gz=std::clamp((int)((z-g_minZ)/(g_maxZ-g_minZ)*(g_gridN-1)+0.5f),0,g_gridN-1);
        float& cel=g_altura[(size_t)gz*g_gridN+gx]; if(y>cel)cel=y;
    }
    for(auto&h:g_altura)if(h<-1e8f)h=g_minY;
}
bool cargarTerreno(const std::string& ruta, std::vector<float>& outPos, std::vector<unsigned int>& outIdx) {
    std::string ext=fs::path(ruta).extension().string();
    std::transform(ext.begin(),ext.end(),ext.begin(),::tolower);
    std::vector<glm::vec3> raw; bool ok=false; g_esLineas=false;
    if(ext==".obj")ok=cargarOBJ(ruta,raw,outIdx);
    else if(ext==".glb"||ext==".gltf")ok=cargarGLBterreno(ruta,raw,outIdx);
    else if(ext==".csv"){ok=cargarCSV(ruta,raw,outIdx);g_esLineas=true;}
    else {std::cerr<<"  Formato no soportado\n";return false;}
    if(!ok)return false;
    finalizarTerreno(raw,outPos);
    std::cout<<"  Vertices: "<<raw.size()<<"  Triangulos: "<<(outIdx.size()/3)<<"\n";
    return true;
}

// ============================================================================
//  DRON SKINNED: hornear pose de reposo + marcar helices
// ============================================================================
static glm::mat4 matNodo(const tinygltf::Node& n) {
    if (n.matrix.size()==16) {
        float v[16]; for(int i=0;i<16;i++) v[i]=(float)n.matrix[i];
        return glm::make_mat4(v);
    }
    glm::mat4 T(1),R(1),S(1);
    if(n.translation.size()==3) T=glm::translate(glm::mat4(1),glm::vec3(n.translation[0],n.translation[1],n.translation[2]));
    if(n.rotation.size()==4) R=glm::mat4_cast(glm::quat((float)n.rotation[3],(float)n.rotation[0],(float)n.rotation[1],(float)n.rotation[2]));
    if(n.scale.size()==3) S=glm::scale(glm::mat4(1),glm::vec3(n.scale[0],n.scale[1],n.scale[2]));
    return T*R*S;
}
static void calcGlobal(const tinygltf::Model& m, int nodo, const glm::mat4& padre,
                       std::vector<glm::mat4>& g, std::vector<glm::mat4>& l) {
    g[nodo]=padre*l[nodo];
    for(int c:m.nodes[nodo].children) calcGlobal(m,c,g[nodo],g,l);
}

// Devuelve datos interleaved [x,y,z,propId] + indices y los pivotes de las helices
bool cargarDron(const std::string& ruta, std::vector<float>& outVtx, std::vector<unsigned int>& outIdx) {
    tinygltf::TinyGLTF loader; tinygltf::Model m; std::string e,w;
    if(!loader.LoadBinaryFromFile(&m,&e,&w,ruta)){std::cerr<<"ERROR dron: "<<e<<"\n";return false;}
    if(m.skins.empty()){std::cerr<<"ERROR: el dron no tiene skin\n";return false;}

    // 1) Matrices locales y globales de todos los nodos
    int N=(int)m.nodes.size();
    std::vector<glm::mat4> local(N),global(N,glm::mat4(1));
    for(int i=0;i<N;i++) local[i]=matNodo(m.nodes[i]);
    int escena=m.defaultScene>=0?m.defaultScene:0;
    for(int r:m.scenes[escena].nodes) calcGlobal(m,r,glm::mat4(1),global,local);

    // 2) Skin: joints + inverseBindMatrices
    const tinygltf::Skin& skin=m.skins[0];
    int numJ=(int)skin.joints.size();
    std::vector<glm::mat4> invBind(numJ,glm::mat4(1));
    if(skin.inverseBindMatrices>=0){
        const tinygltf::Accessor& ac=m.accessors[skin.inverseBindMatrices];
        const tinygltf::BufferView& bv=m.bufferViews[ac.bufferView];
        const tinygltf::Buffer& bf=m.buffers[bv.buffer];
        const uint8_t* p=bf.data.data()+bv.byteOffset+ac.byteOffset;
        for(int j=0;j<numJ;j++) invBind[j]=glm::make_mat4(reinterpret_cast<const float*>(p+(size_t)j*64));
    }
    // Matriz de skin por joint (lleva vertices del bind-space al mundo)
    std::vector<glm::mat4> skinMat(numJ);
    for(int j=0;j<numJ;j++) skinMat[j]=global[skin.joints[j]]*invBind[j];

    // Joints de las 4 helices (prop_1..4_jnt) -> indice dentro de skin.joints
    int propJoint[4]={-1,-1,-1,-1};
    for(int j=0;j<numJ;j++){
        std::string nm=m.nodes[skin.joints[j]].name;
        for(int k=0;k<4;k++){ std::string clave="prop_"+std::to_string(k+1)+"_jnt"; if(nm.find(clave)!=std::string::npos) propJoint[k]=j; }
    }
    // Pivote (posicion mundial) de cada helice
    for(int k=0;k<4;k++) dronPivots[k]= (propJoint[k]>=0) ? glm::vec3(global[skin.joints[propJoint[k]]][3]) : glm::vec3(0);

    // 3) Recorrer mallas skinned y hornear cada vertice
    std::vector<glm::vec3> pos; std::vector<float> pid;
    for(auto& node:m.nodes){
        if(node.mesh<0) continue;
        const tinygltf::Mesh& mesh=m.meshes[node.mesh];
        for(auto& pr:mesh.primitives){
            auto itP=pr.attributes.find("POSITION");
            auto itJ=pr.attributes.find("JOINTS_0");
            auto itW=pr.attributes.find("WEIGHTS_0");
            if(itP==pr.attributes.end()||itJ==pr.attributes.end()||itW==pr.attributes.end()) continue;
            const tinygltf::Accessor& aP=m.accessors[itP->second];
            const tinygltf::Accessor& aJ=m.accessors[itJ->second];
            const tinygltf::Accessor& aW=m.accessors[itW->second];
            const tinygltf::BufferView& vP=m.bufferViews[aP.bufferView];
            const tinygltf::BufferView& vJ=m.bufferViews[aJ.bufferView];
            const tinygltf::BufferView& vW=m.bufferViews[aW.bufferView];
            const uint8_t* pP=m.buffers[vP.buffer].data.data()+vP.byteOffset+aP.byteOffset;
            const uint8_t* pJ=m.buffers[vJ.buffer].data.data()+vJ.byteOffset+aJ.byteOffset;
            const uint8_t* pW=m.buffers[vW.buffer].data.data()+vW.byteOffset+aW.byteOffset;
            size_t sP=aP.ByteStride(vP), sJ=aJ.ByteStride(vJ), sW=aW.ByteStride(vW);
            size_t base=pos.size();
            for(size_t i=0;i<aP.count;i++){
                const float* fp=reinterpret_cast<const float*>(pP+i*sP);
                glm::vec4 vpos(fp[0],fp[1],fp[2],1.0f);
                // joints (ushort o ubyte)
                unsigned int jt[4];
                const uint8_t* jp=pJ+i*sJ;
                if(aJ.componentType==TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) for(int k=0;k<4;k++) jt[k]=jp[k];
                else for(int k=0;k<4;k++) jt[k]=reinterpret_cast<const uint16_t*>(jp)[k];
                const float* wp=reinterpret_cast<const float*>(pW+i*sW);
                glm::mat4 sk(0.0f); float wsum=0;
                for(int k=0;k<4;k++){ float ww=wp[k]; if(ww<=0)continue; sk+=ww*skinMat[jt[k]]; wsum+=ww; }
                if(wsum<1e-6f) sk=glm::mat4(1.0f);
                glm::vec4 wpos=sk*vpos;
                pos.push_back(glm::vec3(wpos));
                // propId = helice dominante
                int kmax=0; float wmax=wp[0];
                for(int k=1;k<4;k++) if(wp[k]>wmax){wmax=wp[k];kmax=k;}
                int dom=(int)jt[kmax]; float id=0.0f;
                for(int k=0;k<4;k++) if(dom==propJoint[k]) id=(float)(k+1);
                pid.push_back(id);
            }
            // indices
            if(pr.indices>=0){
                const tinygltf::Accessor& ia=m.accessors[pr.indices];
                const tinygltf::BufferView& ibv=m.bufferViews[ia.bufferView];
                const uint8_t* ip=m.buffers[ibv.buffer].data.data()+ibv.byteOffset+ia.byteOffset;
                for(size_t i=0;i<ia.count;i++){unsigned int id=0;
                    switch(ia.componentType){
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: id=ip[i];break;
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: id=reinterpret_cast<const uint16_t*>(ip)[i];break;
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: id=reinterpret_cast<const uint32_t*>(ip)[i];break;
                        default:continue;}
                    outIdx.push_back((unsigned int)(base+id));}
            } else for(size_t i=base;i<pos.size();i++) outIdx.push_back((unsigned int)i);
        }
    }
    if(pos.empty()){std::cerr<<"ERROR: dron sin vertices\n";return false;}

    // 4) Centrar + escalar a TAMANIO_DRON (y los pivotes igual)
    glm::vec3 bMin(1e9f),bMax(-1e9f);
    for(auto&p:pos){bMin=glm::min(bMin,p);bMax=glm::max(bMax,p);}
    glm::vec3 c=(bMin+bMax)*0.5f; glm::vec3 sp=bMax-bMin;
    float spanMax=std::max(sp.x,std::max(sp.y,sp.z));
    float esc=(spanMax>1e-6f)?(TAMANIO_DRON/spanMax):1.0f;
    outVtx.clear(); outVtx.reserve(pos.size()*4);
    for(size_t i=0;i<pos.size();i++){
        glm::vec3 n=(pos[i]-c)*esc;
        outVtx.push_back(n.x);outVtx.push_back(n.y);outVtx.push_back(n.z);outVtx.push_back(pid[i]);
    }
    for(int k=0;k<4;k++) dronPivots[k]=(dronPivots[k]-c)*esc;

    std::cout<<"Dron horneado: "<<pos.size()<<" vertices, "<<(outIdx.size()/3)<<" triangulos\n";
    return true;
}

// ============================================================================
//  DESCUBRIR TERRENOS (excluye el dron)
// ============================================================================
std::vector<std::string> descubrirModelos(const std::string& carpeta) {
    std::vector<std::string> l;
    if(!fs::exists(carpeta)){std::cerr<<"WARN: no existe "<<carpeta<<"\n";return l;}
    for(auto&e:fs::directory_iterator(carpeta)){
        if(!e.is_regular_file())continue;
        std::string ext=e.path().extension().string(),nl=e.path().filename().string();
        std::transform(ext.begin(),ext.end(),ext.begin(),::tolower);
        std::transform(nl.begin(),nl.end(),nl.begin(),::tolower);
        if(nl.find("drone")!=std::string::npos||nl.find("dron")!=std::string::npos)continue;
        if(ext==".obj"||ext==".glb"||ext==".gltf"||ext==".csv") l.push_back(e.path().string());
    }
    std::sort(l.begin(),l.end());
    return l;
}

// ============================================================================
//  SUBIR TERRENO A GPU
// ============================================================================
void subirTerreno(GLuint& vM,GLuint& bM,GLuint& eb,GLuint& vP,GLuint& bP,int& nP,
                  const std::vector<float>& pos,const std::vector<unsigned int>& idx) {
    if(vM){glDeleteVertexArrays(1,&vM);glDeleteBuffers(1,&bM);glDeleteBuffers(1,&eb);}
    if(vP){glDeleteVertexArrays(1,&vP);glDeleteBuffers(1,&bP);}
    glGenVertexArrays(1,&vM);glGenBuffers(1,&bM);glGenBuffers(1,&eb);
    glBindVertexArray(vM);
    glBindBuffer(GL_ARRAY_BUFFER,bM);
    glBufferData(GL_ARRAY_BUFFER,(GLsizeiptr)(pos.size()*sizeof(float)),pos.data(),GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,eb);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,(GLsizeiptr)(idx.size()*sizeof(unsigned int)),idx.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0); glEnableVertexAttribArray(0);
    int numV=(int)(pos.size()/3),paso=std::max(1,numV/PUNTOS_MAX);
    std::vector<float> pts; pts.reserve((numV/paso+1)*3);
    for(int v=0;v<numV;v+=paso){pts.push_back(pos[v*3]);pts.push_back(pos[v*3+1]);pts.push_back(pos[v*3+2]);}
    nP=(int)(pts.size()/3);
    glGenVertexArrays(1,&vP);glGenBuffers(1,&bP);
    glBindVertexArray(vP);
    glBindBuffer(GL_ARRAY_BUFFER,bP);
    glBufferData(GL_ARRAY_BUFFER,(GLsizeiptr)(pts.size()*sizeof(float)),pts.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0); glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    std::cout<<"  Puntos: "<<nP<<" (paso "<<paso<<")\n";
}

// ============================================================================
//  HUD 2D  (rectangulos + texto con stb_easy_font)
// ============================================================================
GLuint hudVAO=0,hudVBO=0,hudProg=0; GLint hudLocProj,hudLocModel,hudLocColor;
static char hudTextBuf[80000];

glm::vec4 rectMapBoton(int i){ // x,y,w,h en pixeles
    float bw=46,bh=46,gap=10,x0=24; float y0=SCR_HEIGHT-bh-24;
    return glm::vec4(x0+i*(bw+gap), y0, bw, bh);
}
// Rects de teclas de movimiento: 0=^ 1=< 2=v 3=> 4=SHIFT 5=SPACE
glm::vec4 rectTecla(int i){
    float kw=48,kh=40,gap=8;
    float bx=SCR_WIDTH-(kw*3+gap*2)-26;
    float yBot=SCR_HEIGHT-24-kh, yTop=yBot-kh-gap, yRow0=yTop-kh-gap;
    switch(i){
        case 0: return glm::vec4(bx+kw+gap, yTop, kw, kh);          // arriba
        case 1: return glm::vec4(bx,         yBot, kw, kh);          // izq
        case 2: return glm::vec4(bx+kw+gap,  yBot, kw, kh);          // abajo
        case 3: return glm::vec4(bx+2*(kw+gap), yBot, kw, kh);       // der
        case 4: return glm::vec4(bx, yRow0, kw*1.5f, kh);            // SHIFT
        case 5: return glm::vec4(bx+kw*1.5f+gap, yRow0, kw*1.5f+gap, kh); // SPACE
    }
    return glm::vec4(0);
}

void hudRect(float x,float y,float w,float h,glm::vec4 col){
    float v[12]={x,y, x+w,y, x+w,y+h, x,y, x+w,y+h, x,y+h};
    glBindBuffer(GL_ARRAY_BUFFER,hudVBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(v),v,GL_DYNAMIC_DRAW);
    glm::mat4 I(1.0f);
    glUniformMatrix4fv(hudLocModel,1,GL_FALSE,glm::value_ptr(I));
    glUniform4fv(hudLocColor,1,glm::value_ptr(col));
    glDrawArrays(GL_TRIANGLES,0,6);
}
void hudTexto(float x,float y,float escala,const char* txt,glm::vec4 col){
    unsigned char c[4]={255,255,255,255};
    int nq=stb_easy_font_print(0,0,(char*)txt,c,hudTextBuf,sizeof(hudTextBuf));
    // Expandir quads (4 v, stride 16B: 3 float pos + 4 byte color) a triangulos vec2
    std::vector<float> tri; tri.reserve(nq*6*2);
    const char* bp=hudTextBuf;
    for(int q=0;q<nq;q++){
        float qp[4][2];
        for(int k=0;k<4;k++){const float* f=reinterpret_cast<const float*>(bp+(q*4+k)*16);qp[k][0]=f[0];qp[k][1]=f[1];}
        int ord[6]={0,1,2,0,2,3};
        for(int k=0;k<6;k++){tri.push_back(qp[ord[k]][0]);tri.push_back(qp[ord[k]][1]);}
    }
    glBindBuffer(GL_ARRAY_BUFFER,hudVBO);
    glBufferData(GL_ARRAY_BUFFER,(GLsizeiptr)(tri.size()*sizeof(float)),tri.data(),GL_DYNAMIC_DRAW);
    glm::mat4 M=glm::translate(glm::mat4(1),glm::vec3(x,y,0))*glm::scale(glm::mat4(1),glm::vec3(escala,escala,1));
    glUniformMatrix4fv(hudLocModel,1,GL_FALSE,glm::value_ptr(M));
    glUniform4fv(hudLocColor,1,glm::value_ptr(col));
    glDrawArrays(GL_TRIANGLES,0,(GLsizei)(tri.size()/2));
}
void hudBoton(glm::vec4 r,const char* label,bool activo,bool resaltado){
    glm::vec4 borde = activo ? glm::vec4(1,0.83,0,1) : glm::vec4(0.5,0.55,0.6,0.9);
    glm::vec4 fondo = activo ? glm::vec4(1,0.83,0,0.25) : (resaltado?glm::vec4(0.3,0.34,0.4,0.85):glm::vec4(0.07,0.09,0.13,0.8));
    hudRect(r.x,r.y,r.z,r.w,borde);
    hudRect(r.x+2,r.y+2,r.z-4,r.w-4,fondo);
    float esc=2.0f, tw=stb_easy_font_width((char*)label)*esc;
    glm::vec4 tcol = activo ? glm::vec4(1,0.9,0.3,1) : glm::vec4(0.85,0.9,0.95,1);
    hudTexto(r.x+(r.z-tw)*0.5f, r.y+(r.w-7*esc)*0.5f, esc, label, tcol);
}

// ============================================================================
//  CALLBACKS
// ============================================================================
void framebuffer_size_callback(GLFWwindow*,int w,int h){ SCR_WIDTH=w;SCR_HEIGHT=h; glViewport(0,0,w,h); }

void mouse_button_callback(GLFWwindow* win,int button,int action,int){
    if(button!=GLFW_MOUSE_BUTTON_LEFT) return;
    if(action==GLFW_PRESS){
        double mx,my; glfwGetCursorPos(win,&mx,&my);
        // Hit-test botones de mapa
        int n=(int)listaModelos.size();
        for(int i=0;i<n&&i<9;i++){
            glm::vec4 r=rectMapBoton(i);
            if(mx>=r.x&&mx<=r.x+r.z&&my>=r.y&&my<=r.y+r.w){
                if(i!=modeloActual){modeloActual=i;recargarModelo=true;}
                return; // no rotar camara
            }
        }
        arrastrando=true; primerMouse=true;
    } else if(action==GLFW_RELEASE) arrastrando=false;
}
void mouse_callback(GLFWwindow*,double x,double y){
    if(!arrastrando)return;
    if(primerMouse){lastX=x;lastY=y;primerMouse=false;}
    camYaw+=(float)(x-lastX)*0.3f; camElev+=(float)(lastY-y)*0.3f;
    lastX=x;lastY=y; camElev=std::clamp(camElev,ELEV_MIN,ELEV_MAX);
}
void scroll_callback(GLFWwindow*,double,double yo){ camRadius=std::clamp(camRadius-(float)yo*5.0f,RADIO_MIN,RADIO_MAX); }
void key_callback(GLFWwindow* win,int key,int,int action,int){
    if(action!=GLFW_PRESS)return;
    if(key==GLFW_KEY_ESCAPE){glfwSetWindowShouldClose(win,true);return;}
    int n=(int)listaModelos.size(); if(n==0)return;
    if(key==GLFW_KEY_TAB){modeloActual=(modeloActual+1)%n;recargarModelo=true;}
    else if(key==GLFW_KEY_BACKSPACE){modeloActual=(modeloActual-1+n)%n;recargarModelo=true;}
    else if(key>=GLFW_KEY_1&&key<=GLFW_KEY_9){int i=key-GLFW_KEY_1;if(i<n){modeloActual=i;recargarModelo=true;}}
}

// ============================================================================
//  MAIN
// ============================================================================
int main() {
    listaModelos=descubrirModelos("assets");
    if(listaModelos.empty()){std::cerr<<"ERROR: no hay terrenos en assets/\n";return -1;}
    std::cout<<"Terrenos:\n";
    for(int i=0;i<(int)listaModelos.size();i++) std::cout<<"  ["<<(i+1)<<"] "<<fs::path(listaModelos[i]).filename().string()<<"\n";

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window=glfwCreateWindow(SCR_WIDTH,SCR_HEIGHT,"Simulador Topografico",nullptr,nullptr);
    if(!window){std::cerr<<"ERROR ventana\n";glfwTerminate();return -1;}
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window,framebuffer_size_callback);
    glfwSetMouseButtonCallback(window,mouse_button_callback);
    glfwSetCursorPosCallback(window,mouse_callback);
    glfwSetScrollCallback(window,scroll_callback);
    glfwSetKeyCallback(window,key_callback);
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){std::cerr<<"ERROR GLAD\n";return -1;}

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glPointSize(1.6f);

    GLuint shTerreno=crearShader("shaders/terrain.vert","shaders/terrain.frag");
    GLint tLocM=glGetUniformLocation(shTerreno,"model"),tLocV=glGetUniformLocation(shTerreno,"view"),
          tLocP=glGetUniformLocation(shTerreno,"projection"),tLocC=glGetUniformLocation(shTerreno,"uColor"),
          tLocA=glGetUniformLocation(shTerreno,"uAlpha");

    GLuint shDron=crearShader("shaders/drone.vert","shaders/drone.frag");
    GLint dLocM=glGetUniformLocation(shDron,"model"),dLocV=glGetUniformLocation(shDron,"view"),
          dLocP=glGetUniformLocation(shDron,"projection"),dLocT=glGetUniformLocation(shDron,"uTime"),
          dLocS=glGetUniformLocation(shDron,"uSpin"),dLocPiv=glGetUniformLocation(shDron,"uPivots"),
          dLocC=glGetUniformLocation(shDron,"uColor"),dLocA=glGetUniformLocation(shDron,"uAlpha");

    hudProg=crearShader("shaders/hud.vert","shaders/hud.frag");
    hudLocProj=glGetUniformLocation(hudProg,"uProj");
    hudLocModel=glGetUniformLocation(hudProg,"uModel");
    hudLocColor=glGetUniformLocation(hudProg,"uColor");
    glGenVertexArrays(1,&hudVAO); glGenBuffers(1,&hudVBO);
    glBindVertexArray(hudVAO); glBindBuffer(GL_ARRAY_BUFFER,hudVBO);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,2*sizeof(float),(void*)0); glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // ---- Terreno ----
    GLuint vM=0,bM=0,eb=0,vP=0,bP=0; int numIdx=0,numPts=0;
    auto cargarActual=[&](){
        const std::string& ruta=listaModelos[modeloActual];
        std::string nombre=fs::path(ruta).filename().string();
        std::cout<<"\nCargando ["<<(modeloActual+1)<<"/"<<listaModelos.size()<<"] "<<nombre<<" ...\n";
        std::vector<float> pos; std::vector<unsigned int> idx;
        if(!cargarTerreno(ruta,pos,idx)){std::cerr<<"  Fallo.\n";return;}
        subirTerreno(vM,bM,eb,vP,bP,numPts,pos,idx); numIdx=(int)idx.size();
        dronPos=glm::vec3(0,alturaTerreno(0,0)+ALTURA_INICIAL,0); dronYaw=0; camRadius=55.0f;
        std::string t="Simulador Topografico  |  "+nombre+"  ["+std::to_string(modeloActual+1)+"/"+std::to_string(listaModelos.size())+"]";
        glfwSetWindowTitle(window,t.c_str());
    };
    cargarActual();

    // ---- Dron ----
    GLuint vD=0,bD=0,eD=0; int numIdxD=0; bool hayDron=false;
    {
        std::vector<float> dv; std::vector<unsigned int> di;
        if(fs::exists(RUTA_DRON)&&cargarDron(RUTA_DRON,dv,di)){
            glGenVertexArrays(1,&vD);glGenBuffers(1,&bD);glGenBuffers(1,&eD);
            glBindVertexArray(vD);
            glBindBuffer(GL_ARRAY_BUFFER,bD);
            glBufferData(GL_ARRAY_BUFFER,(GLsizeiptr)(dv.size()*sizeof(float)),dv.data(),GL_STATIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,eD);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,(GLsizeiptr)(di.size()*sizeof(unsigned int)),di.data(),GL_STATIC_DRAW);
            glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)0); glEnableVertexAttribArray(0);
            glVertexAttribPointer(1,1,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)(3*sizeof(float))); glEnableVertexAttribArray(1);
            glBindVertexArray(0); numIdxD=(int)di.size(); hayDron=true;
        } else std::cerr<<"AVISO: no se cargo el dron\n";
    }

    std::cout<<"\nControles: Flechas/WASD mover | Espacio/Shift subir-bajar | mouse rotar | scroll zoom | botones 1-4 mapa | ESC salir\n\n";

    float tPrev=(float)glfwGetTime();
    while(!glfwWindowShouldClose(window)){
        float ahora=(float)glfwGetTime(),dt=ahora-tPrev; tPrev=ahora;
        if(recargarModelo){recargarModelo=false;cargarActual();}

        float yawR=glm::radians(camYaw),elevR=glm::radians(camElev);
        glm::vec3 adelante=glm::normalize(glm::vec3(-sinf(yawR),0,-cosf(yawR)));
        glm::vec3 derecha =glm::normalize(glm::vec3( cosf(yawR),0,-sinf(yawR)));

        // ---- Mover dron (sin fisica) ----
        bool kUp =glfwGetKey(window,GLFW_KEY_UP)==GLFW_PRESS||glfwGetKey(window,GLFW_KEY_W)==GLFW_PRESS;
        bool kDn =glfwGetKey(window,GLFW_KEY_DOWN)==GLFW_PRESS||glfwGetKey(window,GLFW_KEY_S)==GLFW_PRESS;
        bool kRt =glfwGetKey(window,GLFW_KEY_RIGHT)==GLFW_PRESS||glfwGetKey(window,GLFW_KEY_D)==GLFW_PRESS;
        bool kLf =glfwGetKey(window,GLFW_KEY_LEFT)==GLFW_PRESS||glfwGetKey(window,GLFW_KEY_A)==GLFW_PRESS;
        bool kSp =glfwGetKey(window,GLFW_KEY_SPACE)==GLFW_PRESS;
        bool kSh =glfwGetKey(window,GLFW_KEY_LEFT_SHIFT)==GLFW_PRESS||glfwGetKey(window,GLFW_KEY_RIGHT_SHIFT)==GLFW_PRESS;

        glm::vec3 mov(0);
        if(kUp)mov+=adelante; if(kDn)mov-=adelante; if(kRt)mov+=derecha; if(kLf)mov-=derecha;
        if(glm::length(mov)>1e-4f){ mov=glm::normalize(mov); dronPos+=mov*VEL_DRON*dt; dronYaw=glm::degrees(atan2f(mov.x,mov.z))+DRON_OFFSET_YAW; }
        if(kSp)dronPos.y+=VEL_ALTURA*dt;
        if(kSh)dronPos.y-=VEL_ALTURA*dt;

        glm::vec3 camPos=dronPos+glm::vec3(camRadius*cosf(elevR)*sinf(yawR),camRadius*sinf(elevR),camRadius*cosf(elevR)*cosf(yawR));

        glClearColor(0.039f,0.051f,0.078f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        glm::mat4 view=glm::lookAt(camPos,dronPos,glm::vec3(0,1,0));
        glm::mat4 proj=glm::perspective(glm::radians(45.0f),(float)SCR_WIDTH/(float)SCR_HEIGHT,0.1f,1500.0f);

        // ---- Terreno (wireframe + puntos) ----
        glUseProgram(shTerreno);
        glm::mat4 I(1.0f);
        glUniformMatrix4fv(tLocM,1,GL_FALSE,glm::value_ptr(I));
        glUniformMatrix4fv(tLocV,1,GL_FALSE,glm::value_ptr(view));
        glUniformMatrix4fv(tLocP,1,GL_FALSE,glm::value_ptr(proj));
        glUniform3f(tLocC,0.85f,0.88f,0.92f); glUniform1f(tLocA,0.7f);
        glBindVertexArray(vP); glDrawArrays(GL_POINTS,0,numPts);
        glBindVertexArray(vM);
        if(g_esLineas){
            // Mapa de calles: dibujar segmentos directamente
            glDrawElements(GL_LINES,numIdx,GL_UNSIGNED_INT,0);
        } else {
            // Terreno 3D: malla en modo alambre
            glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
            glDrawElements(GL_TRIANGLES,numIdx,GL_UNSIGNED_INT,0);
            glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
        }

        // ---- Dron (solido, amarillo, helices girando) ----
        if(hayDron){
            glUseProgram(shDron);
            glm::mat4 mD=glm::translate(glm::mat4(1),dronPos);
            mD=glm::rotate(mD,glm::radians(dronYaw),glm::vec3(0,1,0));
            glUniformMatrix4fv(dLocM,1,GL_FALSE,glm::value_ptr(mD));
            glUniformMatrix4fv(dLocV,1,GL_FALSE,glm::value_ptr(view));
            glUniformMatrix4fv(dLocP,1,GL_FALSE,glm::value_ptr(proj));
            glUniform1f(dLocT,ahora); glUniform1f(dLocS,GIRO_HELICES);
            glUniform3fv(dLocPiv,4,glm::value_ptr(dronPivots[0]));
            glBindVertexArray(vD);

            // Pasada 1: relleno oscuro (oculta el interior -> se ve la silueta)
            glEnable(GL_POLYGON_OFFSET_FILL); glPolygonOffset(1.0f,1.0f);
            glUniform3f(dLocC,0.12f,0.11f,0.04f); glUniform1f(dLocA,1.0f);
            glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
            glDrawElements(GL_TRIANGLES,numIdxD,GL_UNSIGNED_INT,0);
            glDisable(GL_POLYGON_OFFSET_FILL);

            // Pasada 2: aristas amarillas encima (estructura del dron)
            glUniform3f(dLocC,0.5f,0.1f,0.0f); glUniform1f(dLocA,1.0f);
            glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
            glDrawElements(GL_TRIANGLES,numIdxD,GL_UNSIGNED_INT,0);
            glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
        }

        // ---- HUD 2D ----
        glDisable(GL_DEPTH_TEST);
        glUseProgram(hudProg);
        glm::mat4 ortho=glm::ortho(0.0f,(float)SCR_WIDTH,(float)SCR_HEIGHT,0.0f,-1.0f,1.0f);
        glUniformMatrix4fv(hudLocProj,1,GL_FALSE,glm::value_ptr(ortho));
        glBindVertexArray(hudVAO);

        // Botones de mapa (1..N) clicables
        for(int i=0;i<(int)listaModelos.size()&&i<9;i++){
            hudBoton(rectMapBoton(i),std::to_string(i+1).c_str(),i==modeloActual,false);
        }
        // Titulo de botones de mapa
        hudTexto(24,SCR_HEIGHT-86,1.6f,"MAPAS",glm::vec4(0.7,0.75,0.8,1));

        // Teclas de movimiento (se resaltan al presionarlas)
        hudBoton(rectTecla(0),"^",false,kUp);
        hudBoton(rectTecla(1),"<",false,kLf);
        hudBoton(rectTecla(2),"v",false,kDn);
        hudBoton(rectTecla(3),">",false,kRt);
        hudBoton(rectTecla(4),"SHIFT",false,kSh);
        hudBoton(rectTecla(5),"SPACE",false,kSp);
        { glm::vec4 r=rectTecla(4); hudTexto(r.x,r.y-18,1.4f,"BAJAR / SUBIR",glm::vec4(0.7,0.75,0.8,1)); }

        glEnable(GL_DEPTH_TEST);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteProgram(shTerreno); glDeleteProgram(shDron); glDeleteProgram(hudProg);
    glfwTerminate();
    return 0;
}
