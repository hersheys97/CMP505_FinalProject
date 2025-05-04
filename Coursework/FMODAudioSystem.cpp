#include "FMODAudioSystem.h"

template<typename T>
T clamp(const T& value, const T& min, const T& max) {
	return (value < min) ? min : (value > max) ? max : value;
}

inline float smoothstep(float edge0, float edge1, float x) {
	x = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
	return x * x * (3.0f - 2.0f * x);
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
	const vector<string> bankPaths = {
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
		bgm.targetVolume = 0.6f;
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
			bgm.targetVolume = 0.6f;

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

	// Use logarithmic scaling for perceived loudness
	bgm.targetVolume = log10(1 + 9 * targetVolume);
	bgm.restoreTimer = duration;
	bgm.dimmed = true;

	if (!bgm.lowpass) {
		FMOD::System* coreSystem = nullptr;
		studioSystem->getCoreSystem(&coreSystem);
		if (!coreSystem) return;

		// Use multi-effect processing for natural ducking
		coreSystem->createDSPByType(FMOD_DSP_TYPE_THREE_EQ, &bgm.lowpass);
		if (bgm.lowpass) {
			// Reduce mid/high frequencies instead of harsh cutoff
			bgm.lowpass->setParameterFloat(FMOD_DSP_THREE_EQ_MIDGAIN, -6.0f);
			bgm.lowpass->setParameterFloat(FMOD_DSP_THREE_EQ_HIGHGAIN, -3.0f);

			events.bgm1->getChannelGroup(&bgm.channelGroup);
			if (bgm.channelGroup) {
				bgm.channelGroup->addDSP(0, bgm.lowpass);

				// Add subtle compression to maintain perceived presence
				FMOD::DSP* compressor;
				coreSystem->createDSPByType(FMOD_DSP_TYPE_COMPRESSOR, &compressor);
				compressor->setParameterFloat(FMOD_DSP_COMPRESSOR_THRESHOLD, -15.0f);
				bgm.channelGroup->addDSP(1, compressor);
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

void FMODAudioSystem::playGhostWhisper(const XMFLOAT3& position) {
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

		// Start with minimum volume - it will be updated in updateGhostWhisperVolume
		events.girlWhisper->setVolume(0.3f);  // Start slightly louder
		events.girlWhisper->start();

		// Initialize ghost effects
		ghostEffectIntensity = 0.0f;
		ghostEffectTimer = 0.0f;
	}
}

void FMODAudioSystem::stopGhostWhisper() {
	if (events.girlWhisper) {
		events.girlWhisper->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);
		events.girlWhisper->release();
		events.girlWhisper = nullptr;
	}
}

void FMODAudioSystem::updateGhostPosition(const XMFLOAT3& position) {
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

void FMODAudioSystem::updateGhostWhisperVolume(const XMFLOAT3& listenerPosition) {
	if (!events.girlWhisper) return;

	FMOD_3D_ATTRIBUTES ghostAttributes;
	events.girlWhisper->get3DAttributes(&ghostAttributes);

	// Calculate distance and direction using spherical interpolation
	XMVECTOR listenerPos = XMVectorSet(listenerPosition.x, listenerPosition.y, listenerPosition.z, 0.0f);
	XMVECTOR ghostPos = XMVectorSet(ghostAttributes.position.x, ghostAttributes.position.y, ghostAttributes.position.z, 0.0f);
	XMVECTOR toListener = XMVectorSubtract(listenerPos, ghostPos);
	float distance = XMVectorGetX(XMVector3Length(toListener));

	// Logarithmic distance attenuation (more natural perception)
	float volume = 1.0f - (log10(1 + distance) / log10(1 + WHISPER_FADE_RANGE));
	volume = clamp(volume * (1.0f + ghostEffectIntensity * 0.3f), WHISPER_MIN_VOLUME, 1.2f);

	// Get the channel group for additional 3D control
	FMOD::ChannelGroup* channelGroup;
	events.girlWhisper->getChannelGroup(&channelGroup);

	if (channelGroup) {
		// Proper 3D spatialization using available FMOD API
		FMOD_VECTOR dir;
		XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&dir), XMVector3Normalize(toListener));

		// Set 3D cone orientation (alternative to pan level)
		channelGroup->set3DConeOrientation(&dir);

		// Dynamic cone settings based on distance
		float coneInside, coneOutside;
		if (distance < WHISPER_CLOSE_RANGE) {
			coneInside = 30.0f;
			coneOutside = 90.0f;
		}
		else if (distance < WHISPER_MID_RANGE) {
			coneInside = 60.0f;
			coneOutside = 150.0f;
		}
		else {
			coneInside = 90.0f;
			coneOutside = 180.0f;
		}
		channelGroup->set3DConeSettings(coneInside, coneOutside, 0.3f);

		// Smooth Doppler effect
		static float lastRelativeVelocity = 0.0f;
		XMVECTOR ghostVel = XMVectorSet(ghostAttributes.velocity.x, ghostAttributes.velocity.y, ghostAttributes.velocity.z, 0.0f);
		float relativeVelocity = XMVectorGetX(XMVector3Dot(ghostVel, XMVector3Normalize(toListener)));

		// Low-pass filter for velocity changes
		float smoothedVelocity = 0.8f * lastRelativeVelocity + 0.2f * relativeVelocity;
		lastRelativeVelocity = smoothedVelocity;

		// Non-linear Doppler scaling
		float dopplerLevel = 1.0f + 0.3f * tanh(smoothedVelocity / 5.0f);
		channelGroup->set3DDopplerLevel(clamp(dopplerLevel, 0.8f, 1.2f));

		// Set spread for better spatialization (alternative to pan level)
		float spread = 180.0f * (1.0f - volume); // Wider spread when quieter
		channelGroup->set3DSpread(spread);
	}

	// Apply volume with perceptual loudness correction
	events.girlWhisper->setVolume(powf(volume, 0.6f));
}

void FMODAudioSystem::updateGhostEffects(float deltaTime, const XMFLOAT3& listenerPosition) {
	if (!events.girlWhisper) return;

	ghostEffectTimer += deltaTime;
	if (ghostEffectTimer >= GHOST_EFFECT_INTERVAL) {
		ghostEffectTimer = 0.0f;

		FMOD_3D_ATTRIBUTES ghostAttrs;
		events.girlWhisper->get3DAttributes(&ghostAttrs);
		float distance = sqrtf(powf(listenerPosition.x - ghostAttrs.position.x, 2) +
			powf(listenerPosition.y - ghostAttrs.position.y, 2) +
			powf(listenerPosition.z - ghostAttrs.position.z, 2));

		// Use Fletcher-Munson curves for perceived loudness
		float proximityFactor = 1.0f - min(powf(distance / WHISPER_FADE_RANGE, 0.3f), 1.0f);

		if (rand() % 3 == 0) {
			// Natural pitch variation within critical bandwidth
			float pitch = 1.0f + ((rand() % 11) - 5) * 0.005f * proximityFactor;
			events.girlWhisper->setPitch(pitch);

			if (distance < WHISPER_CLOSE_RANGE * 1.5f) {
				// Use transient shaping instead of volume boost
				FMOD::ChannelGroup* cg;
				events.girlWhisper->getChannelGroup(&cg);
				if (cg) {
					FMOD::System* coreSystem = nullptr;
					FMOD::DSP* transient = nullptr;

					studioSystem->getCoreSystem(&coreSystem);
					coreSystem->createDSPByType(FMOD_DSP_TYPE_TRANSCEIVER, &transient);

					transient->setParameterFloat(FMOD_DSP_TRANSCEIVER_GAIN, 2.0f);
					cg->addDSP(1, transient);
				}
			}
		}
	}
}

void FMODAudioSystem::setGhostEffectIntensity(float intensity) {
	ghostEffectIntensity = clamp(intensity, 0.0f, 1.0f);
}

void FMODAudioSystem::createIslandAmbience(const XMFLOAT3& position) {
	if (!studioSystem) return;

	FMOD::Studio::EventDescription* eventDesc = nullptr;
	FMOD_RESULT result = studioSystem->getEvent("event:/Ambience", &eventDesc);
	if (result != FMOD_OK || !eventDesc) return;

	FMOD::Studio::EventInstance* instance = nullptr;
	result = eventDesc->createInstance(&instance);
	if (result != FMOD_OK || !instance) return;

	// Set 3D attributes
	FMOD_3D_ATTRIBUTES attributes = { { 0 } };
	attributes.position = { position.x, position.y, position.z };
	instance->set3DAttributes(&attributes);

	// Start with full volume (we'll control it in update)
	instance->setVolume(1.0f); // Changed from 0.0f to 1.0f
	instance->start();

	// Ensure the ambience is playable
	instance->setPaused(false);

	islandAmbiences.push_back({ instance, position, true }); // Set active to true initially
}

void FMODAudioSystem::updateIslandAmbiences(const XMFLOAT3& listenerPos, int activeIslandIndex) {
	if (islandAmbiences.empty()) return;

	for (size_t i = 0; i < islandAmbiences.size(); i++) {
		auto& ambience = islandAmbiences[i];
		if (!ambience.instance) continue;

		// Calculate distance (ignore height for simpler calculation)
		float dx = ambience.position.x - listenerPos.x;
		float dz = ambience.position.z - listenerPos.z;
		float distance = sqrtf(dx * dx + dz * dz);

		// Determine if this is the active island
		bool isActiveIsland = (i == activeIslandIndex);

		// Calculate volume based on distance and active status
		float volume = 0.0f;
		if (isActiveIsland) {
			// Active island volume curve
			if (distance < AMBIENCE_MIN_DISTANCE) {
				volume = 1.0f; // Full volume when close
			}
			else if (distance < AMBIENCE_MAX_DISTANCE) {
				// Linear fade from MIN to MAX distance
				volume = 1.0f - ((distance - AMBIENCE_MIN_DISTANCE) /
					(AMBIENCE_MAX_DISTANCE - AMBIENCE_MIN_DISTANCE));
			}
			// Else volume stays 0
		}

		// Apply volume directly (no parameter needed)
		ambience.instance->setVolume(volume);

		// Update 3D position
		FMOD_3D_ATTRIBUTES attributes = { { 0 } };
		attributes.position = { ambience.position.x, ambience.position.y, ambience.position.z };
		ambience.instance->set3DAttributes(&attributes);

		// Debug output
		char buffer[256];
		sprintf_s(buffer, "Island %d: Active=%d, Dist=%.1f, Vol=%.2f\n",
			(int)i, isActiveIsland, distance, volume);
		OutputDebugStringA(buffer);
	}
}

void FMODAudioSystem::stopAllIslandAmbience() {
	for (auto& ambience : islandAmbiences) {
		if (ambience.instance) {
			ambience.instance->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);
			ambience.instance->release();
		}
	}
	islandAmbiences.clear();
}