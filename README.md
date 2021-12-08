# falling-sand-cubed
A 3D falling sand game, written in OpenGL for a Computer Graphics class project. 

Intended final use is to operate upon the sand grid using a compute shader, which subdivides it into configurably-sized columns to operate on. In the initial stages, the grid will be using a CPU-managed render technique (manipulating uniforms to transform a voxel model), but eventually it is planned to use the tessellation and geometry shaders in order to generate the necessary geometry to render in the fragment shader.

Logic for particles follows the standard rules of "falling sand" or "powder" type games.

## Downloads

Pre-built downloads can be found in the [Releases](https://github.com/d-mckee/falling-sand-cubed/releases) section of the repo.

To download and build the project yourself, the included .sln file should work with Visual Studio 2019 (or 2022, as that's what we use to develop), and requires:
- Windows 64-bit
- A GPU with support for at least OpenGL 4.3
- Visual C++ Runtime (should be included in your Visual Studio installation)

Any issues doing so can be reported in the [Issues](https://github.com/d-mckee/falling-sand-cubed/issues) section.

## Demo

Short video of basic functionality
https://user-images.githubusercontent.com/36770835/145133517-5377dbb7-a3ed-4378-ac2a-fa35aa32194a.mp4

Screenshot of some UI elements
![UI element demo](https://user-images.githubusercontent.com/36770835/145133572-bcfefc5a-1248-4b32-8a80-35fa195a23aa.png)
