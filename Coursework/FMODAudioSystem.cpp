#include "FMODAudioSystem.h"

template<typename T>
T clamp(const T& value, const T& min, const T& max) {
	return (value < min) ? min : (value > max) ? max : value;
}

FMODAudioSystem::FMODAudioSystem() : studioSystem(nullptr) {
	bgm.instance = nullptr;
	bgm.channelGroup = nullptr;
	bgm.lowpass = nullptr;
	events.bgm1 = nullptr;
	events.heavyWhisper = nullptr;
	events.girlWhisper = nullptr;
}

bool FMODAudioSystem::init() {
	// First check if already initialized
	if (studioSystem) {
		return true;
	}

	FMOD_RESULT result;

	// 1. Create studio system
	result = FMOD::Studio::System::create(&studioSystem);
	if (result != FMOD_OK || !studioSystem) {
		OutputDebugStringA("FMOD: Failed to create studio system - ");
		OutputDebugStringA(FMOD_ErrorString(result));
		OutputDebugStringA("\n");
		return false;
	}

	// 2. Initialize system
	result = studioSystem->initialize(
		64,
		FMOD_STUDIO_INIT_NORMAL,
		FMOD_INIT_NORMAL,
		nullptr
	);
	if (result != FMOD_OK) {
		OutputDebugStringA("FMOD: Failed to initialize system - ");
		OutputDebugStringA(FMOD_ErrorString(result));
		OutputDebugStringA("\n");
		release();
		return false;
	}

	// 3. Load banks
	const std::vector<std::string> bankPaths = {
		"../FMOD Project/CMP505_Audio/Build/Desktop/CMP505.bank",
		"../FMOD Project/CMP505_Audio/Build/Desktop/CMP505.strings.bank"
	};

	bool allBanksLoaded = true;
	for (const auto& path : bankPaths) {
		FMOD::Studio::Bank* bank = nullptr;
		result = studioSystem->loadBankFile(
			path.c_str(),
			FMOD_STUDIO_LOAD_BANK_NORMAL,
			&bank
		);

		if (result != FMOD_OK) {
			OutputDebugStringA(("FMOD: Failed to load bank " + path + " - ").c_str());
			OutputDebugStringA(FMOD_ErrorString(result));
			OutputDebugStringA("\n");
			allBanksLoaded = false;
		}
	}

	if (!allBanksLoaded) {
		OutputDebugStringA("FMOD: Some banks failed to load\n");
	}

	// Wait for banks to load
	studioSystem->flushCommands();
	return true;
}

void FMODAudioSystem::playBGM1() {
	if (!studioSystem || events.bgm1) return; // Already playing

	FMOD::Studio::EventDescription* desc = nullptr;
	FMOD_RESULT result = studioSystem->getEvent("event:/BGM1", &desc);
	if (result != FMOD_OK || !desc) return;

	result = desc->createInstance(&events.bgm1);
	if (result == FMOD_OK && events.bgm1) {
		events.bgm1->start();
		bgm.started = true;
		bgm.targetVolume = 1.0f; // Full volume
	}
}

void FMODAudioSystem::stopBGM1() {
	if (events.bgm1) {
		events.bgm1->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);
		events.bgm1->release();
		events.bgm1 = nullptr;
		bgm.started = false;
	}
}

void FMODAudioSystem::update(float deltaTime) {
	if (!studioSystem) return;

	// BGM volume control
	if (bgm.started && events.bgm1) {
		float currentVolume;
		events.bgm1->getVolume(&currentVolume);

		// Smooth volume transition
		const float fadeSpeed = 0.2f; // Speed of volume change per second
		const float volumeDelta = fadeSpeed * deltaTime;

		if (currentVolume < bgm.targetVolume) {
			currentVolume = min(currentVolume + volumeDelta, bgm.targetVolume);
		}
		else if (currentVolume > bgm.targetVolume) {
			currentVolume = max(currentVolume - volumeDelta, bgm.targetVolume);
		}

		events.bgm1->setVolume(currentVolume);
	}

	// BGM restore after dimming
	if (bgm.dimmed) {
		bgm.restoreTimer -= deltaTime;
		if (bgm.restoreTimer <= 0.0f && bgm.targetVolume < 1.0f) {
			bgm.targetVolume = 1.0f;

			// Remove lowpass effect
			if (bgm.lowpass) {
				if (bgm.channelGroup) {
					bgm.channelGroup->removeDSP(bgm.lowpass);
				}
				bgm.lowpass->release();
				bgm.lowpass = nullptr;
			}
			bgm.dimmed = false;
		}
	}

	studioSystem->update();
}

// Play one-shot sound
void FMODAudioSystem::playOneShot(const string& eventPath) {
	if (!studioSystem) return;

	FMOD::Studio::EventDescription* desc = nullptr;
	FMOD_RESULT result = studioSystem->getEvent(eventPath.c_str(), &desc);
	if (result != FMOD_OK || !desc) return;

	FMOD::Studio::EventInstance* instance = nullptr;
	result = desc->createInstance(&instance);
	if (result == FMOD_OK && instance) {
		instance->start();
		instance->release(); // Release immediately for one-shot sounds
	}
}

void FMODAudioSystem::dimBGM(float duration, float targetVolume) {
	if (!bgm.started || !events.bgm1) return;

	// Only modify BGM volume, don't affect whispers
	bgm.targetVolume = targetVolume;
	bgm.restoreTimer = duration;
	bgm.dimmed = true;

	// Optional: Add a subtle high-pass filter instead of low-pass
	if (!bgm.lowpass) {
		FMOD::System* coreSystem = nullptr;
		studioSystem->getCoreSystem(&coreSystem);
		if (!coreSystem) return;

		coreSystem->createDSPByType(FMOD_DSP_TYPE_HIGHPASS, &bgm.lowpass);
		if (bgm.lowpass) {
			bgm.lowpass->setParameterFloat(FMOD_DSP_HIGHPASS_CUTOFF, 500.0f);

			events.bgm1->getChannelGroup(&bgm.channelGroup);
			if (bgm.channelGroup) {
				bgm.channelGroup->addDSP(0, bgm.lowpass);
			}
		}
	}
}

// Release resources
void FMODAudioSystem::release() {
	// Release DSP effects
	if (bgm.lowpass) {
		if (bgm.channelGroup) {
			bgm.channelGroup->removeDSP(bgm.lowpass);
		}
		bgm.lowpass->release();
		bgm.lowpass = nullptr;
	}

	// Release event instances
	auto releaseInstance = [](FMOD::Studio::EventInstance*& instance) {
		if (instance) {
			instance->stop(FMOD_STUDIO_STOP_IMMEDIATE);
			instance->release();
			instance = nullptr;
		}
		};

	releaseInstance(events.bgm1);
	releaseInstance(events.heavyWhisper);
	releaseInstance(events.girlWhisper);

	// Release system
	if (studioSystem) {
		studioSystem->release();
		studioSystem = nullptr;
	}
}

void FMODAudioSystem::playFireflyWhisper(const XMFLOAT3& position) {
	if (!studioSystem || events.girlWhisper) return;

	FMOD::Studio::EventDescription* desc = nullptr;
	FMOD_RESULT result = studioSystem->getEvent("event:/GirlWhisper", &desc);
	if (result != FMOD_OK || !desc) return;

	result = desc->createInstance(&events.girlWhisper);
	if (result == FMOD_OK && events.girlWhisper) {
		// Set 3D attributes
		FMOD_3D_ATTRIBUTES attributes = { { 0 } };
		attributes.position = { position.x, position.y, position.z };
		attributes.forward = { 0, 0, 1 };
		attributes.up = { 0, 1, 0 };
		events.girlWhisper->set3DAttributes(&attributes);

		// Start with minimum volume - it will be updated in updateFireflyWhisperVolume
		events.girlWhisper->setVolume(0.3f);  // Start slightly louder
		events.girlWhisper->start();

		// Initialize ghost effects
		ghostEffectIntensity = 0.0f;
		ghostEffectTimer = 0.0f;
	}
}

void FMODAudioSystem::stopFireflyWhisper() {
	if (events.girlWhisper) {
		events.girlWhisper->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);
		events.girlWhisper->release();
		events.girlWhisper = nullptr;
	}
}

void FMODAudioSystem::updateFireflyPosition(const XMFLOAT3& position) {
	if (events.girlWhisper) {
		FMOD_3D_ATTRIBUTES attributes = { { 0 } };
		attributes.position = { position.x, position.y, position.z };
		attributes.forward = { 0, 0, 1 };  // Default forward
		attributes.up = { 0, 1, 0 };       // Default up

		// Calculate velocity for Doppler effect
		static XMFLOAT3 lastPosition = position;
		float deltaTime = 0.016f; // Approximate frame time

		attributes.velocity.x = (position.x - lastPosition.x) / deltaTime;
		attributes.velocity.y = (position.y - lastPosition.y) / deltaTime;
		attributes.velocity.z = (position.z - lastPosition.z) / deltaTime;

		lastPosition = position;

		events.girlWhisper->set3DAttributes(&attributes);
	}
}

void FMODAudioSystem::updateListenerPosition(const XMFLOAT3& position, const XMFLOAT3& forward, const XMFLOAT3& up) {
	if (!studioSystem) return;

	FMOD_3D_ATTRIBUTES attributes = { { 0 } };
	attributes.position = { position.x, position.y, position.z };
	attributes.forward = { forward.x, forward.y, forward.z };
	attributes.up = { up.x, up.y, up.z };

	studioSystem->setListenerAttributes(0, &attributes);
}

void FMODAudioSystem::updateFireflyWhisperVolume(const XMFLOAT3& listenerPosition) {
	if (!events.girlWhisper) return;

	// Get current firefly position and velocity
	FMOD_3D_ATTRIBUTES fireflyAttributes;
	events.girlWhisper->get3DAttributes(&fireflyAttributes);

	// Calculate distance and direction vector between listener and firefly
	XMVECTOR listenerPos = XMVectorSet(listenerPosition.x, listenerPosition.y, listenerPosition.z, 0.0f);
	XMVECTOR fireflyPos = XMVectorSet(fireflyAttributes.position.x, fireflyAttributes.position.y, fireflyAttributes.position.z, 0.0f);
	XMVECTOR toListener = XMVectorSubtract(listenerPos, fireflyPos);
	float distance = XMVectorGetX(XMVector3Length(toListener));
	XMVECTOR direction = XMVector3Normalize(toListener);

	// Calculate relative velocity for Doppler effect
	XMVECTOR fireflyVel = XMVectorSet(fireflyAttributes.velocity.x, fireflyAttributes.velocity.y, fireflyAttributes.velocity.z, 0.0f);
	float relativeVelocity = XMVectorGetX(XMVector3Dot(fireflyVel, direction));

	// Smooth volume attenuation based on distance
	float volume = WHISPER_MIN_VOLUME;
	if (distance < WHISPER_CLOSE_RANGE) {
		// Very close - full volume with slight boost when moving towards player
		float closeness = 1.0f - (distance / WHISPER_CLOSE_RANGE);
		volume = WHISPER_CLOSE_VOLUME + closeness * 0.2f; // Slight boost when very close
	}
	else if (distance < WHISPER_MID_RANGE) {
		// Close-mid range - smooth transition
		float t = (distance - WHISPER_CLOSE_RANGE) / (WHISPER_MID_RANGE - WHISPER_CLOSE_RANGE);
		volume = WHISPER_CLOSE_VOLUME + t * (WHISPER_MID_VOLUME - WHISPER_CLOSE_VOLUME);
	}
	else if (distance < WHISPER_FAR_RANGE) {
		// Mid-far range
		float t = (distance - WHISPER_MID_RANGE) / (WHISPER_FAR_RANGE - WHISPER_MID_RANGE);
		volume = WHISPER_MID_VOLUME + t * (WHISPER_FAR_VOLUME - WHISPER_MID_VOLUME);
	}
	else if (distance < WHISPER_FADE_RANGE) {
		// Far-fade range
		float t = (distance - WHISPER_FAR_RANGE) / (WHISPER_FADE_RANGE - WHISPER_FAR_RANGE);
		volume = WHISPER_FAR_VOLUME + t * (WHISPER_MIN_VOLUME - WHISPER_FAR_VOLUME);
	}

	// Apply intensity modulation
	volume *= (1.0f + ghostEffectIntensity * 0.5f);
	volume = clamp(volume, WHISPER_MIN_VOLUME, WHISPER_CLOSE_VOLUME * 1.2f);

	// Apply volume
	events.girlWhisper->setVolume(volume);

	// Update 3D effects through channel group
	FMOD::ChannelGroup* channelGroup = nullptr;
	events.girlWhisper->getChannelGroup(&channelGroup);
	if (channelGroup) {
		// Calculate normalized direction for cone orientation
		XMFLOAT3 dir;
		XMStoreFloat3(&dir, direction);
		FMOD_VECTOR forward = { dir.x, dir.y, dir.z };
		channelGroup->set3DConeOrientation(&forward);

		// Dynamic cone settings - tighter when closer
		float coneInsideAngle, coneOutsideAngle;
		if (distance < WHISPER_CLOSE_RANGE) {
			coneInsideAngle = 45.0f;
			coneOutsideAngle = 90.0f;
		}
		else if (distance < WHISPER_MID_RANGE) {
			coneInsideAngle = 60.0f;
			coneOutsideAngle = 120.0f;
		}
		else {
			coneInsideAngle = 90.0f;
			coneOutsideAngle = 180.0f;
		}
		channelGroup->set3DConeSettings(coneInsideAngle, coneOutsideAngle, 0.3f);

		// Doppler effect - more pronounced when firefly is moving towards/away from player
		float dopplerLevel = 0.0f;
		if (fabs(relativeVelocity) > 0.1f) {
			// Scale Doppler effect based on relative velocity (0.5 to 2.0)
			dopplerLevel = clamp(1.5f + relativeVelocity * 0.5f, 0.5f, 2.0f);
		}
		channelGroup->set3DDopplerLevel(dopplerLevel);

		// Pitch variation based on relative movement
		float pitch = 1.0f;
		if (relativeVelocity > 0.5f) {
			// Firefly moving towards player - higher pitch
			pitch = clamp(1.0f + relativeVelocity * 0.1f, 1.0f, 1.2f);
		}
		else if (relativeVelocity < -0.5f) {
			// Firefly moving away - lower pitch
			pitch = clamp(1.0f + relativeVelocity * 0.05f, 0.9f, 1.0f);
		}
		channelGroup->setPitch(pitch);
	}

	// Debug output - D3D11 text rendering

	char debugBuffer[512];
	sprintf_s(debugBuffer,
		"Firefly Audio Debug:\n"
		"Distance: %.2f units\n"
		"Firefly Pos: (%.2f, %.2f, %.2f)\n"
		"Volume: %.2f%%\n"
		"Relative Velocity: %.2f units/sec\n"
		"Distance Range: %s\n\n",
		distance,
		fireflyAttributes.position.x, fireflyAttributes.position.y, fireflyAttributes.position.z,
		volume * 100.0f,
		relativeVelocity,

		(distance < WHISPER_CLOSE_RANGE) ? "VERY CLOSE (<10)" :
		(distance < WHISPER_MID_RANGE) ? "CLOSE (10-20)" :
		(distance < WHISPER_FAR_RANGE) ? "MID (20-40)" :
		(distance < WHISPER_FADE_RANGE) ? "FAR (40-50)" : "VERY FAR (>50)"
	);

	OutputDebugStringA(debugBuffer);
}

void FMODAudioSystem::updateGhostEffects(float deltaTime, const XMFLOAT3& listenerPosition) {
	if (!events.girlWhisper) return;

	ghostEffectTimer += deltaTime;
	if (ghostEffectTimer >= GHOST_EFFECT_INTERVAL) {
		ghostEffectTimer = 0.0f;

		// Get distance to ghost
		FMOD_3D_ATTRIBUTES ghostAttrs;
		events.girlWhisper->get3DAttributes(&ghostAttrs);
		float distance = sqrtf(powf(listenerPosition.x - ghostAttrs.position.x, 2) +
			powf(listenerPosition.y - ghostAttrs.position.y, 2) +
			powf(listenerPosition.z - ghostAttrs.position.z, 2));

		// More intense effects when closer
		float proximityFactor = 1.0f - min(distance / WHISPER_FADE_RANGE, 1.0f);

		// Random effects
		if (rand() % 3 == 0) { // 33% chance per interval
			// Pitch variation
			float pitch = 1.0f + ((rand() % 21) - 10) * 0.02f * proximityFactor; // ±20% variation
			events.girlWhisper->setPitch(pitch);

			// Occasional whisper bursts when close
			if (distance < WHISPER_CLOSE_RANGE * 1.5f) {
				float currentVol;
				events.girlWhisper->getVolume(&currentVol);
				events.girlWhisper->setVolume(currentVol * 1.5f); // 50% louder burst

				// Add temporary reverb for creepiness
				FMOD::System* coreSystem = nullptr;
				studioSystem->getCoreSystem(&coreSystem);
				if (coreSystem) {
					FMOD::DSP* reverb;
					coreSystem->createDSPByType(FMOD_DSP_TYPE_SFXREVERB, &reverb);
					if (reverb) {
						reverb->setParameterFloat(FMOD_DSP_SFXREVERB_DECAYTIME, 3.0f);
						reverb->setParameterFloat(FMOD_DSP_SFXREVERB_DIFFUSION, 80.0f);

						FMOD::ChannelGroup* cg;
						events.girlWhisper->getChannelGroup(&cg);
						if (cg) {
							cg->addDSP(1, reverb);
							// Note: In production code you'd need to track and release this DSP
						}
					}
				}
			}
		}
	}
}

void FMODAudioSystem::setGhostEffectIntensity(float intensity) {
	ghostEffectIntensity = clamp(intensity, 0.0f, 1.0f);
}