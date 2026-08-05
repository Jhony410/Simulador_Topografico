@echo off
echo Compilando Simulador Topografico...

g++ ^
 src/main.cpp ^
 src/Aplicacion.cpp ^
 src/Escena.cpp ^
 src/NodoEscena.cpp ^
 src/Terreno.cpp ^
 src/Dron.cpp ^
 src/MapaExploracion.cpp ^
 src/CurvasNivel.cpp ^
 src/MarcadoresSondeo.cpp ^
 src/SistemaEscaneo.cpp ^
 src/EstadoMision.cpp ^
 src/AvisoHUD.cpp ^
 src/Persistencia.cpp ^
 src/marchingSquares.cpp ^
 src/GrafoCurvas.cpp ^
 src/GeneradorCurvas.cpp ^
 src/Frustum.cpp ^
 src/Quadtree.cpp ^
 src/BVH.cpp ^
 src/AristasCaracteristicas.cpp ^
 src/CargadorModelos.cpp ^
 src/Camara.cpp ^
 src/UtilidadesGL.cpp ^
 src/GestorRecursos.cpp ^
 src/ContadorRendimiento.cpp ^
 src/DisenoHUD.cpp ^
 src/VistaTerreno.cpp ^
 src/VistaMarcadores.cpp ^
 src/VistaDron.cpp ^
 src/VistaCurvas.cpp ^
 src/VistaHUD.cpp ^
 src/PostProceso.cpp ^
 src/Renderizador.cpp ^
 src/ControladorEntrada.cpp ^
 src/glad.c ^
 -o simulador.exe -I include -L lib -lglfw3 -lgdi32 -lopengl32 -std=c++17

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Hubo un problema al compilar. Revisa el texto de arriba.
) else (
    echo [EXITO] Compilacion terminada. Abriendo simulador...
    echo ==================================================
    simulador.exe
)
