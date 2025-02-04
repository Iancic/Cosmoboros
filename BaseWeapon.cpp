#include "BaseWeapon.h"

BaseWeapon::BaseWeapon(Transform* Slot, Camera* camera)
{
	slotPosition = Slot;
	activeCamera = camera;

	// Default Value For Gun Orientation
	target.position.x = 600;
	target.position.y = 600;
}

void BaseWeapon::Update(Transform Target, float deltaTime)
{
	// Weapon Updates
	transform.position = slotPosition->position;

	target.position = Target.position;

	vector.x = target.position.x - transform.position.x;
	vector.y = target.position.y - transform.position.y;

	reloadCounter += deltaTime;
	if (reloadCounter >= reloadTime)
	{
		reloadCounter = 0.f;
		Shoot();
	}

	// Bullet Updates
	for (auto& bullet : ammunition)
	{
		bullet->Update(deltaTime, bulletSpeed);
	}
}

void BaseWeapon::Render()
{
	magnitude.x = target.position.x - transform.position.x;
	magnitude.y = target.position.y - transform.position.y;

	float distance = sqrt(magnitude.x * magnitude.x + magnitude.y * magnitude.y);

	float normalizedX = (target.position.x - transform.position.x) / distance;
	float normalizedY = (target.position.y - transform.position.y) / distance;

	direction = { normalizedX, normalizedY };

	float finalposX = normalizedX * lenght;
	float finalposY = normalizedY * lenght;

	/*SDL_RenderDrawLine(renderer, 
		transform.position.x - activeCamera->transform->position.x,
		transform.position.y - activeCamera->transform->position.y,
		transform.position.x + finalposX - activeCamera->transform->position.x,
		transform.position.y + finalposY - activeCamera->transform->position.y);*/

	// Bullet Rendering
	for (auto& bullet : ammunition)
	{
		bullet->Render();
	}
}

void BaseWeapon::Shoot()
{
	ammunition.push_back(new Bullet(transform, activeCamera, direction));
}
