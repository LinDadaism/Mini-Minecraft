# CIS560 Final Project: Mini Minecraft

## Milestone-3

**Features & Distribution**

	Tianyi Xiao:
		- Additional Biomes
		- L-system River
		- Procedurally Placed Assets
		- Procedural buildings with shape grammars
	Runjie Zhao:
		- Fluid Simulation
		- Sound
		- Water Waves
		- Post-process Camera Overlay
	Linda Zhu:
		- Procedural Sky Box
		- Distance Fog
		- Shadow Mapping



**Implementation Details**

Linda - Procedural Sky Box:

	I built up from the given demo code. The main idea is to build a procedural sky background using raycasting from the camera to the pixel to decide which color we should color it in. With a dedicated shaderprogram, I draw the skybox on the quadrangle that represents the current scene, and overlay the terrain texture/ post-processing render on top of it. While the skybox vertex shader simplying pass through the position data, the fragment shader does all of the visual manipulation work. In the fragment shader, I modified the position of the sun over time using some variation of cos(time) so that the sun moves around the center of the scene in the y-z plane. I blended in the sky's color as the sun moves from a morning sunrise to afternoon overhead to sunset to night-time. During daylight time, I added animated perlin-noise clouds. During night time, I added a slowly rotating starry background field around the screen center by layering noise points that mimics stars with various sizes. I also added shooting stars with random chances of appearance in the night sky to create a mysterious vitality of the scene. The starry night background and comet features are inspired from and referenced to two Shadertoy posts I linked in the helplog. In addition, the light direction and color in the lambert shader that we use to render our terrain also match the sky's sun and color in our skybox shader. I use a range of hard-coded intervals with unique dusk and sunset colors provided to interpolate between them as time advances. To make things look more realistic, I also blend in the daylight color and dusk color to the terrain texturing. 

	Since the procedural skybox is heavily dependent upon noise functions, the biggest challenge for me was to tweak around the LERP factors to create a smooth sunrise or sunset effect near the horizon such that the sky changes color near the sun, but should be dusky-colored farther away from the sun. In the demo code, the dot product method that indirectly implies the angle in between our ray direction and the sunlight direction is not quite intuitive to me. Thus, I changed to use the y and z coordinates of our current sun position to generate the procedural sky color. I spent forever adjusting the y coord thresholds and color mix for sunrise and sunset, but still can't be satisfied. The output is now okay, not to my standard perfect.

Linda - Distance Fog:

	I used a simplied fog model, which is essentially a linear function of Euclidean distance. In other words, the fog factor increases linearly with the distance from the current camera view. The lambert fragment shader that we use to render out terrain is where we introduce color changes in order to imitate foggy environment. I calculate the distance between the camera’s eye and the vertex to assign a color mix of the hard-coded fog color and the distant sky background.The logic is very easy to understand, but the result effecs are conving enough.

Linda - Shadow Mapping:

	The main concept of shadow mapping is from opengl-tutorial 16. First, I render my entire world using an orthographic camera oriented along the direction from which virtual light comes. Notably, this render pass is saved to a texture buffer on the GPU, and only saves the depth information of each fragment in screen space. Using this 2D array of depths, I then determine whether or not each fragment seen by the main viewing camera in the lambert shader is in shadow or in light. To implement this logic, I created a separate frame buffer and a shader program just to render a depth texture for sampling later. I only passed in the position data as our VBO data to the first render pass. After the depth texture is created (we can't visually see an output from this but we could add an extra step or use RenderDoc tool to generate the actual depth image) and stored in texture slot 2, apart from the main scene and post-processing slots, I sample the light depth from the shadow map as a Sampler2DShadow texture in the lambert fragment shader to compare the retreived light depth to the z coordinate of each pixel, and assign a visibility scalar to be multiplied to the current pixel color.

	It was in the first place difficult to understand the whole process/ sequence of shadow mapping steps. I went in deep reading through the code provided in the opengl-tutorial website and found it well-explained and very helpful to follow. The next difficult was to make the shadow show up in the scene after I set up the least required frame buffer and shader program. With Adam's help, I realized that when calculating the light source MVP, I was using a viewing frustum scaled too small (both in terms of width, height and depth) to include the entire world. My light source direction was also not "dramatic" enough to first create a prominent appearance of shaodw. I fixed the matries and gladly the shadow at least was drawn. Lastly, our shadow was quite coarse, shaky as it moves with jaggy sharp edges. A TA suggest me look at the improvement tips on opengl-tutorial. After a few experiments attempting at the optimization tricks, I ended up using the poisson sampling and bias to blur out the sharp shadow edge, and increse the shadow map texture resolution in the first render pass. Now the overall look is much better but the close-up still reveals the underlying zigzag edge issues. Due to the time limit, this is the most effort I could put into refinement and we are all satisfied at the results.


## Milestone-2

**Features & Distribution**

	Linda Zhu - Procedural generation of cave systems using 3D noises
	Runjie Zhao - Texturing and texture animation in OpenGL
	Tianyi Xiao - Multithreading of terrain loading

**Implementation Details**

Linda - Cave System and Post-processing:

	The cave generation was fairly straightforward. It was understanding how 3D Perlin noise works that took me a while. I implemented the 3D Perlin noise from the Noise Functions slides. Then I looped through blocks between y = 1 and y = 128 to determine the block type using 3D Perlin noise.
	For cave system, I also added another three BlockTypes, namely CAVE, LAVA, and BEDROCK and worked with my partner to add in texture sampling for these new blocks.

	The tricky parts of my features are the new player physics when in liquid, and setting up the post-processing pipeline on top of the existing surface shader. For unblocking player in water or lava, I created a helper set of movableBlocks and a helper set of collidableBlocks for collision detection. All the opaque blocktypes are in these two sets. I then replaced the old condition in the player collision
	if-statement with these sets simply because it's easier to read and modify in the future if we add more blocktypes. For player movement in liquid, I added conditional checking in player::processInputs. Basically if player's position is a liquid BlockType, make player's velocity (in all six directions) 2/3 of the original.

	For post-processing effect when player is in lava or water, I added a 30% opaque red or blue overlay onto the rendered texture. Setting up the post-processing pipeline and debugging it was the most difficult and confusing part of this milestone. I referred to hw4-shaderfun for adding post-processing.


## Milestone-1

**Features & Distribution**

	Tianyi Xiao - Procedural generation of terrain using noise functions.
	Linda Zhu - Altering the provided Terrain system to efficiently store and render blocks.
	Runjie Zhao - Constructing a controllable Player class with simple physics.

**Implementation Details**

Linda - Efficient Chunk Rendering:

	I mainly followed the instructions for chunkVBOS, i.e. rendering the "shell" of a terrain rather than drawing every cube, using an interleaved VBO to pass vertex data to the GPU, and enabling terrain expansion.
	For Chunk's create() function, I checked the adjacent blocks around a block to see if I need to render their boundary face. If a block is around the edge of a chunk, I check the adjacent block in the neighboring chunks.
	For the interleaved VBO, I created new handles and shaderProgram bind functions for this new big VBO buffer. The most important and tricy part is to bind the buffer and tell OpenGL how to interpret the buffer data using
	the concept of stride in draw(). My VBO data is a collection of vec4s composed of position1|normal1|color1|position2|normal2|color2.... The stride is the start point in this array I extract the corresponding info and pass it
        to the appropriate "in" variables in the shader. Finally, I implemented terrain expansion by checking if a player is within 16 blocks away from an edge of the current rendering boundary. To do this, I looped around an array
	of the +x, -x, +z, -z, and the diagonal directions, namely +x+z, +x-z, -x+z, and -x-z, of the current chunk the player is on. If there's no exisiting neighboring chunk at a certain direction, I call the procedural terrain
	generation and populate the existing VBO for rendering.
	In addition, for general performance improvement, I tried to flag the chunks that either have VBO data created or already buffered to GPU, to avoid duplicate VBO creation. Instead of calling shaderProgram::setModelMatrix
	in the loop for each chunk rendering, I buffered the vertex position in global space directly because the less frequent and fewer data we pass to the GPU, the better.

	Difficult 1: After I implemented my chunkVBOs, I was not able to render the original 64x64 blocks = 4x4 chunks as a complete plane. The 4x4 chunks were always some distance apart from each other at the same level. I tried to
	adjust my model matrix, i.e. translating the chunk by certain manipulations of x and z, but not fixing anything. I focused solely on vertex positions/ chunk location and transformation, overlooking the scale/ dimension of each
	chunk until TA Lorant helped me look into it with a different perspective.
	Solution: It was me adding two vec4s with both w=1, resulting in a w=2, when passing the vertex position into the buffer, that causes the vs_Pos in the vertex shader half of the original value. During the homogenized step in OpenGL,
	all the positions are divided by w (which was 2 in my case), so the result will be half size smaller. Simply changing one of the position vec4's w to be zero fixes this problem.
