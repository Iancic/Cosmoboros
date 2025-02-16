#pragma once
#include "Program.h"

// Game Specific Header Files Only
#include "common.h"
#include "UserInterface.h"
#include "Star.h"
#include "Creature.h"
#include "Player.h"

class Game : public Program
{
public:
	Game();
	~Game();

	void Init(const char* title, int x, int y, int width, int height, bool fullscreen);
	void Update();
	void Render();
	void HandleEvents();
	void Quit();
	void Time();

	void UserInterfaceUpdate(Creature* creatureArg);

	bool Running() { return isRunning; };

private:

	enum class GameState
	{
		GAME_ACTIVE,
		GAME_PAUSED,
	};

	glm::vec2 galaxyOffset = { 0, 0 };
	glm::vec2 screenSector = { 0, 0 }; // Coordinates of the sector

	ResourceManager* Resources = ResourceManager::getInstance();

	FrameBuffer* RenderingBuffer = nullptr;

	SDL_GLContext glContext;
	SDL_Window* window;

	UserInterface* UI = UserInterface::getInstance();

	Transform* cameraTarget = new Transform();
	Transform weaponsTarget{};

	Camera* ActiveCamera = nullptr;

	Player* playerPtr = nullptr;

	std::vector<Creature*> creaturePool;
	unordered_map< uint32_t, Star*> starCache;

	const Uint8* keyState = SDL_GetKeyboardState(NULL);
	bool keyPressedOnce[SDL_NUM_SCANCODES] = { false };
	bool keyPressed[SDL_NUM_SCANCODES] = { false };

	int milisecondsPreviousFrame;
	double deltaTime;
	float timeElapsed;

	int counterLeft = 0, counterRight = 0, counterTop = 0, counterBot = 0;

	int sectorOffset = 1;
	int nSectorsX = SCREEN_WIDTH / SECTOR_WIDTH;
	int nSectorsY = SCREEN_HEIGHT / SECTOR_HEIGHT;

	SDL_Joystick* joystickPlayer1 = nullptr;
	const int deadZone = 1500;
	int xDirection = 0;
	int yDirection = 0;

	bool isRunning;

};