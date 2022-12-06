/* TODO:
 * ok. Correctly upload the map
 * #ok Fix the map
 * #ok Correctly implement collisions
 * #ok Fix shaders
 * # Add treasures
 * # Improve textures
 * # Fix map to have it easyer to work with
 * # Add sound
 */

#include "SoundMaze.hpp"

const std::string MODEL_PATH_MAZE = "models/maze_translated.obj";
const std::string TEXTURE_PATH_MAZE = "textures/maze_rock.jpg";

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
	alignas(16) Texture texture;
	alignas(16) glm::vec4 specular_color;

	void cleanup() {
		model.cleanup();
		texture.cleanup();
	}
};

struct Pipe {
	alignas(16) Pipeline pipeline;
	alignas(16) DescriptorSetLayout dsl_global;
	alignas(16) DescriptorSet ds_global;
	alignas(16) DescriptorSetLayout dsl;
	alignas(16) DescriptorSet ds;
	alignas(16) int setsInPipe = 2;

	void cleanup() {
		ds_global.cleanup();
		ds.cleanup();
		pipeline.cleanup();
		dsl_global.cleanup();
		dsl.cleanup();
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
// UNIFORMS
 
class SoundMaze : public BaseProject {
protected:

	Object maze{};
	Pipe p_graphic;
	stbi_uc* map;
	int map_width, map_height;

	void setWindowParameters() {	
		windowWidth = 1200;
		windowHeight = 800;
		windowTitle = "Inside the Sound Maze...";
		initialBackgroundColor = {0.0f, 0.0f, 0.0f, 1.0f};
		
		uniformBlocksInPool = 3;
		texturesInPool = 1;
		setsInPool = p_graphic.setsInPipe;
	}
	
	void localInit() {
		map = stbi_load("textures/high-constrast-maze-map-specular.png", &map_width, &map_height, NULL, 1);
		maze.model.init(this, MODEL_PATH_MAZE);
		maze.texture.init(this, TEXTURE_PATH_MAZE);
		maze.specular_color = glm::vec4(1.0, 1.0, 1.0, 32);

		p_graphic.dsl_global.init(this, { {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT} });

		p_graphic.dsl.init(this, {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT},
			{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
			});

		p_graphic.pipeline.init(this, "shaders/vert0.spv", "shaders/frag0.spv", { &p_graphic.dsl_global, &p_graphic.dsl });

		p_graphic.ds_global.init(this, &p_graphic.dsl_global, { {0, UNIFORM, sizeof(ModelViewProjection), nullptr} });
		p_graphic.ds.init(this, &p_graphic.dsl, {
			{0, UNIFORM, sizeof(Material), nullptr},
			{1, TEXTURE, 0, &maze.texture},
		});
	}
		
	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {
		VkBuffer vertexBuffers_maze[] = { maze.model.vertexBuffer };
		VkDeviceSize offsets_maze[] = { 0 };

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_graphic.pipeline.graphicsPipeline);
		
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
			p_graphic.pipeline.pipelineLayout, 0, 1, &p_graphic.ds_global.descriptorSets[currentImage],
			0, nullptr);

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers_maze, offsets_maze);
		vkCmdBindIndexBuffer(commandBuffer, maze.model.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
			p_graphic.pipeline.pipelineLayout, 1, 1, &p_graphic.ds.descriptorSets[currentImage],
			0, nullptr);

		vkCmdDrawIndexed(commandBuffer, 
			static_cast<uint32_t>(maze.model.indices.size()), 1, 0, 0, 0);
	}

	bool canStep(float x, float y) {

		static const float checkRadius = 0.15;
		static const int checkSteps = 4;
		static const float model_diameter = 10.0f;

		for (int i = 0; i < checkSteps; i++) {
			double check_x = x + cos(6.2832 * i / (float)checkSteps) * checkRadius;
			double check_y = y + sin(6.2832 * i / (float)checkSteps) * checkRadius;

			int x_p = map_width - round(fmax(0.0f, fmin(map_width - 1, (- check_x / model_diameter) * map_width)));
			int y_p = map_height - round(fmax(0.0f, fmin(map_height - 1, (- check_y / model_diameter) * map_height)));
			
			bool walkable = map[map_width * int(x_p) + int(y_p)] != 0;
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
		vkMapMemory(device, p_graphic.ds_global.uniformBuffersMemory[0][currentImage], 0, sizeof(mvp), 0, &data);
		memcpy(data, &mvp, sizeof(mvp));
		vkUnmapMemory(device, p_graphic.ds_global.uniformBuffersMemory[0][currentImage]);


		Material m{};
		m.specular_color = maze.specular_color;
		vkMapMemory(device, p_graphic.ds.uniformBuffersMemory[0][currentImage], 0, sizeof(m), 0, &data);
		memcpy(data, &m, sizeof(m));
		vkUnmapMemory(device, p_graphic.ds.uniformBuffersMemory[0][currentImage]);

	}

	void localCleanup() {
		stbi_image_free(map);
		maze.cleanup();
		p_graphic.cleanup();
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
