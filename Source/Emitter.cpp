#include "Emitter.hpp"

Emitter::Emitter()
{
	//particlePool.resize(10);
}

void Emitter::Emit(const ParticleProperties& particleprops)
{
	// Set particle witht the next index iin pool to active and initialize it's properties.
	particlePool.push_back(new Particle());

	//particle.transform.position.x = 0.f;
	//particle.transform.position.y = 0.f;

	// Velocity
	particlePool[particlePool.size() - 1]->velocity.x += Random::RandomFloat() * particleprops.velocityVariationX;
	particlePool[particlePool.size() - 1]->velocity.y += Random::RandomFloat() * particleprops.velocityVariationY;

	// Color
	particlePool[particlePool.size() - 1]->colorBegin = particleprops.colorBegin;
	particlePool[particlePool.size() - 1]->colorEnd = particleprops.colorEnd;

	// Others
	particlePool[particlePool.size() - 1]->lifeTime = particleprops.lifeTime;
	particlePool[particlePool.size() - 1]->lifeTimeRemaining = particleprops.lifeTime;
	particlePool[particlePool.size() - 1]->sizeBegin = particleprops.sizeBegin + particleprops.sizeVariation * (Random::RandomFloat() - 0.5f);
	particlePool[particlePool.size() - 1]->sizeEnd = particleprops.sizeEnd; // Size end and begin used for interpolation
}

void Emitter::Update(float deltaTime)
{
	for (size_t i = 0; i < particlePool.size(); ++i )
	{
		Particle* particle = particlePool[i];

		if (particle->lifeTimeRemaining <= 0.0f)
		{
			delete particle;
			particlePool.erase(particlePool.begin() + i);
			continue;
		}

		particle->lifeTimeRemaining -= deltaTime;
		particle->transform.position += glm::vec3{ particle->velocity.x * deltaTime, particle->velocity.y * deltaTime, 0.f };
		particle->rotation -= particle->rotationSpeed * deltaTime;

		float interpolationFactor = 1.0f - (particle->lifeTimeRemaining / particle->lifeTime);

		particle->color = glm::mix(particle->colorBegin, particle->colorEnd, interpolationFactor);
		particle->size = glm::mix(particle->sizeBegin, particle->sizeEnd, interpolationFactor);
	}
}

void Emitter::Render(Camera* activeCamera)
{
	for (auto& particle : particlePool)
		Circle::getInstance()->DrawCircle(particle->transform.position.x, particle->transform.position.y, particle->size, 5, particle->color, particle->rotation);
}