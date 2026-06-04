@echo off
echo Compilando Simulador Topografico (Fase 1)...
g++ src/main.cpp src/glad.c -o simulador.exe -I include -L lib -lglfw3 -lgdi32 -lopengl32 -std=c++17
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Hubo un problema al compilar. Revisa el texto de arriba.
) else (
    echo [EXITO] Compilacion terminada. Abriendo simulador...
    echo ==================================================
    simulador.exe
)
