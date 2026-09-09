#include "DissolveFade.h"
#include "EasingManager.h"
#include "EasingManager.h"
using namespace GameEngine;

DissolveFade::DissolveFade(GameEngine::Dissolve* dissolve) {
	dissolve_ = dissolve;
}

void DissolveFade::Initialize() {
	
}

void DissolveFade::Update(float timer) {

	if (timer <= 0.5f) {
		float localT = timer / 0.5f;

		float s = Lerp(0.0f, 1.0f, localT, EaseType::kEaseInQuad);
		dissolve_->SetThreshold(s);
	} else {
		float localT = (timer - 0.5f) / 0.5f;
		float s = Lerp(1.0f, 0.0f, localT, EaseType::kEaseInQuad);
		dissolve_->SetThreshold(s);
	}
}

void DissolveFade::Draw() {
	
}

bool DissolveFade::IsMidTransition(float timer) const {
	return timer >= 0.5f;
}

