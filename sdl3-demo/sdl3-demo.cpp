// sdl-demo.cpp : Defines he entry point for the application.
//

#include <SDL3/SDL.h>
//in order to init SDL it needs to rename our main function, replace it with its main function, 
//init some setup and then recall our main to start program
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include "gameObject.h"
#include <array>
#include <vector>
#include <string>
#include <format>
using namespace std;

//hold important SDL objects in state to make cleanup and init more efficient by just passing through single SDL state object instead of passing SDL objects to SDL state
struct SDLState
{
	SDL_Window* window;
	SDL_Renderer* renderer;
	int width, height, logicalWidth, logicalHeight;
	const bool* keys;

	SDLState() : keys(SDL_GetKeyboardState(nullptr))
	{
	}
};


const size_t LAYER_IDX_LEVEL = 0;
const size_t LAYER_IDX_CHARACTERS = 1;
const int MAP_ROWS = 5;
const int MAP_COLS = 50;
const int TILE_SIZE = 32;

struct GameState
{
	std::array<std::vector<GameObject>, 2> layers;
	std::vector<GameObject> backgroundTiles;
	std::vector<GameObject> foregroundTiles;
	std::vector<GameObject> bullets;

	int playerIndex;
	SDL_FRect mapViewport;
	float bg2Scroll, bg3Scroll, bg4Scroll;

	GameState(const SDLState& state)
	{
		playerIndex = -1;
		mapViewport = SDL_FRect{
			.x = 0,
			.y = 0,
			.w = static_cast<float>(state.logicalWidth),
			.h = static_cast<float>(state.logicalHeight)
		};
		bg2Scroll = bg3Scroll = bg4Scroll = 0;
	}
	GameObject& player() { return layers[LAYER_IDX_CHARACTERS][playerIndex]; }
};
struct Resources
{
	const int ANIM_PLAYER_IDLE = 0;
	const int ANIM_PLAYER_RUN = 1;
	const int ANIM_PLAYER_JUMP = 2;
	const int ANIM_PLAYER_SLIDE = 3;
	std::vector<Animation> playerAnims;
	const int ANIM_BULLET_MOVING = 0;
	const int ANIM_BULLET_HIT = 1;
	std::vector<Animation> bulletAnims;

	std::vector<SDL_Texture*> textures;
	SDL_Texture* texIdle, * texRun, * texJump, * texSlide, * texBrick, * texGrass, * texGround, * texPanel,
		* texBg1, * texBg2, * texBg3, * texBg4, * texBg5, * texBg6, * texBg7, * texBg8, * texBg9, * texBullet, * texBulletHit;
	SDL_Texture* loadTexture(SDL_Renderer* renderer, const std::string& filepath)
	{
		SDL_Texture* tex = IMG_LoadTexture(renderer, filepath.c_str());
		SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST); //changes from linear scaling to nearest neighbour scaling on sprite for better quality
		textures.push_back(tex);
		return tex;
	}

	void load(SDLState& state)
	{
		playerAnims.resize(5);
		playerAnims[ANIM_PLAYER_IDLE] = Animation(2, 1.6f, 0, 1);
		playerAnims[ANIM_PLAYER_RUN] = Animation(8, 1.6f, 0, 3);
		playerAnims[ANIM_PLAYER_JUMP] = Animation(8, 1.6f, 0, 5);
		playerAnims[ANIM_PLAYER_SLIDE] = Animation(2, 1.0f, 0, 1);
		bulletAnims.resize(2);
		bulletAnims[ANIM_BULLET_MOVING] = Animation(4, 0.05f, 0, 0);
		bulletAnims[ANIM_BULLET_HIT] = Animation(4, 0.15f, 0, 0);
		texIdle = loadTexture(state.renderer, "data/spriteHood.png");
		texRun = texIdle; //anim in the same file
		texJump = texIdle;
		texSlide = texIdle;
		texBrick = loadTexture(state.renderer, "data/PixelTexturePack/Textures/Bricks/REDBRICKS.png");
		texGrass = loadTexture(state.renderer, "data/PixelTexturePack/grass.png");
		texGround = loadTexture(state.renderer, "data/PixelTexturePack/Textures/Rocks/FLATSTONES.png");
		texPanel = loadTexture(state.renderer, "data/PixelTexturePack/Textures/Tech/BIGSQUARES.png");
		texBullet = loadTexture(state.renderer, "data/bulletDemo.png");
		texBulletHit = loadTexture(state.renderer, "data/bullet_hitDemo.png");
		texBg1 = loadTexture(state.renderer, "data/nature_4/1.png");
		texBg2 = loadTexture(state.renderer, "data/nature_4/2.png");
		texBg3 = loadTexture(state.renderer, "data/nature_4/3.png");
		texBg4 = loadTexture(state.renderer, "data/nature_4/4.png");


	}

	void unload()
	{
		for (SDL_Texture* tex : textures)
		{
			SDL_DestroyTexture(tex);

		}
	}
};

bool initialize(SDLState& state);
void cleanup(SDLState& state);
void drawObject(const SDLState& state, GameState& gs, GameObject& obj, float width, float height, float deltaTime);
void update(const SDLState& state, GameState& gs, Resources& res, GameObject& obj, float deltaTime);
void createTiles(const SDLState& state, GameState& gs, const Resources& res);
void checkCollision(const SDLState& state, GameState& gs, Resources& res, GameObject& a, GameObject& b, float deltaTime);
void handleKeyInput(const SDLState& state, GameState& gs, GameObject& obj, SDL_Scancode key, bool keyDown);
void drawParalaxBackground(SDL_Renderer* renderer, SDL_Texture* texture, float xVelocity, float& scrollPos, float scrollFactor, float deltaTime);

int main(int argc, char* argv[])
{
	SDLState state;
	state.width = 1600;
	state.height = 900;
	state.logicalWidth = 640;
	state.logicalHeight = 320;

	if (!initialize(state))
	{
		return 1;
	}


	// load game assets
	//SDL texture: piece of memory holding picture on the graphics card
	Resources res;
	res.load(state);

	// setup game data
	GameState gs(state);
	createTiles(state, gs, res);


	uint64_t prevTime = SDL_GetTicks();



	//start game loop
	bool running = true;

	while (running)
	{
		uint64_t nowTime = SDL_GetTicks();
		float deltaTime = (nowTime - prevTime) / 1000.0f; //convert to seconds 
		SDL_Event event{ 0 };
		while (SDL_PollEvent(&event)) //keeps calling SDL Poll event which returns true if event occurs
		{
			switch (event.type)
			{
			case SDL_EVENT_QUIT:
			{
				running = false;
				break;
			}
			case  SDL_EVENT_WINDOW_RESIZED:
			{
				state.width = event.window.data1;
				state.height = event.window.data2;
				break;
			}
			case SDL_EVENT_KEY_DOWN:
			{
				handleKeyInput(state, gs, gs.player(), event.key.scancode, true);
				break;
			}
			case SDL_EVENT_KEY_UP:
			{
				handleKeyInput(state, gs, gs.player(), event.key.scancode, false);
				break;
			}

			}
		}

		/*
		NOTE:
		Instead of moving the sprite an arbitrary amount per frame, we should be moving the sprite certain amount per second.
		This is because if we use frame-based movement, the speed of the sprite will be dictated based on the speed of the machine running the game.
		Implications arise since everyone uses different machines now. Frame-based movement used to be acceptable back in the day where developers
		were building games exclusive to certain home consoles where everyone had the same specs.

		game loop must track time so that we move sprite according to passed time rather than speed of the computer.

		We can use SDL_GetTicks() to take time of when previous frame started subtracted from the time of when current frame started
		to get length of time between the frame in ms. If we convert this to seconds we get the amount of time it takes for one frame to execute.
		*/

		// update all objects
		for (auto& layer : gs.layers)
		{
			for (GameObject& obj : layer)
			{
				update(state, gs, res, obj, deltaTime);
				// update Animation
				if (obj.currentAnimation != -1)
				{
					obj.animations[obj.currentAnimation].step(deltaTime);
				}
			}
		}

		// update bullets
		for (GameObject& bullet : gs.bullets)
		{
			update(state, gs, res, bullet, deltaTime);

			// update the animation
			if (bullet.currentAnimation != -1)
			{
				bullet.animations[bullet.currentAnimation].step(deltaTime);
			}
		}
		// calculate viewport position
		gs.mapViewport.x = (gs.player().position.x + TILE_SIZE / 2) - gs.mapViewport.w / 2;

		// perform drawing commands
		SDL_SetRenderDrawColor(state.renderer, 20, 10, 30, 255);
		SDL_RenderClear(state.renderer);

		// draw background images
		SDL_RenderTexture(state.renderer, res.texBg1, nullptr, nullptr);
		drawParalaxBackground(state.renderer, res.texBg2, gs.player().velocity.x, gs.bg2Scroll, 0.075f, deltaTime);
		drawParalaxBackground(state.renderer, res.texBg3, gs.player().velocity.x, gs.bg2Scroll, 0.150f, deltaTime);
		drawParalaxBackground(state.renderer, res.texBg4, gs.player().velocity.x, gs.bg2Scroll, 0.3f, deltaTime);


		// draw background tiles
		for (GameObject& obj : gs.backgroundTiles)
		{
			SDL_FRect dst{
				.x = obj.position.x - gs.mapViewport.x,
				.y = obj.position.y,
				.w = static_cast<float>(obj.texture->w),
				.h = static_cast<float>(obj.texture->h),
			};
			SDL_RenderTexture(state.renderer, obj.texture, nullptr, &dst);
		}
		//draw all objects
		for (auto& layer : gs.layers)
		{
			for (GameObject& obj : layer)
			{
				drawObject(state, gs, obj, TILE_SIZE, TILE_SIZE, deltaTime);
			}
		}
		// draw bullets
		for (GameObject& bullet : gs.bullets)
		{
			drawObject(state, gs, bullet, bullet.collider.w, bullet.collider.h, deltaTime);
		}

		// draw foreground tiles
		for (GameObject& obj : gs.foregroundTiles)
		{
			SDL_FRect dst{
				.x = obj.position.x - gs.mapViewport.x,
				.y = obj.position.y,
				.w = static_cast<float>(obj.texture->w),
				.h = static_cast<float>(obj.texture->h),
			};
			SDL_RenderTexture(state.renderer, obj.texture, nullptr, &dst);
		}

		// display some debug info
		SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, 255);
		SDL_RenderDebugText(state.renderer, 5, 5,
			std::format("S: {}, B: {}, G: {}", static_cast<int>(gs.player().data.player.state), gs.bullets.size(), gs.player().grounded).c_str());

		//swap buffers and present
		SDL_RenderPresent(state.renderer);
		prevTime = nowTime;

	}


	res.unload();
	cleanup(state);
	return 0;
}

bool initialize(SDLState& state)
{
	bool initSuccess = true;
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error initializing SDL3", nullptr);
		initSuccess = false;
	}

	// creating the window

	state.window = SDL_CreateWindow("SDL3 Game", state.width, state.height, SDL_WINDOW_RESIZABLE);

	//check if window is created
	if (!state.window)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error creating window", nullptr);

		cleanup(state);
		initSuccess = false;
	}

	//create the renderer
	state.renderer = SDL_CreateRenderer(state.window, nullptr);
	if (!state.renderer)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error, creating renderer", state.window);
		cleanup(state);
		initSuccess = false;
	}

	SDL_SetRenderVSync(state.renderer, 1);

	//configure presentation

	SDL_SetRenderLogicalPresentation(state.renderer, state.logicalWidth, state.logicalHeight, SDL_LOGICAL_PRESENTATION_LETTERBOX);
	return initSuccess;
}
void cleanup(SDLState& state)
{
	SDL_DestroyRenderer(state.renderer);
	SDL_DestroyWindow(state.window);
	SDL_Quit();
}

void drawObject(const SDLState& state, GameState& gs, GameObject& obj, float width, float height, float deltaTime)
{

	// first frame = 0 * 32, second frame = 1 * 32, etc... as the animation moves forward, srcX value points to new starting x pos in the sprite spreadsheet. 
	float srcX = obj.currentAnimation != -1
		? obj.animations[obj.currentAnimation].currentFrame() * width : 0.0f;

	float srcY = 0.0f;

	if (obj.currentAnimation != -1)
	{
		srcY = obj.animations[obj.currentAnimation].getRowIndex() * width;
	}

	SDL_FRect src{
		.x = srcX,
		.y = srcY,
		.w = width,
		.h = height
	};

	//destination of the sprite
	SDL_FRect dst{
		.x = obj.position.x - gs.mapViewport.x, //setting horizontal position of player in destination rect of drawing code
		.y = obj.position.y,
		.w = width,
		.h = height
	};

	SDL_FlipMode flipMode = obj.direction == -1 ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
	SDL_RenderTextureRotated(state.renderer, obj.texture, &src, &dst, 0, nullptr, flipMode);

}

void update(const SDLState& state, GameState& gs, Resources& res, GameObject& obj, float deltaTime)
{
	if (obj.dynamic)
	{
		// applying gravity
		obj.velocity += glm::vec2(0, 500) * deltaTime;
	}


	float currentDirection = 0;
	if (obj.type == ObjectType::player)
	{

		if (state.keys[SDL_SCANCODE_A])
		{
			currentDirection += -1;
		}
		if (state.keys[SDL_SCANCODE_D])
		{
			currentDirection += 1;
		}
		if (currentDirection)
		{
			obj.direction = currentDirection;
		}
		Timer& weaponTimer = obj.data.player.weaponTimer;
		weaponTimer.step(deltaTime);

		switch (obj.data.player.state)
		{
		case PlayerState::idle:
		{
			// switch to running state
			if (currentDirection)
			{
				obj.data.player.state = PlayerState::running;
			}
			else
			{
				// decelerate
				if (obj.velocity.x)
				{
					const float factor = obj.velocity.x > 0 ? -1.5f : 1.5f;
					float amount = factor * obj.acceleration.x * deltaTime;
					if (std::abs(obj.velocity.x) < std::abs(amount))
					{
						obj.velocity.x = 0;
					}
					else
					{
						obj.velocity.x += amount;
					}
				}
			}
			if (state.keys[SDL_SCANCODE_J])
			{
				if (weaponTimer.isTimeout())
				{
					weaponTimer.reset();
					// spawn some bullets
					GameObject bullet;
					bullet.type = ObjectType::bullet;
					bullet.direction = gs.player().direction;
					bullet.texture = res.texBullet;
					bullet.currentAnimation = res.ANIM_BULLET_MOVING;
					bullet.collider = SDL_FRect{
						.x = 0,
						.y = 0,
						.w = static_cast<float>(res.texBullet->h),
						.h = static_cast<float>(res.texBullet->h),
					};
					bullet.velocity = glm::vec2(
						obj.velocity.x + 600.0f * obj.direction,
						0
					);
					bullet.animations = res.bulletAnims;

					// adjust bullet start position
					const float left = 4;
					const float right = 24;
					const float t = (obj.direction + 1) / 2.0f; // results in a value of 0 .. 1
					const float xOffset = left + right * t; // LERP between left and right based on direction
					bullet.position = glm::vec2(
						obj.position.x + xOffset,
						obj.position.y + TILE_SIZE / 2
					);
					gs.bullets.push_back(bullet);
				}
			}
			obj.texture = res.texIdle;
			obj.currentAnimation = res.ANIM_PLAYER_IDLE;
			break;
		}
		case PlayerState::running:
		{
			// switching to idle state
			if (!currentDirection)
			{
				obj.data.player.state = PlayerState::idle;
				obj.texture = res.texIdle;
				obj.currentAnimation = res.ANIM_PLAYER_IDLE;
			}
			// moving in opposite direction of velocity
			if (obj.velocity.x * obj.direction < 0 && obj.grounded)
			{
				obj.texture = res.texSlide;
				obj.currentAnimation = res.ANIM_PLAYER_SLIDE;
			}
			else
			{
				obj.texture = res.texRun;
				obj.currentAnimation = res.ANIM_PLAYER_RUN;
			}

			break;
		}
		case PlayerState::jumping:
		{
			obj.texture = res.texJump;
			obj.currentAnimation = res.ANIM_PLAYER_JUMP;
			break;
		}
		}
	}

	// add acceleration to velocity
	obj.velocity += currentDirection * obj.acceleration * deltaTime;
	if (std::abs(obj.velocity.x) > obj.maxSpeedX)
	{
		obj.velocity.x = currentDirection * obj.maxSpeedX;
	}

	// add velocity to position
	obj.position += obj.velocity * deltaTime;

	// handle collision detection
	bool foundGround = false;
	for (auto& layer : gs.layers)
	{
		for (GameObject& objB : layer)
		{
			if (&obj != &objB)
			{
				checkCollision(state, gs, res, obj, objB, deltaTime);
				// grounded sensor
				SDL_FRect sensor{
					.x = obj.position.x + obj.collider.x,
					.y = obj.position.y + obj.collider.y + obj.collider.h,
					.w = obj.collider.w,
					.h = 1
				};
				SDL_FRect rectB{
					.x = objB.position.x + objB.collider.x,
					.y = objB.position.y + objB.collider.y,
					.w = objB.collider.w,
					.h = objB.collider.h
				};
				if (SDL_HasRectIntersectionFloat(&sensor, &rectB))
				{
					foundGround = true;
				}
			}
		}
	}
	if (obj.grounded != foundGround)
	{
		// swithing grounded state
		obj.grounded = foundGround;
		if (obj.grounded == foundGround && obj.type == ObjectType::player)
		{
			obj.data.player.state = PlayerState::running;
		}
	}
}

void collisionResponse(const SDLState& state, GameState& gs, Resources& res, const SDL_FRect& rectA, const SDL_FRect& rectB,
	const SDL_FRect& rectC, GameObject& objA, GameObject& objB, float deltaTime)
{
	// object to check
	if (objA.type == ObjectType::player)
	{
		// object it is colliding with
		switch (objB.type)
		{
		case ObjectType::level:
		{
			// if player collides with level object the height of rectC would be much greater than the width
			// we resolve the collision by increasing the height of player by the height of rectC and vice versa for horizontally colliding 
			if (rectC.w < rectC.h)
			{
				// horizontal collision

				if (objA.velocity.x > 0)
				{
					objA.position.x -= rectC.w; // going right
				}
				else if (objA.velocity.x < 0)
				{
					objA.position.x += rectC.w; // going left
				}
				objA.velocity.x = 0;
			}
			else
			{
				// vertical collision

				if (objA.velocity.y > 0)
				{
					objA.position.y -= rectC.h; // going down
				}
				else if (objA.velocity.y < 0)
				{
					objA.position.y += rectC.h; // going up
				}
				objA.velocity.y = 0;
			}
			break;
		}

		}
	}
}


//rect A, B, C are representing hitboxes to see if two gameObjects are intersecting
//rect c will show how far two objects are overlapping
void checkCollision(const SDLState& state, GameState& gs, Resources& res, GameObject& a, GameObject& b, float deltaTime)
{
	SDL_FRect rectA{
		.x = a.position.x + a.collider.x,
		.y = a.position.y + a.collider.y,
		.w = a.collider.w,
		.h = a.collider.h
	};
	SDL_FRect rectB{
		.x = b.position.x + b.collider.x,
		.y = b.position.y + b.collider.y,
		.w = b.collider.w,
		.h = b.collider.h
	};
	SDL_FRect rectC{ 0 };

	if (SDL_GetRectIntersectionFloat(&rectA, &rectB, &rectC))
	{
		// found the intersection
		collisionResponse(state, gs, res, rectA, rectB, rectC, a, b, deltaTime);
	}
}
void createTiles(const SDLState& state, GameState& gs, const Resources& res)
{
	/*
	1 - ground
	2 - panel
	3 - enemy
	4 - player
	5 - grass
	6 - brick
	*/
	short map[MAP_ROWS][MAP_COLS] = {
		0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 0, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	};

	short foreground[MAP_ROWS][MAP_COLS] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		5, 0, 0, 5, 5, 5, 5, 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	};

	short background[MAP_ROWS][MAP_COLS] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	};

	const auto loadMap = [&state, &gs, &res](short layer[MAP_ROWS][MAP_COLS])
		{
			const auto createObject = [&state](int r, int c, SDL_Texture* tex, ObjectType type)
				{
					GameObject o;
					o.type = type;
					o.position = glm::vec2(c * TILE_SIZE, state.logicalHeight - (MAP_ROWS - r) * TILE_SIZE);
					o.texture = tex;
					o.collider = {
						.x = 0,
						.y = 0,
						.w = TILE_SIZE,
						.h = TILE_SIZE
					};
					return o;
				};

			for (int r = 0; r < MAP_ROWS; r++)
			{
				for (int c = 0; c < MAP_COLS; c++)
				{
					switch (layer[r][c])
					{
					case 1:
					{
						GameObject o = createObject(r, c, res.texGround, ObjectType::level);
						gs.layers[LAYER_IDX_LEVEL].push_back(o);
						break;
					}

					case 2:
					{
						GameObject o = createObject(r, c, res.texPanel, ObjectType::level);
						gs.layers[LAYER_IDX_LEVEL].push_back(o);
						break;
					}

					case 4: // player
					{

						GameObject player = createObject(r, c, res.texIdle, ObjectType::player);
						player.data.player = PlayerData();
						player.animations = res.playerAnims;
						player.currentAnimation = res.ANIM_PLAYER_IDLE;
						player.acceleration = glm::vec2(300, 0);
						player.maxSpeedX = 100;
						player.dynamic = true;
						player.collider = {
							.x = 11,
							.y = 6,
							.w = 10,
							.h = 26
						};

						gs.layers[LAYER_IDX_CHARACTERS].push_back(player);
						gs.playerIndex = gs.layers[LAYER_IDX_CHARACTERS].size() - 1;

						break;
					}
					case 5: // grass
					{
						GameObject o = createObject(r, c, res.texGrass, ObjectType::level);
						gs.foregroundTiles.push_back(o);
						break;
					}
					case 6: // brick
					{
						GameObject o = createObject(r, c, res.texBrick, ObjectType::level);
						gs.backgroundTiles.push_back(o);
						break;
					}
					}
				}
			}
		};
	loadMap(map);
	loadMap(background);
	loadMap(foreground);
	assert(gs.playerIndex != -1);
}

void handleKeyInput(const SDLState& state, GameState& gs, GameObject& obj,
	SDL_Scancode key, bool keyDown)
{

	const float JUMP_FORCE = -200.0f;
	if (obj.type == ObjectType::player)
	{
		switch (obj.data.player.state)
		{
		case PlayerState::idle:
		{
			if (key == SDL_SCANCODE_K && keyDown)
			{
				obj.data.player.state = PlayerState::jumping;
				obj.velocity.y += JUMP_FORCE;

			}
			break;
		}
		case PlayerState::running:
		{
			if (key == SDL_SCANCODE_K && keyDown)
			{
				obj.data.player.state = PlayerState::jumping;
				obj.velocity.y += JUMP_FORCE;
			}
			break;
		}
		}
	}
}


// as our player walks towards the right, the background moves towards the left relative to the movement speed of the character
void drawParalaxBackground(SDL_Renderer* renderer, SDL_Texture* texture,
	float xVelocity, float& scrollPos, float scrollFactor, float deltaTime)
{
	scrollPos -= xVelocity * scrollFactor * deltaTime;
	if (scrollPos <= -texture->w)
	{
		scrollPos = 0;
	}

	SDL_FRect dst{
		.x = scrollPos,
		.y = 10,
		.w = texture->w * 2.0f,
		.h = static_cast<float>(texture->h)

	};
	SDL_RenderTextureTiled(renderer, texture, nullptr, 1, &dst);
}