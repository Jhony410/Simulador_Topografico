#pragma once
#include "MallaCruda.h"

#include <glm/glm.hpp>
#include <string>
#include <vector>

// ============================================================================
//  Lectura de archivos de geometria. Pertenece al MODELO: produce vectores de
//  numeros, nunca buffers de OpenGL.
// ============================================================================
namespace CargadorModelos {

// Malla triangulada desde Wavefront .obj (ignora el objeto llamado "Sphere").
bool cargarOBJ(const std::string& ruta,
               std::vector<glm::vec3>& posiciones,
               std::vector<unsigned int>& indices);

// Terreno desde glTF binario o ASCII: solo POSITION + indices.
bool cargarGLBTerreno(const std::string& ruta,
                      std::vector<glm::vec3>& posiciones,
                      std::vector<unsigned int>& indices);

// Red de calles OpenStreetMap en CSV con geometria LINESTRING (plano Y = 0).
bool cargarCSVCalles(const std::string& ruta,
                     std::vector<glm::vec3>& posiciones,
                     std::vector<unsigned int>& indices);

// Dron skinned: hornea la pose de reposo en CPU (queda derecho) y marca cada
// vertice con el id de helice dominante para que el shader la haga girar.
// Devuelve la malla interleaved [x, y, z, idHelice] y los 4 pivotes de giro.
bool cargarDronAnimado(const std::string& ruta,
                       MallaCruda& malla,
                       glm::vec3 pivotesHelices[4],
                       float tamanioObjetivo);

// Lista de terrenos disponibles en una carpeta (excluye el modelo del dron).
std::vector<std::string> descubrirTerrenos(const std::string& carpeta);

} // namespace CargadorModelos
