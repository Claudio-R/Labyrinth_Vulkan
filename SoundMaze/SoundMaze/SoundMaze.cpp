/*
DA FIXARE VERT0 E FRAG0 PROBABILMENTE
*/

#include "SoundMaze.hpp"

const std::string MODEL_PATH_MAZE = "models/maze.obj";
const std::string TEXTURE_PATH_MAZE = "textures/maze_rock.jpg";
const std::string MODEL_PATH_TREASURE = "models/icosphere.obj";
const std::string TEXTURE_PATH_TREASURE = "textures/maze_rock.jpg";
const int N = 4;

/*
SETS
set 0: Camera and Environment;
set 1: Objects
*/
struct UniformBufferObject_s0 {
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

struct UniformBufferObject_p0_s1 {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 lightPos; // 4 treasures, each column contains the position of a light
	alignas(16) glm::mat4 lightColors;
	alignas(16) glm::vec3 eyePos;
	alignas(16) glm::vec4 decay;
};

struct UniformBufferObject_p1_s1 {
	alignas(16) glm::mat4 model;
	alignas(16) glm::vec4 lightColor;
};
 
class SoundMaze : public BaseProject {
	protected:
	
	// Pipelines [Shader couples]
	Pipeline P0; // MAZE
	Pipeline P1; // TREASURES

	// Descriptor Layouts [what will be passed to the shaders]
	DescriptorSetLayout DSL_s0;
	DescriptorSetLayout DSL_p0_s1;
	DescriptorSetLayout DSL_p1_s1;

	// Models, textures and Descriptors (values assigned to the uniforms)
	DescriptorSet DS_s0;

	Model M1;
	Texture T1;
	DescriptorSet DS_p0_s1;

	Model M2;
	Texture T2;
	DescriptorSet DS_p1_s1[N];
	
	void setWindowParameters() {	
		windowWidth = 1200;
		windowHeight = 800;
		windowTitle = "Inside the Sound Maze...";
		initialBackgroundColor = {0.0f, 0.0f, 0.0f, 1.0f};
		
		uniformBlocksInPool = 2 + N;
		texturesInPool = 1 + N;
		setsInPool = 2 + N;
	}
	
	void localInit() {
	
		/* DESCRIPTOR SET LAYOUTS */
		DSL_s0.init(this, {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
			});

		DSL_p0_s1.init(this, {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
			{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
			});

		DSL_p1_s1.init(this, {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
			{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
			});

		/* PIPELINE */
		P0.init(this, "shaders/vert0.spv", "shaders/frag0.spv", { &DSL_s0, &DSL_p0_s1 });
		P1.init(this, "shaders/vert1.spv", "shaders/frag1.spv", { &DSL_s0, &DSL_p1_s1 });

		// MODEL, TEXTURES, DESCRIPTORE SETS
		/* GLOBAL */
		DS_s0.init(this, &DSL_s0, {
			{0, UNIFORM, sizeof(UniformBufferObject_s0), nullptr}
			});
		
		/* MAZE */
		M1.init(this, MODEL_PATH_MAZE);
		T1.init(this, TEXTURE_PATH_MAZE);
		DS_p0_s1.init(this, &DSL_p0_s1, {
			{0, UNIFORM, sizeof(UniformBufferObject_p0_s1), nullptr},
			{1, TEXTURE, 0, &T1}
			});
		
		/* TREASURES */
		M2.init(this, MODEL_PATH_TREASURE);
		T2.init(this, TEXTURE_PATH_TREASURE);
		for (int i = 0; i < N; i++) {
			DS_p1_s1[i].init(this, &DSL_p1_s1, {
				{0, UNIFORM, sizeof(UniformBufferObject_p1_s1), nullptr},
				{1, TEXTURE, 0, &T2}
				});
		}

	}
		
	void localCleanup() {
		DS_s0.cleanup();
		
		DS_p0_s1.cleanup();
		T1.cleanup();
		M1.cleanup();

		for (int i = 0; i < N; i++) {
			DS_p1_s1[i].cleanup();
		}

		T2.cleanup();
		M2.cleanup();

		P0.cleanup();
		P1.cleanup();
		DSL_s0.cleanup();
		DSL_p0_s1.cleanup();
		DSL_p1_s1.cleanup();
	}
	
	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {
		
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, P0.graphicsPipeline);
		
		/* GLOBAL */
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, P0.pipelineLayout, 0, 1, &DS_s0.descriptorSets[currentImage], 0, nullptr);

		/* MAZE */
		VkBuffer vertexBuffers_maze[] = { M1.vertexBuffer };
		VkDeviceSize offsets_maze[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers_maze, offsets_maze);
		vkCmdBindIndexBuffer(commandBuffer, M1.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			P0.pipelineLayout, 1, 1, &DS_p0_s1.descriptorSets[currentImage], 0, nullptr);
		vkCmdDrawIndexed(commandBuffer,
			static_cast<uint32_t>(M1.indices.size()), 1, 0, 0, 0);

		/* TREASURES */
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, P1.graphicsPipeline);
		VkBuffer* vertexBuffers_T[N];
		for (int i = 0; i < N; i++) {
			VkBuffer vertexBuffers[] = {M2.vertexBuffer};
			vertexBuffers_T[i] = vertexBuffers;
			VkDeviceSize offsets_treasure1[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers_T[i], offsets_treasure1);
			vkCmdBindIndexBuffer(commandBuffer, M2.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				P1.pipelineLayout, 1, 1, &DS_p1_s1[i].descriptorSets[currentImage], 0, nullptr);
			vkCmdDrawIndexed(commandBuffer,
				static_cast<uint32_t>(M2.indices.size()), 1, 0, 0, 0);
		
		}
	
	}

	void updateCameraAngles(glm::vec3 *CamAng, float deltaT, float ROT_SPEED) {
		static constexpr float MAX_UP_ANGLE = glm::radians(70.0f);
		static constexpr float MAX_DOWN_ANGLE = glm::radians(-70.0f);

		if (glfwGetKey(window, GLFW_KEY_LEFT)) {
			CamAng->y += deltaT * 2 * ROT_SPEED;

		}
		if (glfwGetKey(window, GLFW_KEY_RIGHT)) {
			CamAng->y -= deltaT * 2 * ROT_SPEED;
		}
		if (glfwGetKey(window, GLFW_KEY_UP)) {
			if (CamAng->x < MAX_UP_ANGLE) {
				CamAng->x += deltaT * ROT_SPEED;
			}
		}
		if (glfwGetKey(window, GLFW_KEY_DOWN)) {
			if (CamAng->x > MAX_DOWN_ANGLE) {
				CamAng->x -= deltaT * ROT_SPEED;
			}
		}
	}

	void updateCameraPosition(glm::vec3* CamPos, float deltaT, float MOVE_SPEED, glm::vec3 CamAng) {
		if (glfwGetKey(window, GLFW_KEY_A)) {
			*CamPos -= MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), CamAng.y,
				glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(1, 0, 0, 1)) * deltaT;
		}
		if (glfwGetKey(window, GLFW_KEY_D)) {
			*CamPos += MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), CamAng.y,
				glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(1, 0, 0, 1)) * deltaT;
		}
		if (glfwGetKey(window, GLFW_KEY_S)) {
			*CamPos += MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), CamAng.y,
				glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 0, 1, 1)) * deltaT;
		}
		if (glfwGetKey(window, GLFW_KEY_W)) {
			*CamPos -= MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), CamAng.y,
				glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 0, 1, 1)) * deltaT;
		}
	}

	glm::vec3 setUBO_0(UniformBufferObject_s0* ubo_set0, float deltaT) {

		static constexpr float ROT_SPEED = glm::radians(60.0f);
		static const float MOVE_SPEED = 0.5f;
		static const float MOUSE_RES = 500.0f;

		static glm::vec3 CamAng = glm::vec3(0.0f);
		static glm::vec3 CamPos = glm::vec3(0.0f, 0.1f, 0.0f);
		static double old_xpos = 0, old_ypos = 0;
		double xpos, ypos;

		glfwGetCursorPos(window, &xpos, &ypos);
		double m_dx = xpos - old_xpos;
		double m_dy = ypos - old_ypos;
		old_xpos = xpos; old_ypos = ypos;

		glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);

		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
			CamAng.y += m_dx * ROT_SPEED / MOUSE_RES;
			CamAng.x += m_dy * ROT_SPEED / MOUSE_RES;
		}

		updateCameraAngles(&CamAng, deltaT, ROT_SPEED);

		glm::mat3 CamDir = glm::mat3(glm::rotate(glm::mat4(1.0f), CamAng.y, glm::vec3(0.0f, 1.0f, 0.0f))) *
			glm::mat3(glm::rotate(glm::mat4(1.0f), CamAng.x, glm::vec3(1.0f, 0.0f, 0.0f))) *
			glm::mat3(glm::rotate(glm::mat4(1.0f), CamAng.z, glm::vec3(0.0f, 0.0f, 1.0f)));

		updateCameraPosition(&CamPos, deltaT, MOVE_SPEED, CamAng);

		glm::mat4 CamMat = glm::translate(glm::transpose(glm::mat4(CamDir)), -CamPos);
		ubo_set0->view = CamMat;

		constexpr float FOVY = glm::radians(60.0f);
		const float NEAR_FIELD = 0.001f;
		const float FAR_FIELD = 5.0f;

		ubo_set0->proj = glm::perspective(FOVY, swapChainExtent.width / (float)swapChainExtent.height, NEAR_FIELD, FAR_FIELD);
		ubo_set0->proj[1][1] *= -1;

		return CamPos;

	}

	void updateUniformBuffer(uint32_t currentImage) {

		static auto startTime = std::chrono::high_resolution_clock::now();
		static float lastTime = 0.0f;

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>
			(currentTime - startTime).count();
		float deltaT = time - lastTime;
		lastTime = time;

		void* data;

		UniformBufferObject_s0 ubo_set0{}; /* GLOBAL */
		glm::vec3 CamPos = setUBO_0(&ubo_set0, deltaT);
		vkMapMemory(device, DS_s0.uniformBuffersMemory[0][currentImage], 0, sizeof(ubo_set0), 0, &data);
		memcpy(data, &ubo_set0, sizeof(ubo_set0));
		vkUnmapMemory(device, DS_s0.uniformBuffersMemory[0][currentImage]);
				
		/* positions are stored in columns */
		static glm::mat4 lightPos = glm::mat4(
			glm::vec4(0.2f, 0.5f, -0.2f, 1.0f), 
			glm::vec4(-0.2f, 0.5f, -0.2f, 1.0f),
			glm::vec4(-0.2f, 0.5f, 0.2f, 1.0f),
			glm::vec4(0.2f, 0.5f, 0.2f, 1.0f)
		);

		/* colors are stored in columns */
		static glm::mat4 lightColors = glm::mat4(
			glm::vec4(0.8f, 0.2f, 0.2f, 1.0f),
			glm::vec4(0.2f, 0.8f, 0.2f, 1.0f),
			glm::vec4(0.2f, 0.2f, 0.8f, 1.0f),
			glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)
		);

		/* MAZE */
		UniformBufferObject_p0_s1 ubo_p0_s1{};
		ubo_p0_s1.lightPos = lightPos;
		ubo_p0_s1.lightColors = lightColors;
		ubo_p0_s1.model = glm::mat4(1.0f);
		ubo_p0_s1.eyePos = CamPos;
		ubo_p0_s1.decay = glm::vec4(0.9f, 0.92f, 2.0f, 2.0f);

		vkMapMemory(device, DS_p0_s1.uniformBuffersMemory[0][currentImage], 0, sizeof(ubo_p0_s1), 0, &data);
		memcpy(data, &ubo_p0_s1, sizeof(ubo_p0_s1));
		vkUnmapMemory(device, DS_p0_s1.uniformBuffersMemory[0][currentImage]);

		UniformBufferObject_p1_s1 ubo_p1_s1{}; /* TREASURES */

		/*static const float MIN_HEIGHT = 0.1f;*/
		for (int i = 0; i < N; i++) {
			ubo_p1_s1.model = glm::translate(glm::mat4(1.0f), glm::vec3(lightPos[i])) *
				glm::rotate(glm::mat4(1.0f),
					time * glm::radians(70.0f + 5.0f * i),
					glm::vec3(0.0f, 1.0f, 0.0f));
			ubo_p1_s1.lightColor = lightColors[i];

			vkMapMemory(device, DS_p1_s1[i].uniformBuffersMemory[0][currentImage], 0, sizeof(ubo_p1_s1), 0, &data);
			memcpy(data, &ubo_p1_s1, sizeof(ubo_p1_s1));
			vkUnmapMemory(device, DS_p1_s1[i].uniformBuffersMemory[0][currentImage]);
		}
	}	
};

int main() {
    SoundMaze app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
