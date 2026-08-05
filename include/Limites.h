#pragma once

// ============================================================================
//  Caja de mundo del terreno ya normalizado.
//  La comparten el Modelo (Terreno, MapaExploracion, CurvasNivel) para mapear
//  coordenadas de mundo <-> indices de celda sin repetir constantes.
// ============================================================================
struct LimitesMundo {
    float minX = -50.0f, maxX = 50.0f;
    float minZ = -50.0f, maxZ = 50.0f;
    float minY =   0.0f, maxY =  0.0f;

    float ancho()      const { return maxX - minX; }
    float profundidad() const { return maxZ - minZ; }
    float rangoAltura() const { return maxY - minY; }
};
