#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <sdl3/SDL.h>
#include "animation.h"

/*
NOTE: Code can become very complicated and unmanageable the more features you begin to add like jumping, shooting, crouching.
We want to avoid using using many nested if-conditions that verifies many different combinations of gameplay states to figure out

*/
enum class PlayerState
{
	idle, running, jumping,
};

enum class BulletState
{
	moving, colliding, inactive
};

struct PlayerData
{

	PlayerState state;
	Timer weaponTimer;

	PlayerData() : weaponTimer(0.1f)
	{
		state = PlayerState::idle;
	}
};

struct LevelData {};
struct EnemyData {};
struct BulletData
{
	BulletState state;
	BulletData() : state(BulletState::moving) {

	}
};

union ObjectData
{
	PlayerData player;
	LevelData level;
	EnemyData enemy;
	BulletData bullet;
};

enum class ObjectType
{
	player,
	level,
	enemy,
	bullet // Added bullet type to fix the error
};



struct GameObject
{
	ObjectType type;
	ObjectData data;
	glm::vec2 position, velocity, acceleration;
	float direction;
	float maxSpeedX;
	std::vector<Animation> animations;
	int currentAnimation;
	SDL_Texture* texture;
	bool dynamic;
	bool grounded;
	SDL_FRect collider;


	GameObject() : data{ .level = LevelData() }, collider{ 0 }
	{
		type = ObjectType::level;
		direction = 1;
		maxSpeedX = 0;
		position = velocity = acceleration = glm::vec2(0);
		currentAnimation = -1;
		texture = nullptr;
		dynamic = false;
		grounded = false;
	}
};