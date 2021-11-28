# falling-sand-cubed
A 3D falling sand game, written in OpenGL for a Computer Graphics class project. 

Intended final use is to operate upon the sand grid using a compute shader, which subdivides it into configurably-sized columns to operate on. In the initial stages, the grid will be using a CPU-managed render technique (manipulating uniforms to transform a voxel model), but eventually it is planned to use the tessellation and geometry shaders in order to generate the necessary geometry to render in the fragment shader.

Logic for particles follows the standard rules of "falling sand" or "powder" type games.
