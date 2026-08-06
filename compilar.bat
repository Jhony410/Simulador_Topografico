@echo off
setlocal EnableExtensions
pushd "%~dp0"

echo [BUILD] Compilando GeoDrone con C++17 y OpenGL 3.3...
where g++ >nul 2>nul
if errorlevel 1 (
    echo [ERROR] No se encontro g++ en PATH.
    echo         Instala MinGW-w64/MSYS2 y vuelve a abrir la terminal.
    popd
    exit /b 1
)

g++ ^
 "src\main.cpp" ^
 "src\Aplicacion.cpp" ^
 "src\Escena.cpp" ^
 "src\NodoEscena.cpp" ^
 "src\Terreno.cpp" ^
 "src\Dron.cpp" ^
 "src\MapaExploracion.cpp" ^
 "src\CurvasNivel.cpp" ^
 "src\MarcadoresSondeo.cpp" ^
 "src\SistemaEscaneo.cpp" ^
 "src\SistemaExploracion.cpp" ^
 "src\SistemaMedicion.cpp" ^
 "src\EstadoMision.cpp" ^
 "src\AvisoHUD.cpp" ^
 "src\Persistencia.cpp" ^
 "src\marchingSquares.cpp" ^
 "src\GrafoCurvas.cpp" ^
 "src\GeneradorCurvas.cpp" ^
 "src\Frustum.cpp" ^
 "src\Quadtree.cpp" ^
 "src\BVH.cpp" ^
 "src\AristasCaracteristicas.cpp" ^
 "src\CargadorModelos.cpp" ^
 "src\Camara.cpp" ^
 "src\UtilidadesGL.cpp" ^
 "src\GestorRecursos.cpp" ^
 "src\ContadorRendimiento.cpp" ^
 "src\DisenoHUD.cpp" ^
 "src\VistaTerreno.cpp" ^
 "src\VistaMarcadores.cpp" ^
 "src\VistaExploracion.cpp" ^
 "src\VistaDron.cpp" ^
 "src\VistaCurvas.cpp" ^
 "src\VistaMinimapa.cpp" ^
 "src\VistaHUD.cpp" ^
 "src\PostProceso.cpp" ^
 "src\Renderizador.cpp" ^
 "src\ControladorEntrada.cpp" ^
 "src\glad.c" ^
 -o "simulador.exe" -I"include" -L"lib" -lglfw3 -lgdi32 -lopengl32 -std=c++17 -O2

if errorlevel 1 (
    echo.
    echo [ERROR] La compilacion fallo. Revisa el primer diagnostico mostrado arriba.
    popd
    exit /b 1
)

echo [BUILD] Compilacion terminada: "%CD%\simulador.exe"
if /I "%~1"=="ejecutar" (
    echo [RUN] Iniciando GeoDrone...
    "simulador.exe"
)

popd
exit /b 0
