cscript /nologo update.js "https://drive.google.com/uc?export=download&id=0B57eDGS2U35EZVhsLTFKWTItSms" ./RGC/remoteGamesControll.exe_tmp
cd RGC
move /Y remoteGamesControll.exe_tmp remoteGamesControll.exe
cd ..