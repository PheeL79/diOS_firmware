@ECHO OFF
PUSHD
CD /D %~dp0
%1 firmware.ewp -build %2 -log warnings
POPD