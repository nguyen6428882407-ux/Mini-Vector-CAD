@echo off
chcp 65001 >nul
echo ======================================================
echo    KHOI DONG HE THONG MINI VECTOR CAD (C++ BACKEND + WEB)
echo ======================================================

echo 1. Dang khoi dong C++ Backend & Web Server (Cong 8766)...
start "MiniVectorCAD C++ Backend" "%~dp0build\MiniVectorCADServer.exe"

timeout /t 1 /nobreak >nul

echo 2. Dang mo trinh duyet Web...
start http://localhost:8766/

echo ======================================================
echo He thong da duoc khoi dong thanh cong tai port 8766!
echo ======================================================
