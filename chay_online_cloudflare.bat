@echo off
chcp 65001 >nul
echo ======================================================
echo    KHOI DONG MINI VECTOR CAD QUA CLOUDFLARE TUNNEL
echo ======================================================

echo 1. Dang khoi dong C++ Backend Server (Cong 8766)...
start "MiniVectorCAD C++ Backend" "%~dp0build\MiniVectorCADServer.exe"

timeout /t 2 /nobreak >nul

echo 2. Dang tao Cloudflare Tunnel cong khai (Mien phi)...
echo ------------------------------------------------------
echo Luu y: Trinh duyet se hien thi duong link HTTPS cong khai
echo dang: https://xxxx.trycloudflare.com
echo Ban chi can copy link do gui cho ban be la vao duoc ngay!
echo ------------------------------------------------------

"%~dp0cloudflared.exe" tunnel --url http://localhost:8766

pause
