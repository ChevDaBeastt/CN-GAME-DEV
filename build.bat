call vcvarsall x64
cl ./src/startup_win32.cpp user32.lib gdi32.lib
copy startup_win32.exe "./build/game.exe"
del startup_win32.obj startup_win32.exe