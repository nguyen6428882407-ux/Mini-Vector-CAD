@echo off
chcp 65001 >nul
echo ======================================================
echo    KHOI DONG HE THONG MINI VECTOR CAD (C++ BACKEND + WEB)
echo ======================================================

echo 1. Dang khoi dong C++ Backend Server (Cong 8766)...
start "MiniVectorCAD C++ Backend" "%~dp0build\MiniVectorCADServer.exe"

timeout /t 1 /nobreak >nul

echo 2. Dang khoi dong Web Server (Cong 8765)...
start "MiniVectorCAD Web Server" python -m http.server 8765 --bind 127.0.0.1

timeout /t 1 /nobreak >nul

echo 3. Dang mo trinh duyet Web...
start http://127.0.0.1:8765/web/

echo ======================================================
echo He thong da duoc khoi dong thanh cong!
echo ======================================================
