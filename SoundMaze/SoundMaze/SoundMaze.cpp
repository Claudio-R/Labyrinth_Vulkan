#include "SoundMaze.hpp"

const std::string MODEL_PATH_MAZE = "models/maze.obj";
const std::string TEXTURE_PATH_MAZE = "textures/maze_rock.jpg";
const std::string MODEL_PATH_TREASURE = "models/cube.obj";
const std::string TEXTURE_PATH_TREASURE = "textures/maze_rock.jpg";
const int N = 10;

/*
SETS
set 0: Camera and Environment;
set 1: Objects
*/
struct UniformBufferObject_set0 {
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};
struct UniformBufferObject_set1 {
	alignas(16) glm::mat4 model;
};
 
class SoundMaze : public BaseProject {
	protected:
	
	// Pipelines [Shader couples]
	Pipeline P1;

	// Descriptor Layouts [what will be passed to the shaders]
	/* one DSL for each set */
	DescriptorSetLayout DSL0;
	DescriptorSetLayout DSL1;

	// Models, textures and Descriptors (values assigned to the uniforms)
	DescriptorSet DS0;

	Model M1;
	Texture T1;
	DescriptorSet DS1;

	Model M2;
	Texture T2;
	DescriptorSet DST[N];
	
	void setWindowParameters() {
		// window size, title and initial background
		windowWidth = 1200;
		windowHeight = 800;
		windowTitle = "Inside the Sound Maze...";
		initialBackgroundColor = {0.0f, 0.0f, 0.0f, 1.0f};
		// Descriptor pool sizes
		uniformBlocksInPool = 2 + N;
		texturesInPool = 1 + N;
		setsInPool = 2 + N;
	}
	
	void localInit() {
	
		/* DESCRIPTOR SET LAYOUTS */
		DSL0.init(this, {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
			});

		DSL1.init(this, {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
			{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
			});

		/* PIPELINE */
		P1.init(this, "shaders/vert.spv", "shaders/frag.spv", {&DSL0, &DSL1});

		// MODEL, TEXTURES, DESCRIPTORE SETS
		/* GLOBAL */
		DS0.init(this, &DSL0, {
			{0, UNIFORM, sizeof(UniformBufferObject_set0), nullptr}
			});
		
		/* MAZE */
		M1.init(this, MODEL_PATH_MAZE);
		T1.init(this, TEXTURE_PATH_MAZE);
		DS1.init(this, &DSL1, {
			{0, UNIFORM, sizeof(UniformBufferObject_set1), nullptr},
			{1, TEXTURE, 0, &T1}
			});
		
		/* TREASURES */
		M2.init(this, MODEL_PATH_TREASURE);
		T2.init(this, TEXTURE_PATH_TREASURE);
		for (int i = 0; i < N; i++) {
			DST[i].init(this, &DSL1, {
				{0, UNIFORM, sizeof(UniformBufferObject_set1), nullptr},
				{1, TEXTURE, 0, &T2}
				});
		}

	}
		
	void localCleanup() {
		DS0.cleanup();
		
		DS1.cleanup();
		T1.cleanup();
		M1.cleanup();

		for (int i = 0; i < N; i++) {
			DST[i].cleanup();
		}

		T2.cleanup();
		M2.cleanup();

		P1.cleanup();
		DSL0.cleanup();
		DSL1.cleanup();
	}
	
	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {
		
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, P1.graphicsPipeline);
		
		/* GLOBAL */
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			P1.pipelineLayout, 0, 1, &DS0.descriptorSets[currentImage], 0, nullptr);

		/* MAZE */
		VkBuffer vertexBuffers_maze[] = { M1.vertexBuffer };
		VkDeviceSize offsets_maze[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers_maze, offsets_maze);
		vkCmdBindIndexBuffer(commandBuffer, M1.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			P1.pipelineLayout, 1, 1, &DS1.descriptorSets[currentImage], 0, nullptr);
		vkCmdDrawIndexed(commandBuffer,
			static_cast<uint32_t>(M1.indices.size()), 1, 0, 0, 0);

		/* TREASURES */
		
		VkBuffer* vertexBuffers_T[N];
		for (int i = 0; i < N; i++) {
			VkBuffer vertexBuffers[] = {M2.vertexBuffer};
			vertexBuffers_T[i] = vertexBuffers;
			VkDeviceSize offsets_treasure1[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers_T[i], offsets_treasure1);
			vkCmdBindIndexBuffer(commandBuffer, M2.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				P1.pipelineLayout, 1, 1, &DST[i].descriptorSets[currentImage], 0, nullptr);
			vkCmdDrawIndexed(commandBuffer,
				static_cast<uint32_t>(M2.indices.size()), 1, 0, 0, 0);
		
		}
		/*VkBuffer vertexBuffers_treasure1[] = { M2.vertexBuffer };
		VkDeviceSize offsets_treasure1[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers_treasure1, offsets_treasure1);
		vkCmdBindIndexBuffer(commandBuffer, M2.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			P1.pipelineLayout, 1, 1, &DS2.descriptorSets[currentImage], 0, nullptr);
		vkCmdDrawIndexed(commandBuffer,
			static_cast<uint32_t>(M2.indices.size()), 1, 0, 0, 0);

		VkBuffer vertexBuffers_treasure2[] = { M2.vertexBuffer };
		VkDeviceSize offsets_treasure2[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers_treasure2, offsets_treasure2);
		vkCmdBindIndexBuffer(commandBuffer, M2.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			P1.pipelineLayout, 1, 1, &DS3.descriptorSets[currentImage], 0, nullptr);
		vkCmdDrawIndexed(commandBuffer,
			static_cast<uint32_t>(M2.indices.size()), 1, 0, 0, 0);

		VkBuffer vertexBuffers_treasure3[] = { M2.vertexBuffer };
		VkDeviceSize offsets_treasure3[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers_treasure3, offsets_treasure3);
		vkCmdBindIndexBuffer(commandBuffer, M2.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			P1.pipelineLayout, 1, 1, &DS4.descriptorSets[currentImage], 0, nullptr);
		vkCmdDrawIndexed(commandBuffer,
			static_cast<uint32_t>(M2.indices.size()), 1, 0, 0, 0);*/

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

	void setUBO_0(UniformBufferObject_set0* ubo_set0, float deltaT) {

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

		/*if (state == 3) {
			ang3 += deltaT;
		}
		if (state >= 2) {
			ang2 += deltaT;
		}
		if (state >= 1) {
			ang1 += deltaT;
		}*/

		/* GLOBAL */
		UniformBufferObject_set0 ubo_set0{};
		setUBO_0(&ubo_set0, deltaT);
		vkMapMemory(device, DS0.uniformBuffersMemory[0][currentImage], 0, sizeof(ubo_set0), 0, &data);
		memcpy(data, &ubo_set0, sizeof(ubo_set0));
		vkUnmapMemory(device, DS0.uniformBuffersMemory[0][currentImage]);
		
		/* MAZE */
		UniformBufferObject_set1 ubo_set1{};
		ubo_set1.model = glm::mat4(1.0f);
		vkMapMemory(device, DS1.uniformBuffersMemory[0][currentImage], 0, sizeof(ubo_set1), 0, &data);
		memcpy(data, &ubo_set1, sizeof(ubo_set1));
		vkUnmapMemory(device, DS1.uniformBuffersMemory[0][currentImage]);

		/* TREASURES */
		static const float MIN_HEIGHT = 0.2f;
		for (int i = 0; i < N; i++) {
			ubo_set1.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, MIN_HEIGHT + i * 0.5f, 0.0f)) *
				glm::rotate(glm::mat4(1.0f), 
					time * glm::radians(70.0f + 5.0f * i),
					glm::vec3(0.0f, 1.0f, 0.0f));
			vkMapMemory(device, DST[i].uniformBuffersMemory[0][currentImage], 0, sizeof(ubo_set1), 0, &data);
			memcpy(data, &ubo_set1, sizeof(ubo_set1));
			vkUnmapMemory(device, DST[i].uniformBuffersMemory[0][currentImage]);
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
