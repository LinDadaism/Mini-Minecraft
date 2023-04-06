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

***Runjie Zhao***

Fluid Simulation:

	Press add block button to add lava which will form a fluid simulation.
	
	In order to do fluid simulation, I create 8 more blocks which represent 8 directions. Since one block is hard to identify the directions which I will use in computephysics part, 8 blocks is suitable for this effect. 
	
	The problem is that it is hard to render by time. For example, if a water block is in the air, then at the first second, there should be only one block, then in the second there should be eight blocks in eight direction and so on. So I render all of them together at once.

Sound:
	
	In order to achieve the effects, I create QSoundEffect variables in player.cpp file. In default, the player will hear wind and bird sound anywhere. When the player is not in the fly mode and the player is moving, the user will hear walk sound. When the user approaches the water and lava, they will hear water and lava sound.
	
	One problem that I meet is that sometimes, there is no sound. This should be QSoundEffect problem, in order to deal with this issue, the user needs to turn off the computer's audio and then turn it on again.
	
Water Waves:
	
	In order to achieve water waves effects, I modified the vertex shader glsl and change the position of the vertex in it by applying sin function. In order to make the water waves smooth, I can combine 4 continuous water blocks in the same group.
	
Post-process Camera Overlay:

	In order to distort the view under water and lava, I applied noise function according to the time in the post shader to change their uv position.


***Tianyi Xiao:***

Additional Biomes:

	Using perlin noise functions with different configurations, 2 properties, moisture and temperature, are generated. Then according to these two properties, four different biomes: grassland, mountain, snowland, desert are generated. To make them different, mountain area is steeper than other biomes.

L-system River:

	In the river implementation, we calculate river using L-system, so it's calculated iteratively. In my algorithm, there is a vector to store pairs of beginning position and direction of the new river part. Then in each iteration, every pair will be expand to a part of the whole river with fork, also new pairs of beginning positions and direction will also be generated at the same time.

	To accomodate multithread, in the beginning, the algorithm will first generate some river to cover the beginning area. Then when player move further and enter a new area, river in this area would be generated accordingly. To implement such kind of feature, an unordered_map is used to store the beginning pair of river parts, according to position of the chunk it belongs to.

	And the generated rivers information are stored as lines, with begin position and end position. Then in terrain generation algorithm, after biomes are generated at a given horizontal position, we will check how far is it to the nearest river line. If it's near enough, the ground will carve according to its distance to the river line, and water would be filled.

Procedurally Placed Assets:

	In each chunk, we will randomly pick up a point to generate the placed assets. There are two kinds of assets, one is small asset like flower and mushroom. The other one is bigger asset like a tree.

	Small asset only take one block, so in fact, each small asset is a block. To make it look like flower, we cancel its top and bottom face, and set other 4 faces to the center of block, to make it look like a 3D object. And we discard the fragment with 0 alpha value in texture to avoid wired bugs of blocking other blocks in the wrong way.

	The tree would take 5x5 area. To avoid unnecesssary effort, faster speed, and accomodate multithread, I decide to let the tree generate only within a chunk.

Procedural buildings with shape grammars:

	I also build a large tree-like-building on the map, which is generated using L-system. Basically, this process is similar to the L-system river. So I won't duplicate those words one more time. The main difference between this and river is that, the direction of lines need to be 3D but not only in XZ directions.


***Linda Zhu:***

Procedural Sky Box:

	I built up from the given demo code. The main idea is to build a procedural sky background using raycasting from the camera to the pixel to decide which color we should color it in. With a dedicated shaderprogram, I draw the skybox on the quadrangle that represents the current scene, and overlay the terrain texture/ post-processing render on top of it. While the skybox vertex shader simplying pass through the position data, the fragment shader does all of the visual manipulation work. In the fragment shader, I modified the position of the sun over time using some variation of cos(time) so that the sun moves around the center of the scene in the y-z plane. I blended in the sky's color as the sun moves from a morning sunrise to afternoon overhead to sunset to night-time. During daylight time, I added animated perlin-noise clouds. During night time, I added a slowly rotating starry background field around the screen center by layering noise points that mimics stars with various sizes. I also added shooting stars with random chances of appearance in the night sky to create a mysterious vitality of the scene. The starry night background and comet features are inspired from and referenced to two Shadertoy posts I linked in the helplog. In addition, the light direction and color in the lambert shader that we use to render our terrain also match the sky's sun and color in our skybox shader. I use a range of hard-coded intervals with unique dusk and sunset colors provided to interpolate between them as time advances. To make things look more realistic, I also blend in the daylight color and dusk color to the terrain texturing. 

	Since the procedural skybox is heavily dependent upon noise functions, the biggest challenge for me was to tweak around the LERP factors to create a smooth sunrise or sunset effect near the horizon such that the sky changes color near the sun, but should be dusky-colored farther away from the sun. In the demo code, the dot product method that indirectly implies the angle in between our ray direction and the sunlight direction is not quite intuitive to me. Thus, I changed to use the y and z coordinates of our current sun position to generate the procedural sky color. I spent forever adjusting the y coord thresholds and color mix for sunrise and sunset, but still can't be satisfied. The output is now okay, not to my standard perfect.

Distance Fog:

	I used a simplied fog model, which is essentially a linear function of Euclidean distance. In other words, the fog factor increases linearly with the distance from the current camera view. The lambert fragment shader that we use to render out terrain is where we introduce color changes in order to imitate foggy environment. I calculate the distance between the camera’s eye and the vertex to assign a color mix of the hard-coded fog color and the distant sky background.The logic is very easy to understand, but the result effecs are conving enough.

Shadow Mapping:

	The main concept of shadow mapping is from opengl-tutorial 16. First, I render my entire world using an orthographic camera oriented along the direction from which virtual light comes. Notably, this render pass is saved to a texture buffer on the GPU, and only saves the depth information of each fragment in screen space. Using this 2D array of depths, I then determine whether or not each fragment seen by the main viewing camera in the lambert shader is in shadow or in light. To implement this logic, I created a separate frame buffer and a shader program just to render a depth texture for sampling later. I only passed in the position data as our VBO data to the first render pass. After the depth texture is created (we can't visually see an output from this but we could add an extra step or use RenderDoc tool to generate the actual depth image) and stored in texture slot 2, apart from the main scene and post-processing slots, I sample the light depth from the shadow map as a Sampler2DShadow texture in the lambert fragment shader to compare the retreived light depth to the z coordinate of each pixel, and assign a visibility scalar to be multiplied to the current pixel color.

	It was in the first place difficult to understand the whole process/ sequence of shadow mapping steps. I went in deep reading through the code provided in the opengl-tutorial website and found it well-explained and very helpful to follow. The next difficult was to make the shadow show up in the scene after I set up the least required frame buffer and shader program. With Adam's help, I realized that when calculating the light source MVP, I was using a viewing frustum scaled too small (both in terms of width, height and depth) to include the entire world. My light source direction was also not "dramatic" enough to first create a prominent appearance of shaodw. I fixed the matries and gladly the shadow at least was drawn. Lastly, our shadow was quite coarse, shaky as it moves with jaggy sharp edges. A TA suggest me look at the improvement tips on opengl-tutorial. After a few experiments attempting at the optimization tricks, I ended up using the poisson sampling and bias to blur out the sharp shadow edge, and increse the shadow map texture resolution in the first render pass. Now the overall look is much better but the close-up still reveals the underlying zigzag edge issues. Due to the time limit, this is the most effort I could put into refinement and we are all satisfied at the results.


## Milestone-2

**Features & Distribution**

	Linda Zhu - Procedural generation of cave systems using 3D noises
	Runjie Zhao - Texturing and texture animation in OpenGL
	Tianyi Xiao - Multithreading of terrain loading

**Implementation Details**

Multithreaded Terrain Generation:

	Initially, I try to use std::thread, but finally, considering the performance, I switch from std::thread into QThread, though both version could work out.

	And basically, I followed the instruction in project requirement. So I divide all the work into two workers: BlockTypeWorker and VBOWorker. In BlockTypeWorker, given the x and z position of chunks in the zone, it will decide what blocks should have in the zone, according to the set of previous noise functions in milestone-1.

	Then after the chunks are created, they will be passed into VBOWorker, where their VBO information would be generated. Then finally, after the child thread ends for this chunk, the chunk's information about renderring would be sent to GPU on main thread.

Texture and Texture Animation in OpenGL:

	At first, I create texture.cpp and texture.h files, where I can load texture picture, create texture, load texture, bind texture.

	Then, I modify the shaderprogram.cpp, chunk.cpp and drawable.cpp to connect CPU with GPU. After that, I modify the vertex shader file to change the UV position based on the time to create animation for water and lava. In order to make water and ice transparent, I create two for loops to create the water. By doing so, I can sort the rendered triangles according to their alpha values.

	Difficult 1: When I am trying to create the transparent blocks, I found out that the sky color is under my water. However, under Professor Mally's help, I use two for loops to draw transparent blocks and non transparent blocks separately rather than draw one non transparent, one transparent and one non transparent.

Cave System and Post-processing:

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

Procedural Terrain:

	I use a perlin noise function to decide mountain noise value, and perturbed fbm & worley noise to decide grassland noise value. For the detail of grassland value, I lerp between fbm and worley noise.
	Then, I use another smoothsteped perlin noise to generate a biome noise value, and interpolate between the biomes. If this noise value is bigger than 0.5, then this place is mountain biome, otherwise it's grassland biomme.
	Also, the final height comes from lerpping in mountain noise value and grassland noise value, where u is the biome noise value.

	Difficult 1: The performance of noise function is different between hlsl version used in openGLFun and my C++ version used now.
	Solution: One reason is the input changed from [0, 1] to about several hundered. So I need to test for different configs for better performance.
		   And with minus input value, the execution of some std function is different from what I expect.

Efficient Chunk Rendering:

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

Game Engine Tick Function and Player Physics：

	I divide two modes for players one is fly mode while another one is normal mode. I apply friction for both modes where I set k to be 0.02f. I set deceleration for both modes and the fly mode's deceleration is more obvious since the deceleration speed is larger. Then, I did collision part checking if vertices collide with the wall and change its speed such that it won't collide to the wall(which means that when my velocity is really large and I cast rays on each vertex and do a for loop to change my speed rather than setting the speed to 0 directly). Then, I did block part and cast a ray to find where I should place the block. I set the limitation for ray to avoid place the player inside the block. When player looks down where forward's y axis is between -0.7 and -1.0, the ray's length must be larger than 1.5 to avoid place the player inside the ground. Other than that, the ray's length must be larger than 1.

	Difficult 1: When I tried to collide the player with wall diagonally, it directly pass throught the wall.
	Solution: The vertices do not contact with the wall. So I use the center position of the block to check if they collide or not and this works.

	Difficult 2: When the player is spawn, its position is actually in a block
	Solution: Change mygl.cpp to make player's spawn position a little bit higher.

	Difficult 3: Qt does not allow two buttons to be clicked at the same time(such as w and spacebar) and we can not move while jump by pushing w and spacebar button;
	Solution: Setting the jump height higher and it can leave some times for the player to accelerate and then move.
