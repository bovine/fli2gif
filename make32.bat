@echo off
bcc /ml /ef2g16.exe fli2gif.cpp gif.cpp fliplay.cpp noehl.lib
bcc32 fli2gif.cpp gif.cpp fliplay.cpp
