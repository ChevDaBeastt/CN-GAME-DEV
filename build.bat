call vcvarsall x64
cl -Zi -Fegame ./src/startup_win32.cpp user32.lib gdi32.lib
copy game.exe "./build/game.exe"
copy game.pdb "./build/game.pdb"
del startup_win32.obj game.exe game.pdb game.ilk vc140.pdb