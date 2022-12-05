/* TODO:
 * ok. Correctly upload the map
 * ok. Fix the map
 * ok. Correctly implement collisions
 * 4. Textures
 * 5. Fix shaders
 * 6. Add lights
 * 7. Add sound
 */

#include "SoundMaze.hpp"

const std::string MODEL_PATH_MAZE = "models/maze_translated.obj";
const std::string TEXTURE_PATH_MAZE = "textures/maze_rock.jpg";
const std::string MODEL_PATH_TREASURE = "models/icosphere.obj";
const std::string TEXTURE_PATH_TREASURE = "textures/maze_rock.jpg";

// UNIFORMS
struct ModelViewProjection {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

struct Material {
	alignas(16) glm::vec3 specular;
	alignas(16) float shininess;
};

struct Light {
	alignas(16) glm::mat4 model;
	alignas(16) glm::vec3 position;
	alignas(16) glm::vec3 ambient;
	alignas(16) glm::vec3 diffuse;
	alignas(16) glm::vec3 specular;
};
 
class SoundMaze : public BaseProject {
protected:
	// Map
	int map_width, map_height;
	stbi_uc* map;

	Pipeline P_maze; // MAZE
	//Pipeline P_treasure; // TREASURES

	DescriptorSetLayout DSL_mvp; // equal for both pipelines
	DescriptorSetLayout DSL_maze;
	//DescriptorSetLayout DSL_treasure;

	DescriptorSet DS_mvp;

	Model M_maze;
	Texture T_maze;
	DescriptorSet DS_maze;

	//Model M_treasure;
	//Texture T_treasure;
	//DescriptorSet DS_treasure;
	
	void setWindowParameters() {	
		windowWidth = 1200;
		windowHeight = 800;
		windowTitle = "Inside the Sound Maze...";
		initialBackgroundColor = {0.0f, 0.0f, 0.0f, 1.0f};
		
		uniformBlocksInPool = 3;
		texturesInPool = 1;
		setsInPool = 2;
	}
	
	void loadMap() {
		map = stbi_load("textures/high-constrast-maze-map-specular.png", &map_width, &map_height, NULL, 1);
		if (map) {
			std::cout << "Map loaded! Size --> " << map_width << "x" << map_height << std::endl;
			/* Si muove dando precedenza alle righe */
		}
	}

	void localInit() {

		loadMap();
		std::cout << remap(glm::vec2(0, 0)).x << std::endl;
		std::cout << remap(glm::vec2(0, 5)).x << std::endl;
		std::cout << remap(glm::vec2(5, 0)).x << std::endl;
		std::cout << remap(glm::vec2(5, 5)).x << std::endl;

		/* DESCRIPTOR SET LAYOUTS */
		DSL_mvp.init(this, {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
			});

		DSL_maze.init(this, {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT},
			{1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT},
			{2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
			});

		//DSL_treasure.init(this, {
		//	{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT},
		//	{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
		//	});

		/* PIPELINE */
		P_maze.init(this, "shaders/vert0.spv", "shaders/frag0.spv", { &DSL_mvp, &DSL_maze });
		//P_treasure.init(this, "shaders/vert1.spv", "shaders/frag1.spv", { &DSL_mvp, &DSL_treasure });

		/* DESCRIPTOR SETS */
		DS_mvp.init(this, &DSL_mvp, {
			{0, UNIFORM, sizeof(ModelViewProjection), nullptr}
		});
		
		/* MAZE */
		M_maze.init(this, MODEL_PATH_MAZE);
		T_maze.init(this, TEXTURE_PATH_MAZE);
		DS_maze.init(this, &DSL_maze, {
			{0, UNIFORM, sizeof(Material), nullptr},
			{1, UNIFORM, sizeof(Light), nullptr},
			{2, TEXTURE, 0, &T_maze},
		});
		
		/* TREASURES */
		//M_treasure.init(this, MODEL_PATH_TREASURE);
		//T_treasure.init(this, TEXTURE_PATH_TREASURE);
		//DS_treasure.init(this, &DSL_treasure, {
		//	{0, UNIFORM, sizeof(Light), nullptr},
		//	{1, TEXTURE, 0, &T_treasure}
		//});
	}
		
	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {
		
		/* PIPELINE 1 */
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, P_maze.graphicsPipeline);
		
		// Set0 - MVP
		vkCmdBindDescriptorSets(commandBuffer, 
			VK_PIPELINE_BIND_POINT_GRAPHICS, 
			P_maze.pipelineLayout, 
			0, 1, &DS_mvp.descriptorSets[currentImage],
			0, nullptr);
		
		// Set1 - Maze
		VkBuffer vertexBuffers_maze[] = { M_maze.vertexBuffer };
		VkDeviceSize offsets_maze[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers_maze, offsets_maze);
		vkCmdBindIndexBuffer(commandBuffer, M_maze.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(commandBuffer, 
			VK_PIPELINE_BIND_POINT_GRAPHICS, 
			P_maze.pipelineLayout, 1, 1, &DS_maze.descriptorSets[currentImage], 
			0, nullptr);
		vkCmdDrawIndexed(commandBuffer, 
			static_cast<uint32_t>(M_maze.indices.size()), 1, 0, 0, 0);
		
		/* PIPELINE 2 */
		//vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, P_treasure.graphicsPipeline);
		//vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, P_treasure.pipelineLayout, 0, 1, &DS_mvp.descriptorSets[currentImage], 0, nullptr);
		//VkBuffer vertexBuffers_treasures[] = { M_treasure.vertexBuffer };
		//VkDeviceSize offsets_treasure[] = { 0 };
		//vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers_treasures, offsets_treasure);
		//vkCmdBindIndexBuffer(commandBuffer, M_treasure.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		//vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, P_treasure.pipelineLayout, 1, 1, &DS_treasure.descriptorSets[currentImage], 0, nullptr);
		//vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(M_treasure.indices.size()), 1, 0, 0, 0);

	}

	glm::vec2 remap(glm::vec2 v) {

		int pixX = map_width - round(fmax(0.0f, fmin(map_width - 1, (-v.x / 10.0f) * map_width)));
		int pixY = map_height - round(fmax(0.0f, fmin(map_height - 1, (-v.y / 10.0f) * map_height)));

		double model_radius = 5; //5 cm
		float x = v.x;
		float y = v.y;
		
		x = glm::clamp(double(x), -model_radius, model_radius) + model_radius; // [0, 10]
		x = x / (2 * model_radius); // [0, 1]
		int x_p = round(x * map_width); // [0, 1017]

		y = glm::clamp(double(y), -model_radius, model_radius) + model_radius; // [0, 5.2]
		y = y / (2 * model_radius); // [0, 1]
		int y_p = round(y * map_width);
		return glm::vec2(pixX, pixY);
	}

	bool canStepPoint(float x, float y) {
		glm::vec2 v = remap(glm::vec2(x, y));
		return map[map_width * int(v.x) + int(v.y)] != 0;
	}

	bool canStep(float x, float y) {
		//std::cout << x << " " << y << std::endl;
		glm::vec2 v = remap(glm::vec2(x, y));
		//std::cout << "Pixels: " << v.x << " " << v.y << std::endl;

		static float checkRadius = 0.1;
		static int checkSteps = 12;
		for (int i = 0; i < checkSteps; i++) {
			double check_x = x + cos(6.2832 * i / (float)checkSteps) * checkRadius;
			double check_y = y + sin(6.2832 * i / (float)checkSteps) * checkRadius;
			if(!canStepPoint(check_x, check_y)) return false;
		}
		return true;
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
		glm::vec3 oldCamPos = *CamPos;
		glm::vec3 newCamPos = *CamPos;
		if (glfwGetKey(window, GLFW_KEY_A)) {
			newCamPos -= MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), CamAng.y,
				glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(1, 0, 0, 1)) * deltaT;
		}
		if (glfwGetKey(window, GLFW_KEY_D)) {
			newCamPos += MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), CamAng.y,
				glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(1, 0, 0, 1)) * deltaT;
		}
		if (glfwGetKey(window, GLFW_KEY_S)) {
			newCamPos += MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), CamAng.y,
				glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 0, 1, 1)) * deltaT;
		}
		if (glfwGetKey(window, GLFW_KEY_W)) {
			newCamPos -= MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), CamAng.y,
				glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 0, 1, 1)) * deltaT;
		}

		if (!canStep(newCamPos.x, newCamPos.z)) {
			*CamPos = oldCamPos;
			return;
		}
		*CamPos = newCamPos;
	}

	float computeMVP(ModelViewProjection* mvp) {

		static auto startTime = std::chrono::high_resolution_clock::now();
		static float lastTime = 0.0f;

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>
			(currentTime - startTime).count();
		float deltaT = time - lastTime;
		lastTime = time;

		static constexpr float ROT_SPEED = glm::radians(60.0f);
		static const float MOVE_SPEED = 0.5f;
		static const float MOUSE_RES = 500.0f;

		static glm::vec3 CamAng = glm::vec3(0.0f);
		static glm::vec3 CamPos = glm::vec3(-5.0f, .1f, -5.0f);
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

		constexpr float FOVY = glm::radians(60.0f);
		const float NEAR_FIELD = 0.001f;
		const float FAR_FIELD = 5.0f;

		glm::mat4 model = glm::rotate(glm::scale(glm::mat4(1.0f), glm::vec3(-1, 1, -1)), (float)glm::radians(-35.0f), glm::vec3(0, 1, 0));
		mvp->model = glm::translate(
			glm::rotate(
				glm::translate(glm::mat4(1.0), glm::vec3(-5, 0, -5)), glm::radians(90.0f), glm::vec3(0, 1, 0)),
			glm::vec3(5, 0, 5));
			
		mvp->view = CamMat;
		mvp->proj = glm::perspective(FOVY, swapChainExtent.width / (float)swapChainExtent.height, NEAR_FIELD, FAR_FIELD);
		mvp->proj[1][1] *= -1;

		return time;
	}

	void updateUniformBuffer(uint32_t currentImage) {

		void* data;

		ModelViewProjection mvp{};
		float time = computeMVP(&mvp);
		vkMapMemory(device, DS_mvp.uniformBuffersMemory[0][currentImage], 0, sizeof(mvp), 0, &data);
		memcpy(data, &mvp, sizeof(mvp));
		vkUnmapMemory(device, DS_mvp.uniformBuffersMemory[0][currentImage]);

		Material maze{};
		maze.specular = glm::vec3(0.5f, 0.5f, 0.5f);
		maze.shininess = 32.0f;
		vkMapMemory(device, DS_maze.uniformBuffersMemory[0][currentImage], 0, sizeof(maze), 0, &data);
		memcpy(data, &maze, sizeof(maze));
		vkUnmapMemory(device, DS_maze.uniformBuffersMemory[0][currentImage]);

		Light light{};
	/*	light.model = glm::translate(glm::mat4(1.0f),
			glm::vec3(light.position)) * glm::rotate(glm::mat4(1.0f), time * glm::radians(70.0f + 5.0f), glm::vec3(0.0f, 1.0f, 0.0f));*/
		light.model = glm::mat4(1.0);
		light.position = glm::vec3(0.0f, 0.1f, 0.0f);
		light.ambient = glm::vec3(0.2f);
		light.diffuse = glm::vec3(0.5f);
		light.specular = glm::vec3(1.0f);
		vkMapMemory(device, DS_maze.uniformBuffersMemory[1][currentImage], 0, sizeof(Light), 0, &data);
		memcpy(data, &light, sizeof(Light));
		vkUnmapMemory(device, DS_maze.uniformBuffersMemory[1][currentImage]);

		/*
		Light lightCast{};
		lightCast.position = glm::vec3(0.2f, 0.5f, -0.2f);
		lightCast.ambient = glm::vec3(0.8f, 0.2f, 0.2f);
		lightCast.diffuse = glm::vec3(0.8f, 0.2f, 0.2f);
		lightCast.specular = glm::vec3(0.8f, 0.2f, 0.2f);
		lightCast.model = glm::translate(glm::mat4(1.0f),
			glm::vec3(lightCast.position)) * glm::rotate(glm::mat4(1.0f), time * glm::radians(70.0f + 5.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		vkMapMemory(device, DS_treasure.uniformBuffersMemory[0][currentImage], 0, sizeof(Light), 0, &data);
		memcpy(data, &lightCast, sizeof(Light));
		vkUnmapMemory(device, DS_treasure.uniformBuffersMemory[0][currentImage]);
		*/

	}

	void localCleanup() {
		stbi_image_free(map);
	
		DS_mvp.cleanup();

		DS_maze.cleanup();
		T_maze.cleanup();
		M_maze.cleanup();

		//DS_treasure.cleanup();

		//T_treasure.cleanup();
		//M_treasure.cleanup();

		P_maze.cleanup();
		//P_treasure.cleanup();
		DSL_mvp.cleanup();
		DSL_maze.cleanup();
		//DSL_treasure.cleanup();
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
