/* TODO:
 * #ok. Correctly upload the map
 * #ok Fix the map
 * #ok Correctly implement collisions
 * #ok Fix shaders
 * # Add treasures
 * #ok Improve textures
 * #ok Fix map to have it easier to work with
 * # Add sound
 */

#include "SoundMaze.hpp"

const std::string MODEL_PATH_MAZE = "models/maze_translated.obj";
const std::string TEXTURE_PATH_MAZE_Alb = "textures/maze_albedo.jpg";
const std::string TEXTURE_PATH_MAZE_Ref = "textures/maze_metallic.jpg";
const std::string TEXTURE_PATH_MAZE_Rou = "textures/maze_roughness.jpg";
const std::string TEXTURE_PATH_MAZE_Ao = "textures/maze_ao.jpg";

constexpr float ROT_SPEED = glm::radians(60.0f);
const float MOVE_SPEED = 0.5f;
const float MOUSE_RES = 500.0f;
constexpr float FOVY = glm::radians(60.0f);
const float NEAR_FIELD = 0.001f;
const float FAR_FIELD = 5.0f;
constexpr float MAX_UP_ANGLE = glm::radians(70.0f);
constexpr float MAX_DOWN_ANGLE = glm::radians(-70.0f);

struct Object {
	alignas(16) Model model;

	alignas(16) Texture albedo_map;
	alignas(16) Texture metallic_map;
	alignas(16) Texture roughness_map;
	alignas(16) Texture ao_map;

	alignas(16) glm::vec4 specular_color;

	void cleanup() {
		model.cleanup();
		albedo_map.cleanup();
		metallic_map.cleanup();
		roughness_map.cleanup();
		ao_map.cleanup();
	}
};

struct FloorMap {
	stbi_uc* img;
	int width;
	int height;
	
	void loadImg(const char* url) {
		img = stbi_load(url, &width, &height, NULL, 1);
		if (!img) {
			std::cout << "WARNING! Cannot load the map" << std::endl;
			return;
		}
		std::cout << "Map loaded successfully!" << std::endl;
		std::cout << "Width: " << this->width << "; Height: " << this->height << std::endl;
	}
	void cleanup() {
		stbi_image_free(img);
	}
};

struct Pipe {
	alignas(16) Pipeline pipeline {};
	alignas(16) DescriptorSetLayout dsl_global;
	alignas(16) DescriptorSet ds_global;
	alignas(16) DescriptorSetLayout dsl_maze;
	alignas(16) DescriptorSet ds_maze;
	alignas(16) int setsInPipe = 2;

	void cleanup() {
		ds_global.cleanup();
		ds_maze.cleanup();
		pipeline.cleanup();
		dsl_global.cleanup();
		dsl_maze.cleanup();
	}
};

// UNIFORMS
struct ModelViewProjection {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

struct Material {
	alignas(16) glm::vec4 specular_color;
};

struct Lights {
	glm::vec3 lightPositions = glm::vec3(-5, 0.1, -5);
	glm::vec3 lightColors = glm::vec3(1.0, 1.0, 1.0);
};
// UNIFORMS
 
class SoundMaze : public BaseProject {
protected:

	FloorMap map{};
	Object maze{};
	Pipe graphics;

	void setWindowParameters() {	
		windowWidth = 1200;
		windowHeight = 800;
		windowTitle = "Inside the Sound Maze...";
		initialBackgroundColor = {0.0f, 0.0f, 0.0f, 1.0f};
		
		uniformBlocksInPool = 3;
		texturesInPool = 4;
		setsInPool = graphics.setsInPipe;
	}
	
	void localInit() {

		map.loadImg("textures/high-constrast-maze-map-specular.png");
		
		maze.model.init(this, MODEL_PATH_MAZE);

		maze.albedo_map.init(this, TEXTURE_PATH_MAZE_Alb);
		maze.metallic_map.init(this, TEXTURE_PATH_MAZE_Ref);
		maze.roughness_map.init(this, TEXTURE_PATH_MAZE_Rou);
		maze.ao_map.init(this, TEXTURE_PATH_MAZE_Ao);

		maze.specular_color = glm::vec4(1.0, 1.0, 1.0, 32);

		graphics.dsl_global.init(this, {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT} 
		});

		graphics.dsl_maze.init(this, {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT},
			{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
			{2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
			{3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
			{4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
		});

		graphics.pipeline.init(this, "shaders/graphics_vert.spv", "shaders/graphics_frag.spv", { &graphics.dsl_global, &graphics.dsl_maze });
		
		graphics.ds_global.init(this, &graphics.dsl_global, { 
			{0, UNIFORM, sizeof(ModelViewProjection), 
			nullptr} 
		});
		
		graphics.ds_maze.init(this, &graphics.dsl_maze, {
			{0, UNIFORM, sizeof(Lights), nullptr},
			{1, TEXTURE, 0, &maze.albedo_map},
			{2, TEXTURE, 0, &maze.metallic_map},
			{3, TEXTURE, 0, &maze.roughness_map},
			{4, TEXTURE, 0, &maze.ao_map},
		});
	}
		
	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {
		VkBuffer vertexBuffers_maze[] = { maze.model.vertexBuffer };
		VkDeviceSize offsets_maze[] = { 0 };

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics.pipeline.graphicsPipeline);
		
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
			graphics.pipeline.pipelineLayout, 0, 1, &graphics.ds_global.descriptorSets[currentImage],
			0, nullptr);

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers_maze, offsets_maze);
		vkCmdBindIndexBuffer(commandBuffer, maze.model.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
			graphics.pipeline.pipelineLayout, 1, 1, &graphics.ds_maze.descriptorSets[currentImage],
			0, nullptr);

		vkCmdDrawIndexed(commandBuffer, 
			static_cast<uint32_t>(maze.model.indices.size()), 1, 0, 0, 0);
	}

	bool canStep(float x, float y) {
		static const float checkRadius = 0.1;
		static const int checkSteps = 4;
		static const float model_diameter = 10.0f;

		for (int i = 0; i < checkSteps; i++) {
			double check_x = x + cos(6.2832 * i / (float)checkSteps) * checkRadius;
			double check_y = y + sin(6.2832 * i / (float)checkSteps) * checkRadius;

			int x_p = map.width - round(fmax(0.0f, fmin(map.width - 1, (- check_x / model_diameter) * map.width)));
			int y_p = map.height - round(fmax(0.0f, fmin(map.height - 1, (- check_y / model_diameter) * map.height)));
			
			bool walkable = map.img[map.width * int(x_p) + int(y_p)] != 0;
			if(!walkable) return false;
		}
		return true;
	}

	void updateCameraAngles(glm::vec3 *CamAng, float deltaT, float ROT_SPEED) {
		static double old_xpos = 0, old_ypos = 0;
		double xpos, ypos;

		glfwGetCursorPos(window, &xpos, &ypos);
		double m_dx = xpos - old_xpos;
		double m_dy = ypos - old_ypos;
		old_xpos = xpos; old_ypos = ypos;

		glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);

		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
			CamAng->y += m_dx * ROT_SPEED / MOUSE_RES;
			CamAng->x += m_dy * ROT_SPEED / MOUSE_RES;
		}

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

	ModelViewProjection computeMVP(glm::vec3 camAng, glm::vec3 camPos) {
		
		ModelViewProjection mvp{};
		glm::mat3 CamDir = glm::mat3(glm::rotate(glm::mat4(1.0f), camAng.y, glm::vec3(0.0f, 1.0f, 0.0f))) *
			glm::mat3(glm::rotate(glm::mat4(1.0f), camAng.x, glm::vec3(1.0f, 0.0f, 0.0f))) *
			glm::mat3(glm::rotate(glm::mat4(1.0f), camAng.z, glm::vec3(0.0f, 0.0f, 1.0f)));

		glm::mat4 CamMat = glm::translate(glm::transpose(glm::mat4(CamDir)), -camPos);

		glm::mat4 model = glm::mat4(1.0);
		mvp.model = glm::translate(
			glm::rotate(
				glm::translate(
					model, 
					glm::vec3(-5, 0, -5)
				), 
				glm::radians(90.0f), 
				glm::vec3(0, 1, 0)
			),
			glm::vec3(5, 0, 5)
		);

		mvp.view = CamMat;
		mvp.proj = glm::perspective(FOVY, swapChainExtent.width / (float)swapChainExtent.height, NEAR_FIELD, FAR_FIELD);
		mvp.proj[1][1] *= -1;
		return mvp;

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

		static glm::vec3 camAng = glm::vec3(0.0f);
		static glm::vec3 camPos = glm::vec3(-5.0f, .1f, -5.0f);
		updateCameraAngles(&camAng, deltaT, ROT_SPEED);
		updateCameraPosition(&camPos, deltaT, MOVE_SPEED, camAng);
		ModelViewProjection mvp = computeMVP(camAng, camPos);
		vkMapMemory(device, graphics.ds_global.uniformBuffersMemory[0][currentImage], 0, sizeof(mvp), 0, &data);
		memcpy(data, &mvp, sizeof(mvp));
		vkUnmapMemory(device, graphics.ds_global.uniformBuffersMemory[0][currentImage]);

		//Material m{};
		//m.specular_color = maze.specular_color;
		//vkMapMemory(device, graphics.ds_maze.uniformBuffersMemory[0][currentImage], 0, sizeof(m), 0, &data);
		//memcpy(data, &m, sizeof(m));
		//vkUnmapMemory(device, graphics.ds_maze.uniformBuffersMemory[0][currentImage]);

		Lights l;
		vkMapMemory(device, graphics.ds_maze.uniformBuffersMemory[0][currentImage], 0, sizeof(l), 0, &data);
		memcpy(data, &l, sizeof(l));
		vkUnmapMemory(device, graphics.ds_maze.uniformBuffersMemory[0][currentImage]);

	}

	void localCleanup() {
		map.cleanup();
		maze.cleanup();
		graphics.cleanup();
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