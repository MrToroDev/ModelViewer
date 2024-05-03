@echo off
setlocal

set COMPILATION_VARS=-V --target-env vulkan1.3 -w -t -g
set workspace_folder=%cd%
set BIN_FOLDER=../Content/Shaders
set TEMP_BIN_FOLDER=bin
set OUTPUT_ASSEMBLY=0

if not exist "%BIN_FOLDER%" (
	mkdir "%BIN_FOLDER%"
)

if not exist "%TEMP_BIN_FOLDER%" (
	mkdir "%TEMP_BIN_FOLDER%"
)

cd %BIN_FOLDER%
del /q *.spv
cd %workspace_folder%

cd %TEMP_BIN_FOLDER%
del /q *.spv
del /q *.spvasm
cd %workspace_folder%

call :compile_shaders
exit /b

:compile_shaders

@REM COMPILE SHADERS IN TEMP FOLDER
echo compiling vertex shaders...
for /R %%f in (*.vert) do (
	glslangvalidator %COMPILATION_VARS% %%f -o %TEMP_BIN_FOLDER%/temp_%%~nf.spv
)

echo compiling fragment shaders...
for /R %%f in (*.frag) do (
	glslangvalidator %COMPILATION_VARS% %%f -o %TEMP_BIN_FOLDER%/temp_%%~nf.spv
)

cd %TEMP_BIN_FOLDER%
rename "temp_*.spv" "/////*.spv" 
cd %workspace_folder%

@REM COPY TO BIN_FOLDER
for %%f in (%TEMP_BIN_FOLDER%/*.spv) do (
	if %OUTPUT_ASSEMBLY% == 1 ( spirv-dis --raw-id %TEMP_BIN_FOLDER%/%%f -o %TEMP_BIN_FOLDER%/%%~nf.spvasm )

	move "%TEMP_BIN_FOLDER%\%%f" "%BIN_FOLDER%\" > nul
)

endlocal