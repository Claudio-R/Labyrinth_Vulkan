#include "SoundMaze.hpp"

const std::string MODEL_PATH_MAZE = "models/maze.obj";
const std::string MODEL_PATH_TREASURE = "models/sphere.obj";
const std::string MODEL_PATH_SKYBOX = "models/SkyBoxCube.obj";

const std::string TEXTURE_PATH_MAZE_Alb = "textures/maze/maze_alb.jpg";
const std::string TEXTURE_PATH_MAZE_Ref = "textures/maze/maze_ref.jpg";
const std::string TEXTURE_PATH_MAZE_Rou = "textures/maze/maze_rou.jpg";
const std::string TEXTURE_PATH_MAZE_Ao = "textures/maze/maze_ao.jpg";

const std::string TEXTURE_PATH_TREASURE_base = "textures/treasures/treasure_baseColor.png";
const std::string TEXTURE_PATH_TREASURE_Ref = "textures/treasures/treasure_metallic.png";
const std::string TEXTURE_PATH_TREASURE_Rou = "textures/treasures/treasure_roughness.png";
const std::string TEXTURE_PATH_TREASURE_Em = "textures/treasures/treasure_metallic.png";

const char* SKYBOX_RIGHT = "textures/sky1/right.png";
const char* SKYBOX_LEFT = "textures/sky1/left.png";
const char* SKYBOX_TOP = "textures/sky1/top.png";
const char* SKYBOX_BOTTOM = "textures/sky1/bottom.png";
const char* SKYBOX_FRONT = "textures/sky1/front.png";
const char* SKYBOX_BACK = "textures/sky1/back.png";
const char* AUDIO_PATH = "audio/248.wav";
const char* STEPS_PATH   = "audio/steps.wav";

constexpr float ROT_SPEED = glm::radians(60.0f);
constexpr float MOVE_SPEED = 0.5f;
const float MOUSE_RES = 500.0f;
constexpr float FOVY = glm::radians(60.0f);
const float NEAR_FIELD = 0.001f;
const float FAR_FIELD = 20.0f;
const float SAMPLE_RATE = 44100.0f;
constexpr float MAX_UP_ANGLE = glm::radians(70.0f);
constexpr float MAX_DOWN_ANGLE = glm::radians(-70.0f);

constexpr float SCALE_FACTOR = 2.0f;
constexpr float MODEL_DIAMETER = 10.0f * SCALE_FACTOR;
constexpr float TREASURE_DIAMETER = 0.1f * SCALE_FACTOR;
constexpr int NUM_TREASURES = 10;

struct Object {
    Model model;
    Texture albedo_map;
    Texture metallic_map;
    Texture roughness_map;
    Texture light_map;
    
    void cleanup() {
        model.cleanup();
        albedo_map.cleanup();
        metallic_map.cleanup();
        roughness_map.cleanup();
        light_map.cleanup();
    }
};

struct AppState{
    ALuint sources[NUM_TREASURES + 1];
    ALuint buffers[2];
    double duration;
};

struct FloorMap {
    stbi_uc* img;
    GLFWwindow* window;
    int width;
    int height;
    std::vector <glm::vec3> treasuresPositions;
    int treasuresFound = 0;
    
    void init(const char* url, GLFWwindow* window) {
        loadImg(url);
        this->window = window;
        generateRandomTreasuresPositions();
        
    }
    
    void loadImg(const char* url) {
        img = stbi_load(url, &width, &height, NULL, 1);
        if (!img) {
            std::cout << "WARNING! Cannot load the map" << std::endl;
            return;
        }
    }

    void generateRandomTreasuresPositions() {
        glm::vec2 candidate{};
        srand(time(NULL));
        while (treasuresPositions.size() < NUM_TREASURES) {
            candidate.x = (float)(rand() % 1000) * (MODEL_DIAMETER / (1000)) - (MODEL_DIAMETER / 2.0f);
            candidate.y = (float)(rand() % 1000) * (MODEL_DIAMETER / (1000)) - (MODEL_DIAMETER / 2.0f);

            if (glm::length(candidate) > (MODEL_DIAMETER / 2.0f) - 0.5f) { continue; }
            if (glm::length(candidate) < 0.6f * SCALE_FACTOR) { continue; }
            if (isWallAround(candidate.x, candidate.y, TREASURE_DIAMETER)) { continue; }
			
            bool valid = true;
            for (glm::vec3 pos : treasuresPositions) {
				if (glm::length(pos - glm::vec3(candidate, 0.0f)) < 4 * TREASURE_DIAMETER) {
					valid = false;
					break;
				}
			}
            if (!valid) { continue; }

            treasuresPositions.push_back(glm::vec3(candidate.x, 0.0f, candidate.y));
        }
    }

    bool isWallAround(float center_x, float center_y, float radius = 0.1f) {
        const int steps = 8;
        float r = 0.0f;
        while(r < radius + 0.1f) {
            for (int i = 0; i < steps; i++) {
                double x = center_x + cos(6.2832 * i / (float)steps) * r;
                double y = center_y + sin(6.2832 * i / (float)steps) * r;
                if (getPixel(x, y) == 0) return true;
            }
            r += 0.1f;
        }
        return false;
    }
    
    int getPixel(float x_maze, float y_maze) {
        float x_norm = (MODEL_DIAMETER / 2 - x_maze) / MODEL_DIAMETER;
        float y_norm = (MODEL_DIAMETER / 2 - y_maze) / MODEL_DIAMETER;

        int x_p = width - round(std::clamp(x_norm * width, 0.0f, (float)width - 1));
        int y_p = width - round(std::clamp(y_norm * height, 0.0f, (float)height - 1));
        int map_index = width * y_p + x_p;

        if (map_index < 0 || map_index >= width * height) { return 0; }
        return img[map_index];
    }
    
    void checkPosition(glm::vec3 CamPos, AppState appState) {
        for (glm::vec3 pos : treasuresPositions) {
			if (glm::length(pos) == 0.0f) { continue; }
            if (glm::distance(CamPos, pos) < 2 * TREASURE_DIAMETER) {
                int index = std::find(treasuresPositions.begin(), treasuresPositions.end(), pos) - treasuresPositions.begin();
                treasuresPositions[index] = glm::vec3(0.0f);
                
                //fadeout
                auto t  = 0;
                float d = 1000;
                while (t < d){
                    alSourcef(appState.sources[index], AL_GAIN, 0.2f * (1.0f - t / d));
                    t++;
                }
                
                alSourceStop(appState.sources[index]);
				treasuresFound++;
				glfwSetWindowTitle(window, ("Treasures left: " + std::to_string(NUM_TREASURES - treasuresFound)).c_str());
                break;
            }
        }
    }
    
    void cleanup() {
        stbi_image_free(img);
    }
};

struct Pipe {
    Pipeline pipeline {};
    DescriptorSetLayout dsl_global;
    DescriptorSetLayout dsl_set;
    DescriptorSet ds_global;
    DescriptorSet ds_set;
    std::vector <DescriptorSetLayout> dsls;
    std::vector <DescriptorSet> dss;

    void cleanup() {
        pipeline.cleanup();
        for (auto& dsl : dsls) { dsl.cleanup(); }
        for (auto& ds : dss) { ds.cleanup(); }
    }
};

struct Sky {
	SkyBox skybox;
	void init(BaseProject* BP, std::string objFile, std::vector <const char*> textureFiles) {
		skybox.init(BP, objFile, textureFiles);
	}
	void cleanup() {
		skybox.cleanup();
	}
};

// UNIFORMS
struct WorldViewProjection {
    alignas(16) glm::mat4 world;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

struct Lights {
    alignas(16) glm::vec3 CamPos;
    alignas(16) glm::vec3 lightPositions[NUM_TREASURES];
	alignas(16) int numFound;
};

struct Fire {
    alignas(4) float time = 0.0f;
    alignas(16) glm::vec3 position;
};

class SoundMaze : public BaseProject {
protected:

    bool enableDebug = false;

    Object maze{};
    Object treasure{};
    FloorMap map{};
	
    Lights lights{};
	Sky sky;
    AppState appState;

    Pipe mazePipeline{};
    Pipe treasuresPipeline{};

    ALCdevice *alDevice = OpenDevice();
    ALCcontext *alContext = CreateContext(alDevice);
    
    std::uint8_t channels;
    std::int32_t sampleRate;
    std::uint8_t bitsPerSample;
    std::vector<float> soundData;
    std::vector<float> soundData2;

    ALsizei size = 10000;
    
    void setWindowParameters() {
        windowWidth = 1600;
        windowHeight = 900;
        windowTitle = "Inside the Sound Maze...";
        initialBackgroundColor = {0.0f, 0.0f, 0.0f, 1.0f};
        
        uniformBlocksInPool = 2 + 1 + 2 * NUM_TREASURES; // 2 per maze + 1 per treasure
        texturesInPool = 4 + 1 + 4 * NUM_TREASURES; // 4 per maze + 4 per treasure
        setsInPool = 2 + 1 + 2 * NUM_TREASURES; // 2 per maze + 2 per treasure
    }
    
    void loadAudio(){
        AudioFile<float> a = loadWav(AUDIO_PATH);
        std::vector<float> buffer = a.samples[0];
        for(int i = 0; i < a.getNumSamplesPerChannel(); i++){
            
            soundData.push_back(buffer[i]);
        }

        AudioFile<float> steps = loadWav(STEPS_PATH);
        std::vector<float> buffer2 = steps.samples[0];
        for(int i = 0; i < steps.getNumSamplesPerChannel(); i++){
            
            soundData2.push_back(buffer2[i]);
        }
        
        GenerateBuffer(AL_FORMAT_MONO_FLOAT32, a);
        CreateSource(&appState);
        LinkBufferToSource(&appState);
        PositionListenerInScene(0.0, 0.0, 0.0);
        StartSource(&appState);
    }
    
    bool isPlaying(int index){
          ALenum state;
         alGetSourcei(appState.sources[index], AL_SOURCE_STATE, &state);
        return (state == AL_PLAYING)
    ;}
    
    ALCdevice *OpenDevice(void) {
        ALCdevice *alDevice = alcOpenDevice(NULL);
        //CheckALError("opening the defaul AL device");
        return alDevice;
    }
    
    void CreateSource(AppState *appState) {
        alGenSources(NUM_TREASURES + 1, appState->sources);
        alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);

        for(int i = 0; i < NUM_TREASURES; i++){
            alSourcef(appState->sources[i], AL_GAIN, 0.2f);
            alSourcef(appState->sources[i], AL_MAX_DISTANCE, 2.0f * SCALE_FACTOR);
            alSourcei(appState->sources[i], AL_LOOPING, AL_TRUE);
            CheckALError("setting the AL property for gain");
        }
        alSourcef(appState->sources[NUM_TREASURES], AL_GAIN, 0.2f);

        UpdateSourceLocation(appState);
    }
    
    void UpdateSourceLocation(AppState *appState) {
        std::vector <glm::vec3> pos = map.treasuresPositions;
      
        for(int i = 0; i < NUM_TREASURES; i++){
            
            ALfloat x = pos[i].x;
            ALfloat y = pos[i].y;
            ALfloat z = pos[i].z;
            
            alSource3f(appState->sources[i], AL_POSITION, x, y, z);
            
            CheckALError("updating source location");
        }
        
      return;
    }
        
    void CheckALError(const char *operation) {
        ALenum alErr = alGetError();
      
        if (alErr == AL_NO_ERROR) {
            return;
        }
      
        const char *errFormat = NULL;
        switch (alErr) {
            case AL_INVALID_NAME:
                errFormat = "OpenAL Error: %s (AL_INVALID_NAME)";
                break;
            case AL_INVALID_VALUE:
                errFormat = "OpenAL Error: %s (AL_INVALID_VALUE)";
                break;
            case AL_INVALID_ENUM:
                errFormat = "OpenAL Error: %s (AL_INVALID_ENUM)";
                break;
            case AL_INVALID_OPERATION:
                errFormat = "OpenAL Error: %s (AL_INVALID_OPERATION)";
                break;
            case AL_OUT_OF_MEMORY:
                errFormat = "OpenAL Error: %s (AL_OUT_OF_MEMORY)";
                break;
            default:
                errFormat = "OpenAL Error: %s Unknown";
                break;
        }
        
        fprintf(stderr, errFormat, operation);
        exit(1);
    }
    
    ALCcontext * CreateContext(ALCdevice *alDevice) {
        ALCcontext *alContext = alcCreateContext(alDevice, 0);
        //CheckALError("creating AL context");
        alcMakeContextCurrent(alContext);
        //CheckALError("making the context current");
        return alContext;
    }
    
    AudioFile<float> loadWav(const char *path){
        AudioFile<float> a;
        bool loadedOK = a.load (path);
        assert (loadedOK);
        std::cout << "Bit Depth: " << a.getBitDepth() << std::endl;
        std::cout << "Sample Rate: " << a.getSampleRate() << std::endl;
        std::cout << "Num Channels: " << a.getNumChannels() << std::endl;
        std::cout << "Length in Seconds: " << a.getLengthInSeconds() << std::endl;
        std::cout << std::endl;
        return a;
    }
    
    float calculateTotalDuration(AudioFile<float> a){
        float frames = a.getLengthInSeconds() * a.getSampleRate();
        float framesToPutInBuffer = frames * SAMPLE_RATE / a.getSampleRate();
        appState.duration = framesToPutInBuffer / SAMPLE_RATE;
        //std::cout<<"n frames:  "<< framesToPutInBuffer<< std::endl;
        return framesToPutInBuffer;
    }
    
    void PositionListenerInScene(float x, float y, float z) {
        alListener3f(AL_POSITION, x, y, z);
        CheckALError("setting the listener position");
    }
    
    void LinkBufferToSource(AppState *appState) {
        for(int i = 0; i < NUM_TREASURES; i++){
            alSourcei(appState->sources[i], AL_BUFFER, appState->buffers[0]);
            CheckALError("setting the buffer to the source");
        }
        alSourcei(appState->sources[NUM_TREASURES], AL_BUFFER, appState->buffers[1]);
    }
    
    void StartSource(AppState *appState) {
        for(int i = 0; i < NUM_TREASURES; i++){
          alSourcePlay(appState->sources[i]);
          CheckALError("starting the source");
        }
    }
    
    
    void StopSource(AppState *appState) {
        for(int i = 0; i < NUM_TREASURES; i++){
          alSourceStop(appState->sources[i]);
          CheckALError("stopping the source");
        }
    }
    
    void ReleaseResources(AppState *appState, ALCdevice *alDevice, ALCcontext *alContext) {
        alDeleteSources(NUM_TREASURES + 1, appState->sources);
        alDeleteBuffers(2, appState->buffers);
        alcDestroyContext(alContext);
        alcCloseDevice(alDevice);
    }
    
    void GenerateBuffer(ALenum format, AudioFile<float> a){
        alGenBuffers(2, &appState.buffers[0]);
        alBufferData(appState.buffers[0], format, soundData.data(), calculateTotalDuration(a), SAMPLE_RATE);
        alBufferData(appState.buffers[1], format, soundData2.data(), calculateTotalDuration(a), SAMPLE_RATE);
    }
    
    void localInit() {
        // Load the map and skybox
        {
            map.init("textures/high-constrast-maze-map.png", window);
            assert(map.width > 0);
            assert(map.height > 0);

            sky.init(this, MODEL_PATH_SKYBOX, 
				{
				SKYBOX_RIGHT,
				SKYBOX_LEFT,
				SKYBOX_TOP,
				SKYBOX_BOTTOM,
				SKYBOX_FRONT,
				SKYBOX_BACK
				});
        }

        // Load the models
        {
            maze.model.init(this, MODEL_PATH_MAZE);
            maze.albedo_map.init(this, TEXTURE_PATH_MAZE_Alb);
            maze.metallic_map.init(this, TEXTURE_PATH_MAZE_Ref);
            maze.roughness_map.init(this, TEXTURE_PATH_MAZE_Rou);
            maze.light_map.init(this, TEXTURE_PATH_MAZE_Ao);
            assert(maze.model.indices.size() > 0);

            treasure.model.init(this, MODEL_PATH_TREASURE);
            treasure.albedo_map.init(this, TEXTURE_PATH_TREASURE_base);
            treasure.metallic_map.init(this, TEXTURE_PATH_TREASURE_base);
            treasure.roughness_map.init(this, TEXTURE_PATH_TREASURE_base);
            treasure.light_map.init(this, TEXTURE_PATH_TREASURE_base);
            assert(treasure.model.indices.size() > 0);
        }
        
		// Initialize Pipelines (the pipeline for skybox is initialized automatically)        
        {
            
            // Maze
            mazePipeline.dsls.push_back(DescriptorSetLayout{});
            mazePipeline.dsls[0].init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1} // MVP
                });

            mazePipeline.dsls.push_back(DescriptorSetLayout{});
            mazePipeline.dsls[1].init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1}, // Lights
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1}, // Albedo
                {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1}, // Metallic
                {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1}, // Roughness
                {4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1}, // AO
                });

			mazePipeline.pipeline.init(this, 
				"shaders/graphics_maze_vert.spv", "shaders/graphics_maze_frag.spv", 
				{ 
					&mazePipeline.dsls[0],
					&mazePipeline.dsls[1]
				});
			
			mazePipeline.dss.push_back(DescriptorSet{});
			mazePipeline.dss[0].init(this, &mazePipeline.dsls[0], {
				{0, UNIFORM, sizeof(WorldViewProjection), nullptr}
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
				{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1} // MVP
				});
			
			treasuresPipeline.dsls.push_back(DescriptorSetLayout{});
			treasuresPipeline.dsls[1].init(this, {
				{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1}
				});
			
			treasuresPipeline.pipeline.init(this,
				"shaders/graphics_treasures_vert.spv", "shaders/graphics_treasures_frag.spv",
				{
					&treasuresPipeline.dsls[0],
					&treasuresPipeline.dsls[1]
				});

			for (int i = 0; i < NUM_TREASURES; i++) {
				treasuresPipeline.dss.push_back(DescriptorSet{});
				treasuresPipeline.dss[i].init(this, &treasuresPipeline.dsls[0], {
					{0, UNIFORM, sizeof(WorldViewProjection), nullptr}
					});
			}
			
			for (int i = 0; i < NUM_TREASURES; i++) {
				treasuresPipeline.dss.push_back(DescriptorSet{});
				treasuresPipeline.dss[NUM_TREASURES + i].init(this, &treasuresPipeline.dsls[1], {
					{0, UNIFORM, sizeof(Fire), nullptr}
					});
			}
			
			std::cout << "Treasures Pipeline Initialized" << std::endl;
		}

        loadAudio();

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
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
				mazePipeline.pipeline.pipelineLayout, firstSet, descriptorSetCount, 
				&mazePipeline.dss[firstSet].descriptorSets[currentImage],
				0, nullptr);
			VkBuffer vertexBuffers[] = { maze.model.vertexBuffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(commandBuffer, maze.model.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(maze.model.indices.size()), 1, 0, 0, 0);
		}
		
		{
			//PIPELINE SKYBOX
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, sky.skybox.skyBoxPipeline.graphicsPipeline);
			VkBuffer vertexBuffersSk[] = { sky.skybox.skyboxModel.vertexBuffer };
			VkDeviceSize offsetsSk[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffersSk, offsetsSk);
			vkCmdBindIndexBuffer(commandBuffer, sky.skybox.skyboxModel.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				sky.skybox.skyBoxPipeline.pipelineLayout, 0, 1,
				&sky.skybox.SkyBoxDescriptorSets[currentImage],
				0, nullptr);
			vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(sky.skybox.skyboxModel.indices.size()), 1, 0, 0, 0);
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
        float move_speed = 0.0;
        
        if (glfwGetKey(window, GLFW_KEY_I)) { enableDebug = !enableDebug; }
        
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)) {
			move_speed = MOVE_SPEED * 2;
		}
        else {
            move_speed = MOVE_SPEED;
        }
        
        if (glfwGetKey(window, GLFW_KEY_A)) {
            newCamPos -= move_speed * glm::vec3(glm::rotate(glm::mat4(1.0f), CamAng.y,
                glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(1, 0, 0, 1)) * deltaT;
            if(!isPlaying(NUM_TREASURES)){
                alSource3f(appState.sources[NUM_TREASURES], AL_POSITION, newCamPos.x, newCamPos.y, newCamPos.z);
                alSourcePlay(appState.sources[NUM_TREASURES]);
            }
            if (!enableDebug) {
                if (map.isWallAround(newCamPos.x, newCamPos.z)) {
                    alSourceStop(appState.sources[NUM_TREASURES]);
                    *CamPos = oldCamPos;
                    return;
                }
            }
        }
        
        if (glfwGetKey(window, GLFW_KEY_D)) {
            newCamPos += move_speed * glm::vec3(glm::rotate(glm::mat4(1.0f), CamAng.y,
                glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(1, 0, 0, 1)) * deltaT;
            
            if(!isPlaying(NUM_TREASURES)){
                alSource3f(appState.sources[NUM_TREASURES], AL_POSITION, newCamPos.x, newCamPos.y, newCamPos.z);
                alSourcePlay(appState.sources[NUM_TREASURES]);
            }

            if (!enableDebug) {
                if (map.isWallAround(newCamPos.x, newCamPos.z)) {
                    alSourceStop(appState.sources[NUM_TREASURES]);
                    *CamPos = oldCamPos;
                    return;
                }
            }
        }
        
        if (glfwGetKey(window, GLFW_KEY_S)) {
            newCamPos += move_speed * glm::vec3(glm::rotate(glm::mat4(1.0f), CamAng.y,
                glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 0, 1, 1)) * deltaT;
            
            if(!isPlaying(NUM_TREASURES)){
                alSource3f(appState.sources[NUM_TREASURES], AL_POSITION, newCamPos.x, newCamPos.y, newCamPos.z);
                alSourcePlay(appState.sources[NUM_TREASURES]);
            }

            if (!enableDebug) {
                if (map.isWallAround(newCamPos.x, newCamPos.z)) {
                    alSourceStop(appState.sources[NUM_TREASURES]);
                    *CamPos = oldCamPos;
                    return;
                }
            }
        }
        
        if (glfwGetKey(window, GLFW_KEY_W)) {
            newCamPos -= move_speed * glm::vec3(glm::rotate(glm::mat4(1.0f), CamAng.y,
                glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 0, 1, 1)) * deltaT;

            if(!isPlaying(NUM_TREASURES)){
                alSource3f(appState.sources[NUM_TREASURES], AL_POSITION, newCamPos.x, newCamPos.y, newCamPos.z);
                alSourcePlay(appState.sources[NUM_TREASURES]);
            }

            if (!enableDebug) {
                if (map.isWallAround(newCamPos.x, newCamPos.z)) {
                    alSourceStop(appState.sources[NUM_TREASURES]);
                    *CamPos = oldCamPos;
                    return;
                }
            }
        }

        if (enableDebug) {
            if (glfwGetKey(window, GLFW_KEY_C)) {
                newCamPos += move_speed * glm::vec3(glm::translate(
                    glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 1, 0, 1)) * deltaT;
            }
            if (glfwGetKey(window, GLFW_KEY_X)) {
                newCamPos -= move_speed * glm::vec3(glm::translate(
                    glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 1, 0, 1)) * deltaT;
            }
        }
        
        if (newCamPos == oldCamPos){ alSourceStop(appState.sources[NUM_TREASURES]); }
        else *CamPos = newCamPos;
        
    }
    
    WorldViewProjection computeWVP(glm::vec3 camAng, glm::vec3 camPos, glm::vec3 pos = glm::vec3(0.0f)) {
        WorldViewProjection wvp{};
        glm::mat3 CamView = glm::mat3(glm::rotate(glm::mat4(1.0f), camAng.y, glm::vec3(0.0f, 1.0f, 0.0f))) *
            glm::mat3(glm::rotate(glm::mat4(1.0f), camAng.x, glm::vec3(1.0f, 0.0f, 0.0f))) *
            glm::mat3(glm::rotate(glm::mat4(1.0f), camAng.z, glm::vec3(0.0f, 0.0f, 1.0f)));

        wvp.world = glm::scale(
            glm::translate(glm::mat4(1.0), pos),
            glm::vec3(SCALE_FACTOR));
        wvp.view = glm::translate(glm::transpose(glm::mat4(CamView)), -camPos);
        wvp.proj = glm::perspective(FOVY, swapChainExtent.width / (float)swapChainExtent.height, NEAR_FIELD, FAR_FIELD);
        wvp.proj[1][1] *= -1;
        return wvp;
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
		static glm::vec3 CamPos = glm::vec3(0.0f, .1f, 0.0f);
		updateCameraAngles(&camAng, deltaT, ROT_SPEED);
		updateCameraPosition(&CamPos, deltaT, MOVE_SPEED, camAng);
        PositionListenerInScene(CamPos.x, CamPos.y, CamPos.z);
        map.checkPosition(CamPos, appState);
        
		WorldViewProjection wvp;
		// Maze Pipeline
		{
			wvp = computeWVP(camAng, CamPos, glm::vec3(0.0f, -0.1f, 0.0f));
            if (map.treasuresFound == NUM_TREASURES) {
                static auto endtime = std::chrono::high_resolution_clock::now();
                auto now = std::chrono::high_resolution_clock::now();
                auto countdown = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - endtime).count();
                if (countdown >= 15.f) {
                    wvp.world = glm::mat4(0.0f);
                }
                else {
                    wvp.world = glm::rotate(
						    wvp.world,
                            countdown * countdown * 1.2f,
                            glm::vec3(0.0f, 1.0f, 0.0f)
                        );
                }
                wvp.view = glm::translate(wvp.view, CamPos);
            }
			vkMapMemory(device, mazePipeline.dss[0].uniformBuffersMemory[0][currentImage], 0, sizeof(wvp), 0, &data);
			memcpy(data, &wvp, sizeof(wvp));
			vkUnmapMemory(device, mazePipeline.dss[0].uniformBuffersMemory[0][currentImage]);
		
			lights.CamPos = CamPos;
			lights.numFound = map.treasuresFound;
			for (int i = 0; i < NUM_TREASURES; i++) {
				lights.lightPositions[i] = map.treasuresPositions[i];
			}
			vkMapMemory(device, mazePipeline.dss[1].uniformBuffersMemory[0][currentImage], 0, sizeof(lights), 0, &data);
			memcpy(data, &lights, sizeof(lights));
			vkUnmapMemory(device, mazePipeline.dss[1].uniformBuffersMemory[0][currentImage]);
		}

		// Treasures Pipeline
		{
			Fire f;
			for (int i = 0; i < NUM_TREASURES; i++) {
			    f.time = time;
                f.position = map.treasuresPositions[i];
				wvp = computeWVP(camAng, CamPos, map.treasuresPositions[i]);
                if (glm::length(map.treasuresPositions[i]) < 0.01f) wvp.world = glm::mat4(0.0f);
				vkMapMemory(device, treasuresPipeline.dss[i].uniformBuffersMemory[0][currentImage], 0, sizeof(wvp), 0, &data);
				memcpy(data, &wvp, sizeof(wvp));
				vkUnmapMemory(device, treasuresPipeline.dss[i].uniformBuffersMemory[0][currentImage]);
			
				vkMapMemory(device, treasuresPipeline.dss[NUM_TREASURES + i].uniformBuffersMemory[0][currentImage], 0, sizeof(f), 0, &data);
				memcpy(data, &f, sizeof(f));
				vkUnmapMemory(device, treasuresPipeline.dss[NUM_TREASURES + i].uniformBuffersMemory[0][currentImage]);
			}
		}

        // Skybox
		{
			float aspect_ratio = swapChainExtent.width / (float)swapChainExtent.height;

			glm::mat4 CamMat = wvp.view;
			glm::mat4 out = glm::perspective(glm::radians(90.0f), aspect_ratio, 0.1f, 100.0f);
			out[1][1] *= -1;

			GlobalUniformBufferObjectSkybox guboSky{};
			guboSky.mMat = glm::mat4(1.0f);
			guboSky.nMat = glm::mat4(1.0f);
			guboSky.mvpMat = out * glm::mat4(glm::mat3(CamMat)) * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.15f, 0.0f));
		
			vkMapMemory(device, sky.skybox.SkyBoxUniformBuffersMemory[currentImage], 0, sizeof(guboSky), 0, &data);
			memcpy(data, &guboSky, sizeof(guboSky));
			vkUnmapMemory(device, sky.skybox.SkyBoxUniformBuffersMemory[currentImage]);
		}
    }

    void localCleanup() {
	    map.cleanup();
		maze.cleanup();
		sky.cleanup();
		treasure.cleanup();
		mazePipeline.cleanup();
		treasuresPipeline.cleanup();
        ReleaseResources(&appState, alDevice , alContext);
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
