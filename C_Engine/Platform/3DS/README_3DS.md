3DS Port Guide

Step-by-step to build the 3DS target on Windows (devkitPro):

1. Install devkitPro and devkitARM per https://devkitpro.org/wiki/Getting_Started
   - Recommended path on Windows: C:/devkitPro
   - Ensure ctrulib and citro3d are installed via pacman (devkitPro package manager) if you want rendering: pacman -S citro3d ctrulib makerom

2. Open a devkitPro shell (PowerShell or Bash) that sets DEVKITPRO/DEVKITARM. If you installed to C:/devkitPro, make should detect it automatically. Otherwise set DEVKITPRO manually:
   - PowerShell: $env:DEVKITPRO = 'C:/devkitPro'
   - Or in bash: export DEVKITPRO=/opt/devkitpro

3. From repository root:
   cd C_Engine
   make 3ds-clean
   make 3ds

4. If build succeeds you will get OpenBLOX3DS.3dsx and/or OpenBLOX3DS.cia
   - Copy .3dsx to your SD card and run via homebrew launcher (for 3DS) or test in Citra (3DS emulator).

Notes:
- This port currently provides platform glue and loader. Rendering functions are stubs unless citro3d integration is implemented. For visuals, see Platform/3DS comments and implement Citro3D shaders and attribute bindings.
- If make fails due to missing tools (makerom), install makerom via pacman or add it to PATH.
- If you want me to implement Citro3D-based rendering next, confirm and I will add basic shader and attribute code to render triangles used by the engine.
