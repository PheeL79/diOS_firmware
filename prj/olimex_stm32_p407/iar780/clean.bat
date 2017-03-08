@ECHO OFF
PUSHD
CD /D %~dp0
DEL /Q /F /S *.dep *.ewd
FOR /D %%f IN (debug*, release*, settings) DO RMDIR "%%f" /S /Q
POPD
