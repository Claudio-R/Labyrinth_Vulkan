/* TODO:
 * #ok. Correctly upload the map
 * #ok Fix the map
 * #ok Correctly implement collisions
 * #ok Fix shaders
 * #ok Add lights
 * #ok Add treasures
 * #ok Improve textures
 * #ok Fix map to have it easier to work with --> should be fixed even more
 * # Add sound
 */

#include "SoundMaze.hpp"

const std::string MODEL_PATH_MAZE = "models/maze.obj";
const std::string MODEL_PATH_TREASURE = "models/icosphere.obj";

const std::string TEXTURE_PATH_MAZE_Alb = "textures/maze_albedo.jpg";
const std::string TEXTURE_PATH_MAZE_Ref = "textures/maze_metallic.jpg";
const std::string TEXTURE_PATH_MAZE_Rou = "textures/maze_roughness.jpg";
const std::string TEXTURE_PATH_MAZE_Ao = "textures/maze_ao.jpg";

const std::string TEXTURE_PATH_TREASURE_base = "textures/treasure_baseColor.png";
const std::string TEXTURE_PATH_TREASURE_Ref = "textures/treasure_metallic.png";
const std::string TEXTURE_PATH_TREASURE_Rou = "textures/treasure_roughness.png";
const std::string TEXTURE_PATH_TREASURE_Em = "textures/treasure_metallic.png";
const char *AUDIO_PATH = "audio/148.wav";

constexpr float MODEL_DIAMETER = 10.0f;
constexpr float TREASURE_DIAMETER = 0.1f;
constexpr float NUM_TREASURES = 10;
constexpr float ROT_SPEED = glm::radians(60.0f);
const float MOVE_SPEED = 0.5f;
const float MOUSE_RES = 500.0f;
constexpr float FOVY = glm::radians(60.0f);
const float NEAR_FIELD = 0.001f;
const float FAR_FIELD = 20.0f;
const float SAMPLE_RATE = 44100.0f;
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

struct AppState{
    ALuint sources[(int)NUM_TREASURES];
    ALuint buffers[1];
    
    double duration;
};

struct FloorMap {
    stbi_uc* img;
    int width;
    int height;
    std::vector <glm::vec3> treasuresPositions;
    
    void init(const char* url) {
        loadImg(url);
        generateRandomTreasuresPositions();
    }
    
    std::vector <glm::vec3> getTreasuresPositions(){
        
        return treasuresPositions;
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

            bool valid = true;
            for (glm::vec3 pos : treasuresPositions) {
                if (glm::distance(pos, glm::vec3(candidate.x, 0.0f, candidate.y)) < 10 * TREASURE_DIAMETER) {
                    valid = false;
                }
            }
            if (!valid) { continue; }
            if (getPixel(candidate.x, candidate.y) == 0) { continue; }
            if (isWallAround(candidate.x, candidate.y, 4 * TREASURE_DIAMETER)) { continue; }

            treasuresPositions.push_back(glm::vec3(candidate.x, 0.0f, candidate.y));
        }
    }

    bool isWallAround(float center_x, float center_y, float radius = 0.1f) {
        const int steps = 8;
        float r = 0.0f; // will check again the same position
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
        glm::vec3(- 0.1, 0.1, - 0.1),
        glm::vec3(- 0.1, 0.1, + 0.1),
        glm::vec3(+ 0.1, 0.1, - 0.1),
        glm::vec3(+ 0.1, 0.1, + 0.1)
    };
    
    glm::vec3 lightColors[4] = {
        glm::vec3(1, 0, 0),
        glm::vec3(0, 1, 0),
        glm::vec3(0, 0, 1),
        glm::vec3(1, 1, 1)
    };
};

struct GlobalUniformBufferObject {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
};

// UBO for Skybox
struct UniformBufferObjectSkybox {
    alignas(16) glm::mat4 mvpMat;
    alignas(16) glm::mat4 mMat;
    alignas(16) glm::mat4 nMat;
};

// Skybox
struct SkyBoxModel {
    const char* ObjFile;
    const char* TextureFile[6];
};

const SkyBoxModel SkyBoxToLoad = {
    "models/SkyBoxCube.obj",
    {
        "textures/sky/right.png", "textures/sky/left.png", "textures/sky/top.png",
        "textures/sky/bottom.png", "textures/sky/front.png", "textures/sky/back.png"
    }
};

class SoundMaze : public BaseProject {
protected:

    bool enableDebug = false;
    
    //Custom pipeline for skybox
    Pipeline skyBoxPipeline;
    DescriptorSetLayout skyBoxDSL;
    std::vector<VkBuffer> SkyBoxUniformBuffers;
    std::vector<VkDeviceMemory> SkyBoxUniformBuffersMemory;
    std::vector<VkDescriptorSet> SkyBoxDescriptorSets;    // to access uniforms in the pipeline
    
    Model M_maze;
    Texture T_maze;
    DescriptorSet DS_maze;
    Model skyBox;    // Skybox
    Texture skyBoxTexture;
    FloorMap map{};
    Object maze{};
    Object treasure{};
    AppState appState;

    Pipe mazePipeline{};
    Pipe treasuresPipeline{};
    
    ALCdevice *alDevice = OpenDevice();
    ALCcontext *alContext = CreateContext(alDevice);
    
    std::uint8_t channels;
    std::int32_t sampleRate;
    std::uint8_t bitsPerSample;
    std::vector<float> soundData;
    ALsizei size = 10000;
    
    void setWindowParameters() {
        windowWidth = 1200;
        windowHeight = 800;
        windowTitle = "Inside the Sound Maze...";
        initialBackgroundColor = {0.0f, 0.0f, 0.0f, 1.0f};
        
        // Non puÚ essere spostata dentro init
        uniformBlocksInPool = 2 + 1 + 2 * NUM_TREASURES; // 2 per maze + 1 per treasure
        texturesInPool = 4 + 6 +  4 * NUM_TREASURES; // 4 per maze + 4 per treasure
        setsInPool = 2 + 1 + 2 * NUM_TREASURES; // 2 per maze + 2 per treasure
    }
    
    void loadAudio(){
        
    
        AudioFile<float> a = loadWav(AUDIO_PATH);
        
        std::vector<float> buffer = a.samples[0];
        
        for(int i = 0; i < a.getNumSamplesPerChannel(); i++){
            
            soundData.push_back(buffer[i]);
        }
        
        ALenum format;
        if(a.getNumChannels() == 1 && a.getBitDepth() == 8)
                format = AL_FORMAT_MONO8;
            else if(a.getNumChannels() == 1 && a.getBitDepth() == 16)
                format = AL_FORMAT_MONO16;
            else if(a.getNumChannels() == 2 && a.getBitDepth() == 8)
                format = AL_FORMAT_STEREO8;
            else if(a.getNumChannels() == 2 && a.getBitDepth() == 16)
                format = AL_FORMAT_STEREO16;
            else
        {
            std::cerr
                << "ERROR: unrecognised wave format: "
                << channels << " channels, "
                << bitsPerSample << " bps" << std::endl;
        }
        GenerateBuffer(format, a);
        
        CreateSource(&appState);
        
        LinkBufferToSource(&appState);
        
        PositionListenerInScene(0.0, 0.0, 0.0);
        
        StartSource(&appState);
        
    }
    
    ALCdevice *OpenDevice(void) {
      ALCdevice *alDevice = alcOpenDevice(NULL);
      //CheckALError("opening the defaul AL device");

      return alDevice;
    }
    
    void CreateSource(AppState *appState) {
      alGenSources(NUM_TREASURES, appState->sources);
      alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);

      for(int i = 0; i < NUM_TREASURES; i++){
        
          alSourcef(appState->sources[i],
                    AL_GAIN,
                    0.5f);
          alSourcef(appState->sources[i], AL_MAX_DISTANCE, 5.0f);
          alSourcei(appState->sources[i], AL_LOOPING, AL_TRUE);
          
          CheckALError("setting the AL property for gain");
        }
      
      UpdateSourceLocation(appState);
    }
    
    void UpdateSourceLocation(AppState *appState) {
        std::vector <glm::vec3> pos = map.getTreasuresPositions();
      
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
      
      char *errFormat = NULL;
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
        
        return framesToPutInBuffer;
    }
    
    void PositionListenerInScene(float x, float y, float z) {
      alListener3f(AL_POSITION, x, y, z);
        std::cout<<"LIstener position x : " << x << std::endl;
        std::cout<<"LIstener position y : " << y << std::endl;
        std::cout<<"LIstener position z : " << z << std::endl;
      CheckALError("setting the listener position");
    }
    
    void LinkBufferToSource(AppState *appState) {
        for(int i = 0; i < NUM_TREASURES; i++){
          alSourcei(appState->sources[i], AL_BUFFER, appState->buffers[0]);
          CheckALError("setting the buffer to the source");
            }
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
    
    void ReleaseResources(AppState *appState,
                          ALCdevice *alDevice,
                          ALCcontext *alContext) {
      alDeleteSources(NUM_TREASURES, appState->sources);
      alDeleteBuffers(1, appState->buffers);
      alcDestroyContext(alContext);
      alcCloseDevice(alDevice);
    }
    
    void GenerateBuffer(ALenum format, AudioFile<float> a){
        
        alGenBuffers(1, &appState.buffers[0]);

        alBufferData(appState.buffers[0], format, soundData.data(), calculateTotalDuration(a) , SAMPLE_RATE);
        
    }
    
    void localInit() {
        // Load the map
        {
            map.init("textures/high-constrast-maze-map.png");
            //generateRandomTreasuresTranslations();
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
        skyBoxDSL.initSkybox(this);

        skyBoxPipeline.init(this, "shaders/SkyBoxVert.spv", "shaders/SkyBoxFrag.spv", { &skyBoxDSL });
        loadSkyBox();
            
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
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1} // MVP
                });
            
            treasuresPipeline.dsls.push_back(DescriptorSetLayout{});
            treasuresPipeline.dsls[1].init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1},
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1},
                {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1},
                {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1},
                {4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1},
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
                
        {    //PIPELINE SKYBOX
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        skyBoxPipeline.graphicsPipeline);
        VkBuffer vertexBuffersSk[] = { skyBox.vertexBuffer};
        VkDeviceSize offsetsSk[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffersSk, offsetsSk);
        vkCmdBindIndexBuffer(commandBuffer, skyBox.indexBuffer, 0,
        VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        skyBoxPipeline.pipelineLayout, 0, 1,
        &SkyBoxDescriptorSets[currentImage],
        0, nullptr);
        vkCmdDrawIndexed(commandBuffer,
         static_cast<uint32_t>(skyBox.indices.size()), 1, 0, 0, 0);
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

        if (glfwGetKey(window, GLFW_KEY_I)) {
            enableDebug = !enableDebug;
        }

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

        if (enableDebug) {
            if (glfwGetKey(window, GLFW_KEY_C)) {
                newCamPos += MOVE_SPEED * glm::vec3(glm::translate(
                    glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 1, 0, 1)) * deltaT;
            }
            if (glfwGetKey(window, GLFW_KEY_X)) {
                newCamPos -= MOVE_SPEED * glm::vec3(glm::translate(
                    glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 1, 0, 1)) * deltaT;
            }
        }

        if (!enableDebug) {
            if (map.isWallAround(newCamPos.x, newCamPos.z)) {
                *CamPos = oldCamPos;
                return;
            }
        }
        *CamPos = newCamPos;
        PositionListenerInScene(CamPos->x, CamPos->y, CamPos->z);
    }
    
    ModelViewProjection computeMVP(glm::vec3 camAng, glm::vec3 camPos, glm::vec3 pos = glm::vec3(0.0f)) {

        ModelViewProjection mvp{};
        glm::mat3 CamDir = glm::mat3(glm::rotate(glm::mat4(1.0f), camAng.y, glm::vec3(0.0f, 1.0f, 0.0f))) *
            glm::mat3(glm::rotate(glm::mat4(1.0f), camAng.x, glm::vec3(1.0f, 0.0f, 0.0f))) *
            glm::mat3(glm::rotate(glm::mat4(1.0f), camAng.z, glm::vec3(0.0f, 0.0f, 1.0f)));

        glm::mat4 CamMat = glm::translate(glm::transpose(glm::mat4(CamDir)), -camPos);

        mvp.model = glm::translate(
                    glm::mat4(1.0),
                    -pos
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
        //ASPECT RATIO
        float aspect_ratio = swapChainExtent.width / (float)swapChainExtent.height;
        
        static glm::vec3 camAng = glm::vec3(0.0f);
        static glm::vec3 camPos = glm::vec3(0.0f, .1f, 0.0f);
        updateCameraAngles(&camAng, deltaT, ROT_SPEED);
        updateCameraPosition(&camPos, deltaT, MOVE_SPEED, camAng);
        
        ModelViewProjection mvp;
        Lights l;

        // Maze Pipeline
        {
            mvp = computeMVP(camAng, camPos, glm::vec3(0.0f, 0.1f, 0.0f));
            vkMapMemory(device, mazePipeline.dss[0].uniformBuffersMemory[0][currentImage], 0, sizeof(mvp), 0, &data);
            memcpy(data, &mvp, sizeof(mvp));
            vkUnmapMemory(device, mazePipeline.dss[0].uniformBuffersMemory[0][currentImage]);
        
            vkMapMemory(device, mazePipeline.dss[1].uniformBuffersMemory[0][currentImage], 0, sizeof(l), 0, &data);
            memcpy(data, &l, sizeof(l));
            vkUnmapMemory(device, mazePipeline.dss[1].uniformBuffersMemory[0][currentImage]);
        }

        // SKYBOX

        //CAMERA VIEW MATRIX
        GlobalUniformBufferObject guboObj{};
        glm::mat4 CamMat = mvp.view;
        guboObj.view = CamMat;
        //CAMERA PROJECTION MATRIX
        glm::mat4 out = glm::perspective(glm::radians(90.0f), aspect_ratio, 0.1f, 100.0f);
        out[1][1] *= -1;
        guboObj.proj = out;
        
        UniformBufferObjectSkybox uboSky{};
        uboSky.mMat = glm::mat4(1.0f);
        uboSky.nMat = glm::mat4(1.0f);
        uboSky.mvpMat = out * glm::mat4(glm::mat3(CamMat)) * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.15f, 0.0f));
        // SkyBox uniforms
        vkMapMemory(device, SkyBoxUniformBuffersMemory[currentImage], 0,
        sizeof(uboSky), 0, &data);
        memcpy(data, &uboSky, sizeof(uboSky));
        vkUnmapMemory(device, SkyBoxUniformBuffersMemory[currentImage]);
        
        // Treasures Pipeline
        {
            for (int i = 0; i < NUM_TREASURES; i++) {
                mvp = computeMVP(camAng, camPos, map.treasuresPositions[i]);
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
        skyBoxDSL.cleanup();
        skyBoxPipeline.cleanup();
        skyBox.cleanup();
        skyBoxTexture.cleanup();
        for (size_t i = 0; i < swapChainImages.size(); i++) {
        vkDestroyBuffer(device, SkyBoxUniformBuffers[i], nullptr);
        vkFreeMemory(device, SkyBoxUniformBuffersMemory[i], nullptr);
        }
        mazePipeline.cleanup();
        treasuresPipeline.cleanup();
        ReleaseResources(&appState, alDevice , alContext);
    }

    // Skybox aux functions
    void createCubicTextureImage(const char* const FName[6], Texture& TD) {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels[6];
        for (int i = 0; i < 6; i++) {
            pixels[i] = stbi_load(FName[i], &texWidth, &texHeight,
            &texChannels, STBI_rgb_alpha);
            if (!pixels[i]) {
                std::cout << FName[i]<< "\n";
                throw std::runtime_error("failed to load texture image!");
            }
            std::cout << FName[i] << " -> size: " << texWidth
            << "x" << texHeight << ", ch: " << texChannels << "\n";
        }
        
        VkDeviceSize imageSize = texWidth * texHeight * 4;
        VkDeviceSize totalImageSize = texWidth * texHeight * 4 * 6;
        TD.mipLevels = static_cast<uint32_t>(std::floor(
        std::log2(std::max(texWidth, texHeight)))) + 1;
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        
        createBuffer(totalImageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingBufferMemory);
            void* data;
            
        vkMapMemory(device, stagingBufferMemory, 0, totalImageSize, 0, &data);
        for (int i = 0; i < 6; i++) {
            memcpy(static_cast<char*>(data) + imageSize * i, pixels[i], static_cast<size_t>(imageSize));
        }
        
        vkUnmapMemory(device, stagingBufferMemory);
        for (int i = 0; i < 6; i++) {
        stbi_image_free(pixels[i]);
        }

        createSkyBoxImage(texWidth, texHeight, TD.mipLevels, TD.textureImage,
        TD.textureImageMemory);
        transitionImageLayout(TD.textureImage, VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, TD.mipLevels, 6);
            copyBufferToImage(stagingBuffer, TD.textureImage,
            static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 6);
            generateMipmaps(TD.textureImage, VK_FORMAT_R8G8B8A8_SRGB,
            texWidth, texHeight, TD.mipLevels, 6);
            vkDestroyBuffer(device, stagingBuffer, nullptr);
            vkFreeMemory(device, stagingBufferMemory, nullptr);
    }
    
    void createSkyBoxImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkImage& image, VkDeviceMemory& imageMemory) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 6;
        imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        VkResult result = vkCreateImage(device, &imageInfo, nullptr, &image);
        
        if (result != VK_SUCCESS) {
            PrintVkError(result);
            throw std::runtime_error("failed to create image!");
        }
        
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, image, &memRequirements);
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory!");
        }
        vkBindImageMemory(device, image, imageMemory, 0);
    }
    
    void createSkyBoxImageView(Texture& TD) {
        TD.textureImageView = createImageView(TD.textureImage,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_ASPECT_COLOR_BIT,
        TD.mipLevels,
        VK_IMAGE_VIEW_TYPE_CUBE, 6);
    }
    
    void createSkyBoxDescriptorSets() {
        std::vector<VkDescriptorSetLayout> layouts(swapChainImages.size(),
        skyBoxDSL.descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
        allocInfo.pSetLayouts = layouts.data();
        SkyBoxDescriptorSets.resize(swapChainImages.size());
        VkResult result = vkAllocateDescriptorSets(device, &allocInfo,
        SkyBoxDescriptorSets.data());
        if (result != VK_SUCCESS) {
            PrintVkError(result);
            throw std::runtime_error("failed to allocate Skybox descriptor sets!");
        }
        
        for (size_t k = 0; k < swapChainImages.size(); k++) {
            
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = SkyBoxUniformBuffers[k];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = skyBoxTexture.textureImageView;
            imageInfo.sampler = skyBoxTexture.textureSampler;
            std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = SkyBoxDescriptorSets[k];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;
            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = SkyBoxDescriptorSets[k];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType =
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;
            vkUpdateDescriptorSets(device,
            static_cast<uint32_t>(descriptorWrites.size()),
            descriptorWrites.data(), 0, nullptr);
        }
    }
    
    void loadSkyBox() {
        skyBox.init(this, SkyBoxToLoad.ObjFile);
        createCubicTextureImage(SkyBoxToLoad.TextureFile, skyBoxTexture);
        createSkyBoxImageView(skyBoxTexture);
        skyBoxTexture.BP = this;
        skyBoxTexture.createTextureSampler();
        // Skybox createUniformBuffers
        VkDeviceSize bufferSize = sizeof(UniformBufferObjectSkybox);
        SkyBoxUniformBuffers.resize(swapChainImages.size());
        SkyBoxUniformBuffersMemory.resize(swapChainImages.size());
        for (size_t i = 0; i < swapChainImages.size(); i++) {
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        SkyBoxUniformBuffers[i], SkyBoxUniformBuffersMemory[i]);
        }
        // Skybox descriptor sets
        createSkyBoxDescriptorSets();
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
