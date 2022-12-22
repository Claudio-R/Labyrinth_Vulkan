/* TODO:
 * #ok. Correctly upload the map
 * #ok Fix the map
 * #ok Correctly implement collisions
 * #ok Fix shaders
 * #ok Add lights
 * #ok Add treasures
 * #ok Improve textures
 * #ok Fix map to have it easier to work with
 * # Add sound
 */

#include "SoundMaze.hpp"

const std::string MODEL_PATH_MAZE = "models/maze_translated.obj";
const std::string MODEL_PATH_TREASURE = "models/icosphere_translated.obj";

const std::string TEXTURE_PATH_MAZE_Alb = "textures/maze_albedo.jpg";
const std::string TEXTURE_PATH_MAZE_Ref = "textures/maze_metallic.jpg";
const std::string TEXTURE_PATH_MAZE_Rou = "textures/maze_roughness.jpg";
const std::string TEXTURE_PATH_MAZE_Ao = "textures/maze_ao.jpg";

const std::string TEXTURE_PATH_TREASURE_base = "textures/treasure_baseColor.png";
const std::string TEXTURE_PATH_TREASURE_Ref = "textures/treasure_metallic.png";
const std::string TEXTURE_PATH_TREASURE_Rou = "textures/treasure_roughness.png";
const std::string TEXTURE_PATH_TREASURE_Em = "textures/treasure_metallic.png";

constexpr float NUM_TREASURES = 10;
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
	alignas(16) Texture light_map; // ao or emissive
	
	void cleanup() {
		model.cleanup();
		albedo_map.cleanup();
		metallic_map.cleanup();
		roughness_map.cleanup();
		light_map.cleanup();
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
	alignas(16) DescriptorSetLayout dsl_set;
	alignas(16) DescriptorSet ds_global;
	alignas(16) DescriptorSet ds_set;
	alignas(16) std::vector <DescriptorSetLayout> dsls;
	alignas(16) std::vector <DescriptorSet> dss;

	void cleanup() {
		pipeline.cleanup();
		for (auto& dsl : dsls) { dsl.cleanup(); }
		for (auto& ds : dss) { ds.cleanup(); }
	}
};

// UNIFORMS
struct ModelViewProjection {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

struct Lights {
	glm::vec3 lightPositions[4] = { 
		glm::vec3(-5 - 0.1, 0.1, -5 - 0.1),
		glm::vec3(-5 - 0.1, 0.1, -5 + 0.1),
		glm::vec3(-5 + 0.1, 0.1, -5 - 0.1),
		glm::vec3(-5 + 0.1, 0.1, -5 + 0.1)
	};
	
	glm::vec3 lightColors[4] = {
		glm::vec3(1, 0, 0),
		glm::vec3(0, 1, 0),
		glm::vec3(0, 0, 1),
		glm::vec3(1, 1, 1)
	};
};

class SoundMaze : public BaseProject {
protected:

	FloorMap map{};
	Object maze{};
	Object treasure{};

	Pipe mazePipeline{};
	Pipe treasuresPipeline{};
	
	std::vector <glm::vec3> translations;

	void setWindowParameters() {	
		windowWidth = 1200;
		windowHeight = 800;
		windowTitle = "Inside the Sound Maze...";
		initialBackgroundColor = {0.0f, 0.0f, 0.0f, 1.0f};
		
		// Non può essere spostata dentro init
		uniformBlocksInPool = 2 + 2 * NUM_TREASURES; // 2 per maze + 1 per treasure
		texturesInPool = 4 + 4 * NUM_TREASURES; // 4 per maze + 4 per treasure
		setsInPool = 2 + 2 * NUM_TREASURES; // 2 per maze + 2 per treasure
	}
	
	void localInit() {

		// Load the map
		{
			map.loadImg("textures/high-constrast-maze-map.png");
			generateRandomTranslations();
			assert(map.width > 0);
			assert(map.height > 0);
		}

		// Load the models
		{
			maze.model.init(this, MODEL_PATH_MAZE);
			maze.albedo_map.init(this, TEXTURE_PATH_MAZE_Alb);
			maze.metallic_map.init(this, TEXTURE_PATH_MAZE_Ref);
			maze.roughness_map.init(this, TEXTURE_PATH_MAZE_Rou);
			maze.light_map.init(this, TEXTURE_PATH_MAZE_Ao);
			assert(maze.model.indices.size() > 0);
		}
		{
			treasure.model.init(this, MODEL_PATH_TREASURE);
			treasure.albedo_map.init(this, TEXTURE_PATH_TREASURE_base);
			treasure.metallic_map.init(this, TEXTURE_PATH_TREASURE_base);
			treasure.roughness_map.init(this, TEXTURE_PATH_TREASURE_base);
			treasure.light_map.init(this, TEXTURE_PATH_TREASURE_base);
			assert(treasure.model.indices.size() > 0);
		}
		
		// Initialize Pipelines
		{	
			// Maze	
			mazePipeline.dsls.push_back(DescriptorSetLayout{});
			mazePipeline.dsls[0].init(this, {
				{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT} // MVP
				});

			mazePipeline.dsls.push_back(DescriptorSetLayout{});
			mazePipeline.dsls[1].init(this, {
				{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT}, // Lights 
				{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}, // Albedo
				{2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}, // Metallic
				{3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}, // Roughness
				{4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}, // AO
				});

			mazePipeline.pipeline.init(this, 
				"shaders/graphics_maze_vert.spv", "shaders/graphics_maze_frag.spv", 
				{ 
					&mazePipeline.dsls[0],
					&mazePipeline.dsls[1]
				});
			
			mazePipeline.dss.push_back(DescriptorSet{});
			mazePipeline.dss[0].init(this, &mazePipeline.dsls[0], {
				{0, UNIFORM, sizeof(ModelViewProjection), nullptr}
				});
			
			mazePipeline.dss.push_back(DescriptorSet{});
			mazePipeline.dss[1].init(this, &mazePipeline.dsls[1], {
				{0, UNIFORM, sizeof(Lights), nullptr},
				{1, TEXTURE, 0, &maze.albedo_map},
				{2, TEXTURE, 0, &maze.metallic_map},
				{3, TEXTURE, 0, &maze.roughness_map},
				{4, TEXTURE, 0, &maze.light_map},
				});
			
			std::cout << "Maze Pipeline Initialized" << std::endl;
		}
		{
			// Treasures
			treasuresPipeline.dsls.push_back(DescriptorSetLayout{});
			treasuresPipeline.dsls[0].init(this, {
				{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT} // MVP
				});
			
			treasuresPipeline.dsls.push_back(DescriptorSetLayout{});
			treasuresPipeline.dsls[1].init(this, {
				{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT},
				{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
				{2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
				{3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
				{4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
				});
			
			treasuresPipeline.pipeline.init(this,
				"shaders/graphics_maze_vert.spv", "shaders/graphics_maze_frag.spv",
				{
					&treasuresPipeline.dsls[0],
					&treasuresPipeline.dsls[1]
				});

			for (int i = 0; i < NUM_TREASURES; i++) {
				treasuresPipeline.dss.push_back(DescriptorSet{});
				treasuresPipeline.dss[i].init(this, &treasuresPipeline.dsls[0], {
					{0, UNIFORM, sizeof(ModelViewProjection), nullptr}
					});
			}
			
			for (int i = 0; i < NUM_TREASURES; i++) {
				treasuresPipeline.dss.push_back(DescriptorSet{});
				treasuresPipeline.dss[NUM_TREASURES + i].init(this, &treasuresPipeline.dsls[1], {
					//{0, TEXTURE, 0, &maze.albedo_map},
					{0, UNIFORM, sizeof(Lights), nullptr},
					{1, TEXTURE, 0, &treasure.albedo_map},
					{2, TEXTURE, 0, &treasure.metallic_map},
					{3, TEXTURE, 0, &treasure.roughness_map},
					{4, TEXTURE, 0, &treasure.light_map},
					});
			}
			
			std::cout << "Treasures Pipeline Initialized" << std::endl;
		}
		
	}
		
	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {
	
		int firstSet;
		int descriptorSetCount = 1;
		
		// Graphics pipelines
		{
			// Maze
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mazePipeline.pipeline.graphicsPipeline);
			
			firstSet = 0;
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
				mazePipeline.pipeline.pipelineLayout, firstSet, descriptorSetCount, 
				&mazePipeline.dss[firstSet].descriptorSets[currentImage],
				0, nullptr);
		
			firstSet = 1;
			VkBuffer vertexBuffers[] = { maze.model.vertexBuffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(commandBuffer, maze.model.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
				mazePipeline.pipeline.pipelineLayout, firstSet, descriptorSetCount, 
				&mazePipeline.dss[firstSet].descriptorSets[currentImage],
				0, nullptr);
		
			vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(maze.model.indices.size()), 1, 0, 0, 0);
		}
				
		{
			// Treasures
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, treasuresPipeline.pipeline.graphicsPipeline);

			for (int i = 0; i < NUM_TREASURES; i++) {
				
				firstSet = 0;
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
					treasuresPipeline.pipeline.pipelineLayout, firstSet, descriptorSetCount,
					&treasuresPipeline.dss[i].descriptorSets[currentImage],
					0, nullptr);
				
				
				firstSet = 1;
				VkBuffer vertexBuffers[] = { treasure.model.vertexBuffer };
				VkDeviceSize offsets[] = { 0 };
				vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
				vkCmdBindIndexBuffer(commandBuffer, treasure.model.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
					treasuresPipeline.pipeline.pipelineLayout, firstSet, descriptorSetCount,
					&treasuresPipeline.dss[NUM_TREASURES + i].descriptorSets[currentImage],
					0, nullptr);
				vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(treasure.model.indices.size()), 1, 0, 0, 0);
				
			}
		}
	}

	void generateRandomTranslations() {
		
		static const float model_diameter = 10.0f;
		glm::vec2 candidate{};
		
		srand(time(NULL));
		while (translations.size() < NUM_TREASURES) {

			candidate.x = (float)(rand() % 1000) * (model_diameter / 1000) - (model_diameter / 2.0f);
			candidate.y = (float)(rand() % 1000) * (model_diameter / 1000) - (model_diameter / 2.0f);

			glm::vec2 mapPos = mazeToMap(candidate.x, candidate.y);
			if (mapPos.x < 0 || mapPos.x >= map.width || 
				mapPos.y < 0 || mapPos.y >= map.height ||
				map.img[map.width * (int)mapPos.x + (int)mapPos.y] != 0) {
				continue;
			}
			translations.push_back(glm::vec3(-candidate.x, 0.0f, -candidate.y));
			
		}
		
		std::cout << "Generated " << translations.size() << " treasure positions" << std::endl;
		for (glm::vec3 pos : translations) {
			std::cout << pos.x << ", " << pos.z << std::endl;
		}
	}
	
	glm::vec2 mazeToMap(float x_maze, float y_maze) {
		static const float model_diameter = 10.0f;
		int x_p = map.width - round(fmax(0.0f, fmin(map.width - 1, (-x_maze / model_diameter) * map.width)));
		int y_p = map.height - round(fmax(0.0f, fmin(map.height - 1, (-y_maze / model_diameter) * map.height)));
		return glm::vec2(x_p, y_p);
	}
	
	bool canStep(float x, float y) {
		static const float checkRadius = 0.1;
		static const int checkSteps = 8;
		for (int i = 0; i < checkSteps; i++) {
			double check_x = x + cos(6.2832 * i / (float)checkSteps) * checkRadius;
			double check_y = y + sin(6.2832 * i / (float)checkSteps) * checkRadius;
			glm::vec2 map_pos = mazeToMap(check_x, check_y);
			bool walkable = map.img[map.width * int(map_pos.y) + int(map_pos.x)] != 0;
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
	
	ModelViewProjection computeMVP(glm::vec3 camAng, glm::vec3 camPos, glm::vec3 translation = glm::vec3(0.0f)) {

		ModelViewProjection mvp{};
		glm::mat3 CamDir = glm::mat3(glm::rotate(glm::mat4(1.0f), camAng.y, glm::vec3(0.0f, 1.0f, 0.0f))) *
			glm::mat3(glm::rotate(glm::mat4(1.0f), camAng.x, glm::vec3(1.0f, 0.0f, 0.0f))) *
			glm::mat3(glm::rotate(glm::mat4(1.0f), camAng.z, glm::vec3(0.0f, 0.0f, 1.0f)));

		glm::mat4 CamMat = glm::translate(glm::transpose(glm::mat4(CamDir)), -camPos);

		glm::mat4 model = glm::mat4(1.0);
		mvp.model = glm::translate(
				glm::translate(
					model,
					glm::vec3(-5, 0, -5) + translation
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
		
		ModelViewProjection mvp = computeMVP(camAng, camPos, glm::vec3(0.0f, -0.5f, 0.0f));
		Lights l;

		// Maze Pipeline
		{
			vkMapMemory(device, mazePipeline.dss[0].uniformBuffersMemory[0][currentImage], 0, sizeof(mvp), 0, &data);
			memcpy(data, &mvp, sizeof(mvp));
			vkUnmapMemory(device, mazePipeline.dss[0].uniformBuffersMemory[0][currentImage]);
		
			vkMapMemory(device, mazePipeline.dss[1].uniformBuffersMemory[0][currentImage], 0, sizeof(l), 0, &data);
			memcpy(data, &l, sizeof(l));
			vkUnmapMemory(device, mazePipeline.dss[1].uniformBuffersMemory[0][currentImage]);
		}

		// Treasures Pipeline
		{
			for (int i = 0; i < NUM_TREASURES; i++) {
				mvp = computeMVP(camAng, camPos, translations[i]);
				vkMapMemory(device, treasuresPipeline.dss[i].uniformBuffersMemory[0][currentImage], 0, sizeof(mvp), 0, &data);
				memcpy(data, &mvp, sizeof(mvp));
				vkUnmapMemory(device, treasuresPipeline.dss[i].uniformBuffersMemory[0][currentImage]);
			
				vkMapMemory(device, treasuresPipeline.dss[NUM_TREASURES + i].uniformBuffersMemory[0][currentImage], 0, sizeof(l), 0, &data);
				memcpy(data, &l, sizeof(l));
				vkUnmapMemory(device, treasuresPipeline.dss[NUM_TREASURES + i].uniformBuffersMemory[0][currentImage]);
			}
		}

	}

	void localCleanup() {
		map.cleanup();
		maze.cleanup();
		treasure.cleanup();
		mazePipeline.cleanup();
		treasuresPipeline.cleanup();
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