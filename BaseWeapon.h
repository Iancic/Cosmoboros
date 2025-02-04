#pragma once
#include <SDL.h>
#include <vector>
#include "Transform.h"
#include "Bullet.h"
#include "Camera.h"

class BaseWeapon
{
public:
	BaseWeapon(Transform* Slot, Camera* camera);
	~BaseWeapon() = default;

	// TO DO: SET THESE AS WEAPON TYPE
	float magSize = 16;
	float reloadTime = 2.5f;
	float reloadCounter = 0.f;
	float lenght = 30.f;
	float bulletSpeed = 180.f;

	std::vector<Bullet*> ammunition{};

	Transform* slotPosition;

	Transform transform{};
	Transform target{};

	Camera* activeCamera;

	glm::vec2 vector{}, normalized{}, magnitude{}, direction{};

	void Update(Transform Target, float deltaTime);
	void Render();

	void Shoot();

private:
};

