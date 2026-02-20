#define STB_IMAGE_IMPLEMENTATION  
#include <stb_image.h>

#include "cube_renderer.h"

struct VoxelVertex {
	glm::vec3 pos;
	glm::vec2 uv;
	glm::vec3 normal;
};


	
	cube_renderer::cube_renderer()
		: ourShader{ "normals.vs", "normals.fs" }
	{

		

		model = new float[288] {

			// Vertex Pos       Tex Coord     Normal
			0.5f, 0.5f, 0.5f,       0.5f, 0.66667f,   0.f, 0.f, 1.f,
				0.5f, -0.5f, 0.5f,   .5f, 0.33333f,   0.f, 0.f, 1.f,
				-0.5f, -0.5f, 0.5f,  0.25f, 0.33333f, 0.f, 0.f, 1.f,               // front
				-0.5f, -0.5f, 0.5f, 0.25f, 0.33333f,  0.f, 0.f, 1.f,
				-0.5f, 0.5f, 0.5f,  0.25f, 0.66667f,  0.f, 0.f, 1.f,
				0.5f, 0.5f, 0.5f,  .5f, .66667f,      0.f, 0.f, 1.f,

				0.5f, 0.5f, -0.5f,   0.5f, 0.66667f,  0.f, 0.f, -1.f,
				-0.5f, 0.5f, -0.5f, 0.25f, 0.66667f,   0.f, 0.f, -1.f,
				-0.5f, -0.5f, -0.5f,  0.25f, 0.33333f, 0.f, 0.f, -1.f,       // back
				-0.5f, -0.5f, -0.5f, 0.25f, 0.33333f, 0.f, 0.f, -1.f,
				0.5f, -0.5f, -0.5f,  0.5f, 0.33333f, 0.0f, 0.f, -1.f,
				0.5f, 0.5f, -0.5f,  .5f, .66667f,     0.f, 0.f, -1.f,

				-0.5f, 0.5f, 0.5f,   0.5f, 0.66667f,    -1.f, 0.f, 0.f,
				-0.5f, -0.5f, 0.5f,   .5f, 0.33333f,    -1.f, 0.f, 0.f,
				-0.5f, -0.5f, -0.5f,  0.25f, 0.33333f,  -1.f, 0.f, 0.f,                 // left
				-0.5f, -0.5f, -0.5f, 0.25f, 0.33333f,   -1.f, 0.f, 0.f,
				-0.5f, 0.5f, -0.5f,  0.25f, 0.66667f,   -1.f, 0.f, 0.f,
				-0.5f, 0.5f, 0.5f,  .5f, .66667f,       -1.f, 0.f, 0.f,

				0.5f, 0.5f, -0.5f,  0.5f, 0.66667f,   1.f, 0.f, 0.f,
				0.5f, -0.5f, -0.5f,  .5f, 0.33333f,   1.f, 0.f, 0.f,
				0.5f, -0.5f, 0.5f,   0.25f, 0.33333f, 1.f, 0.f, 0.f,   // right
				0.5f, -0.5f, 0.5f,  0.25f, 0.33333f,  1.f, 0.f, 0.f,
				0.5f, 0.5f, 0.5f,   0.25f, 0.66667f,  1.f, 0.f, 0.f,
				0.5f, 0.5f, -0.5f, .5f, .66667f,      1.f, 0.f, 0.f,

				0.5f, 0.5f, -0.5f,   0.5f, .99f,         0.f, 1.f, 0.f,
				0.5f, 0.5f, 0.5f,     .5f, 0.66667f,   0.f, 1.f, 0.f,
				-0.5f, 0.5f, 0.5f,    0.25f, 0.66667f, 0.f, 1.f, 0.f,     // top
				-0.5f, 0.5f, 0.5f,   0.25f, 0.66667f,  0.f, 1.f, 0.f,
				-0.5f, 0.5f, -0.5f,  0.25f, .99f,       0.f, 1.f, 0.f,
				0.5f, 0.5f, -0.5f,  .5f, .99f,          0.f, 1.f, 0.f,

				0.5f, -0.5f, 0.5f,    0.49f, 0.33333f,        0.f, -1.f, 0.f,
				0.5f, -0.5f, -0.5f,    .49f, 0.1f,   0.f, -1.f, 0.f,
				-0.5f, -0.5f, -0.5f,   0.26f, 0.1f, 0.f, -1.f, 0.f,        // bottom
				-0.5f, -0.5f, -0.5f,  0.26f, 0.1f,  0.f, -1.f, 0.f,
				-0.5f, -0.5f, 0.5f,   0.26f, 0.33333f,       0.f, -1.f, 0.f,
				0.5f, -0.5f, 0.5f,   .49f, 0.33333f,          0.f, -1.f, 0.f,
			};


	
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);

		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		static constexpr GLsizeiptr kFloatCount = 36 * 8;
		glBufferData(GL_ARRAY_BUFFER, kFloatCount * sizeof(float), model, GL_STATIC_DRAW);

		// position attribute
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		// texture coord attribute
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);
		// Normal attribute
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
		glEnableVertexAttribArray(2);



		glGenTextures(1, &texture1);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture1);
		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		// load image, create texture and generate mipmaps
		int width, height, nrChannels;
		stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
		unsigned char* data = stbi_load(std::string("assets/textures/voxel_cube_1.png").c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		else
		{
			std::cout << "Failed to load texture" << std::endl;
		}
		stbi_image_free(data);

		glGenTextures(1, &texture2);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, texture2);
		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		// load image, create texture and generate mipmaps
		data = stbi_load(std::string("assets/textures/light512.png").c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			// note that the awesomeface.png has transparency and thus an alpha channel, so make sure to tell OpenGL the data type is of GL_RGBA
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		else
		{
			std::cout << "Failed to load texture" << std::endl;
		}
		stbi_image_free(data);


		glBindVertexArray(0);

	}

	cube_renderer::~cube_renderer() {
		if (model)
		{
			delete[] model;
		}

		// ------------------------------------------------------------------------
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);

	}

	void cube_renderer::Draw_Cube(glm::mat4& projection,  glm::mat4& view, Camera& camera, int SCR_WIDTH, int SCR_HEIGHT)
	{

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture1);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, texture2);
		glBindVertexArray(VAO);
		ourShader.use();
		ourShader.setInt("texture1", 0);
		ourShader.setInt("texture2", 1);

		// pass projection matrix to shader (note that in this case it could change every frame)
		//glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		ourShader.setMat4("projection", projection);

		// camera/view transformation
		//glm::mat4 view = camera.GetViewMatrix();
		ourShader.setMat4("view", view);


		// calculate the model matrix for each object and pass it to shader before drawing
		glm::mat4 model = glm::mat4(1.f); // make sure to initialize matrix to identity matrix first
		model = glm::translate(model, trns);
		
		//model = glm::rotate(model, glm::radians(rotX), glm::vec3(1.0f, 0.f, 0.f));
		//model = glm::rotate(model, glm::radians(rotY), glm::vec3(0.0f, 1.f, 0.f));
		//model = glm::rotate(model, glm::radians(rotZ), glm::vec3(0.0f, 0.f, 1.f));

		ourShader.setMat4("model", model);

		glDrawArrays(GL_TRIANGLES, 0, 36);

		glBindVertexArray(0);

	}
	
