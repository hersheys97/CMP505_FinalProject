#include "FMODAudioSystem.h"

// Clamps a value between min and max
template<typename T>
T clamp(const T& value, const T& min, const T& max) {
	return (value < min) ? min : (value > max) ? max : value;
}

// Smoothly transitions between values (used for volume fading)
inline float smoothstep(float edge0, float edge1, float x) {
	x = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
	return x * x * (3.0f - 2.0f * x);
}

// Constructor - Initializes FMOD pointers to nullptr
FMODAudioSystem::FMODAudioSystem() : studioSystem(nullptr) {
	bgm.instance = nullptr;
	bgm.channelGroup = nullptr;
	bgm.lowpass = nullptr;
	events.bgm1 = nullptr;
	events.heavyWhisper = nullptr;
	events.girlWhisper = nullptr;
}

// Initializes FMOD audio system
bool FMODAudioSystem::init() {
	// First check if already initialized
	if (studioSystem) {
		return true;
	}

	FMOD_RESULT result;

	// 1. Create the main FMOD Studio system
	result = FMOD::Studio::System::create(&studioSystem);
	if (result != FMOD_OK || !studioSystem) {
		OutputDebugStringA("FMOD: Failed to create studio system");
		OutputDebugStringA(FMOD_ErrorString(result));
		OutputDebugStringA("\n");
		return false;
	}

	// 2. Initialize system with 64 channels (audio streams)
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

	// 3. Load sound banks (containing all audio events)
	const vector<string> bankPaths = {
		//"CMP505.bank", // For Release
		//"CMP505.strings.bank" // For Release
		"../FMOD Project/CMP505_Audio/Build/Desktop/CMP505.bank", // For Debug
		"../FMOD Project/CMP505_Audio/Build/Desktop/CMP505.strings.bank" // For Debug
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

	// Wait for banks to fully load before continuing
	studioSystem->flushCommands();
	return true;
}

// Plays background music (BGM1 event)
void FMODAudioSystem::playBGM1() {
	if (!studioSystem || events.bgm1) return; // Already playing

	// Get event description from FMOD project
	FMOD::Studio::EventDescription* desc = nullptr;
	FMOD_RESULT result = studioSystem->getEvent("event:/BGM1", &desc);
	if (result != FMOD_OK || !desc) return;

	// Create and start the music instance
	result = desc->createInstance(&events.bgm1);
	if (result == FMOD_OK && events.bgm1) {
		events.bgm1->start();
		bgm.started = true;
		bgm.targetVolume = 0.6f; // Start at 60% volume
	}
}

// Stops background music with fade-out
void FMODAudioSystem::stopBGM1() {
	if (events.bgm1) {
		events.bgm1->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT); // Smooth fade
		events.bgm1->release();
		events.bgm1 = nullptr;
		bgm.started = false;
	}
}

// Updates audio system every frame
void FMODAudioSystem::update(float deltaTime) {
	if (!studioSystem) return;

	// BGM volume control (smooth transitions)
	if (bgm.started && events.bgm1) {
		float currentVolume;
		events.bgm1->getVolume(&currentVolume);

		// Smoothly adjust volume toward target
		const float fadeSpeed = 0.2f; // How fast volume changes
		const float volumeDelta = fadeSpeed * deltaTime;

		if (currentVolume < bgm.targetVolume) {
			currentVolume = min(currentVolume + volumeDelta, bgm.targetVolume);
		}
		else if (currentVolume > bgm.targetVolume) {
			currentVolume = max(currentVolume - volumeDelta, bgm.targetVolume);
		}

		events.bgm1->setVolume(currentVolume);
	}

	// Restore BGM after dimming
	if (bgm.dimmed) {
		bgm.restoreTimer -= deltaTime;
		if (bgm.restoreTimer <= 0.0f && bgm.targetVolume < 1.0f) {
			bgm.targetVolume = 0.6f; // Return to normal volume

			// Remove lowpass filter effect
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

	studioSystem->update(); // Must be called every frame
}

// Plays a one-time sound effect
void FMODAudioSystem::playOneShot(const string& eventPath) {
	if (!studioSystem) return;

	// Find and play the sound event
	FMOD::Studio::EventDescription* desc = nullptr;
	FMOD_RESULT result = studioSystem->getEvent(eventPath.c_str(), &desc);
	if (result != FMOD_OK || !desc) return;

	FMOD::Studio::EventInstance* instance = nullptr;
	result = desc->createInstance(&instance);
	if (result == FMOD_OK && instance) {
		instance->start();
		instance->release(); // Automatically cleans up after playing
	}
}

// Temporarily lowers BGM volume (e.g., for dialogue)
void FMODAudioSystem::dimBGM(float duration, float targetVolume) {
	if (!bgm.started || !events.bgm1) return;

	// Logarithmic scaling matches human hearing perception
	bgm.targetVolume = log10(1 + 9 * targetVolume);
	bgm.restoreTimer = duration;
	bgm.dimmed = true;

	// Add audio filter to make music sound "muffled"
	if (!bgm.lowpass) {
		FMOD::System* coreSystem = nullptr;
		studioSystem->getCoreSystem(&coreSystem);
		if (!coreSystem) return;

		// Create EQ filter to reduce mid/high frequencies
		coreSystem->createDSPByType(FMOD_DSP_TYPE_THREE_EQ, &bgm.lowpass);
		if (bgm.lowpass) {
			bgm.lowpass->setParameterFloat(FMOD_DSP_THREE_EQ_MIDGAIN, -6.0f); // Reduce mids
			bgm.lowpass->setParameterFloat(FMOD_DSP_THREE_EQ_HIGHGAIN, -3.0f); // Reduce highs

			// Apply filter to music channel
			events.bgm1->getChannelGroup(&bgm.channelGroup);
			if (bgm.channelGroup) {
				bgm.channelGroup->addDSP(0, bgm.lowpass);

				// Add compressor to maintain perceived loudness
				FMOD::DSP* compressor;
				coreSystem->createDSPByType(FMOD_DSP_TYPE_COMPRESSOR, &compressor);
				compressor->setParameterFloat(FMOD_DSP_COMPRESSOR_THRESHOLD, -15.0f);
				bgm.channelGroup->addDSP(1, compressor);
			}
		}
	}
}

// Clean up all FMOD resources
void FMODAudioSystem::release() {
	// Release DSP effects
	if (bgm.lowpass) {
		if (bgm.channelGroup) {
			bgm.channelGroup->removeDSP(bgm.lowpass);
		}
		bgm.lowpass->release();
		bgm.lowpass = nullptr;
	}

	// Helper to release event instances
	auto releaseInstance = [](FMOD::Studio::EventInstance*& instance) {
		if (instance) {
			instance->stop(FMOD_STUDIO_STOP_IMMEDIATE);
			instance->release();
			instance = nullptr;
		}
		};

	// Release all sound events
	releaseInstance(events.bgm1);
	releaseInstance(events.heavyWhisper);
	releaseInstance(events.girlWhisper);

	// Release main system
	if (studioSystem) {
		studioSystem->release();
		studioSystem = nullptr;
	}
}

// Plays a ghost whisper sound at 3D position
void FMODAudioSystem::playGhostWhisper(const XMFLOAT3& position) {
	if (!studioSystem || events.girlWhisper) return; // Already playing

	// Get whisper event from FMOD
	FMOD::Studio::EventDescription* desc = nullptr;
	FMOD_RESULT result = studioSystem->getEvent("event:/GirlWhisper", &desc);
	if (result != FMOD_OK || !desc) return;

	// Create 3D sound instance
	result = desc->createInstance(&events.girlWhisper);
	if (result == FMOD_OK && events.girlWhisper) {
		// Set 3D position/orientation
		FMOD_3D_ATTRIBUTES attributes = { { 0 } };
		attributes.position = { position.x, position.y, position.z };
		attributes.forward = { 0, 0, 1 }; // Facing forward
		attributes.up = { 0, 1, 0 };// Up direction
		events.girlWhisper->set3DAttributes(&attributes);

		events.girlWhisper->setVolume(0.3f); // Start volume
		events.girlWhisper->start();

		// Initialize ghost effect variables
		ghostEffectIntensity = 0.0f;
		ghostEffectTimer = 0.0f;
	}
}

// Stops ghost whisper with fade-out
void FMODAudioSystem::stopGhostWhisper() {
	if (events.girlWhisper) {
		events.girlWhisper->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);
		events.girlWhisper->release();
		events.girlWhisper = nullptr;
	}
}

// Updates ghost's 3D position (for movement/Doppler effect)
void FMODAudioSystem::updateGhostPosition(const XMFLOAT3& position) {
	if (events.girlWhisper) {
		FMOD_3D_ATTRIBUTES attributes = { { 0 } };
		attributes.position = { position.x, position.y, position.z };
		attributes.forward = { 0, 0, 1 };
		attributes.up = { 0, 1, 0 };

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

// Updates listener (player) position/orientation
void FMODAudioSystem::updateListenerPosition(const XMFLOAT3& position, const XMFLOAT3& forward, const XMFLOAT3& up) {
	if (!studioSystem) return;

	FMOD_3D_ATTRIBUTES attributes = { { 0 } };
	attributes.position = { position.x, position.y, position.z };
	attributes.forward = { forward.x, forward.y, forward.z }; // Where listener is facing
	attributes.up = { up.x, up.y, up.z };// Up direction

	studioSystem->setListenerAttributes(0, &attributes);
}

// Adjusts ghost whisper volume based on distance to listener
void FMODAudioSystem::updateGhostWhisperVolume(const XMFLOAT3& listenerPosition) {
	if (!events.girlWhisper) return;

	FMOD_3D_ATTRIBUTES ghostAttributes;
	events.girlWhisper->get3DAttributes(&ghostAttributes);

	// Calculate distance between ghost and listener
	XMVECTOR listenerPos = XMVectorSet(listenerPosition.x, listenerPosition.y, listenerPosition.z, 0.0f);
	XMVECTOR ghostPos = XMVectorSet(ghostAttributes.position.x, ghostAttributes.position.y, ghostAttributes.position.z, 0.0f);
	XMVECTOR toListener = XMVectorSubtract(listenerPos, ghostPos);
	float distance = XMVectorGetX(XMVector3Length(toListener));

	// Logarithmic volume fade (matches human hearing)
	float volume = 1.0f - (log10(1 + distance) / log10(1 + WHISPER_FADE_RANGE));
	volume = clamp(volume * (1.0f + ghostEffectIntensity * 0.3f), WHISPER_MIN_VOLUME, 1.2f);

	// Get channel group for advanced 3D settings
	FMOD::ChannelGroup* channelGroup;
	events.girlWhisper->getChannelGroup(&channelGroup);

	if (channelGroup) {
		// Set sound direction
		FMOD_VECTOR dir;
		XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&dir), XMVector3Normalize(toListener));
		channelGroup->set3DConeOrientation(&dir);

		// Adjust sound cone based on distance (narrower when close)
		float coneInside, coneOutside;
		if (distance < WHISPER_CLOSE_RANGE) {
			coneInside = 30.0f;  // Narrow cone
			coneOutside = 90.0f;
		}
		else if (distance < WHISPER_MID_RANGE) {
			coneInside = 60.0f;
			coneOutside = 150.0f;
		}
		else {
			coneInside = 90.0f;  // Wide cone
			coneOutside = 180.0f;
		}
		channelGroup->set3DConeSettings(coneInside, coneOutside, 0.3f);

		// Smooth Doppler effect (pitch change when moving)
		static float lastRelativeVelocity = 0.0f;
		XMVECTOR ghostVel = XMVectorSet(ghostAttributes.velocity.x, ghostAttributes.velocity.y, ghostAttributes.velocity.z, 0.0f);
		float relativeVelocity = XMVectorGetX(XMVector3Dot(ghostVel, XMVector3Normalize(toListener)));

		// Low-pass filter for smoother changes
		float smoothedVelocity = 0.8f * lastRelativeVelocity + 0.2f * relativeVelocity;
		lastRelativeVelocity = smoothedVelocity;

		// Adjust Doppler intensity (non-linear)
		float dopplerLevel = 1.0f + 0.3f * tanh(smoothedVelocity / 5.0f);
		channelGroup->set3DDopplerLevel(clamp(dopplerLevel, 0.8f, 1.2f));

		// Wider sound spread when quieter
		float spread = 180.0f * (1.0f - volume);
		channelGroup->set3DSpread(spread);
	}

	// Apply volume with perceptual correction
	events.girlWhisper->setVolume(powf(volume, 0.6f));
}

// Updates ghost sound effects (pitch variations, etc.)
void FMODAudioSystem::updateGhostEffects(float deltaTime, const XMFLOAT3& listenerPosition) {
	if (!events.girlWhisper) return;

	ghostEffectTimer += deltaTime;
	if (ghostEffectTimer >= GHOST_EFFECT_INTERVAL) {
		ghostEffectTimer = 0.0f;

		// Get ghost position and distance to listener
		FMOD_3D_ATTRIBUTES ghostAttrs;
		events.girlWhisper->get3DAttributes(&ghostAttrs);
		float distance = sqrtf(powf(listenerPosition.x - ghostAttrs.position.x, 2) +
			powf(listenerPosition.y - ghostAttrs.position.y, 2) +
			powf(listenerPosition.z - ghostAttrs.position.z, 2));

		// Adjust effects based on proximity
		float proximityFactor = 1.0f - min(powf(distance / WHISPER_FADE_RANGE, 0.3f), 1.0f);

		// Randomly apply effects (1 in 3 chance)
		if (rand() % 3 == 0) {
			// Small pitch variation (creates unnatural feel)
			float pitch = 1.0f + ((rand() % 11) - 5) * 0.005f * proximityFactor;
			events.girlWhisper->setPitch(pitch);

			// Extra effect when very close
			if (distance < WHISPER_CLOSE_RANGE * 1.5f) {
				FMOD::ChannelGroup* cg;
				events.girlWhisper->getChannelGroup(&cg);
				if (cg) {
					// Add temporary volume boost effect
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

// Sets intensity of ghost sound effects (0-1)
void FMODAudioSystem::setGhostEffectIntensity(float intensity) {
	ghostEffectIntensity = clamp(intensity, 0.0f, 1.0f);
}

// Creates ambient sound at island position
void FMODAudioSystem::createIslandAmbience(const XMFLOAT3& position) {
	if (!studioSystem) return;

	// Get ambience event
	FMOD::Studio::EventDescription* eventDesc = nullptr;
	FMOD_RESULT result = studioSystem->getEvent("event:/Ambience", &eventDesc);
	if (result != FMOD_OK || !eventDesc) return;

	// Create instance at 3D position
	FMOD::Studio::EventInstance* instance = nullptr;
	result = eventDesc->createInstance(&instance);
	if (result != FMOD_OK || !instance) return;

	FMOD_3D_ATTRIBUTES attributes = { { 0 } };
	attributes.position = { position.x, position.y, position.z };
	instance->set3DAttributes(&attributes);

	instance->setVolume(1.0f); // Start at full volume
	instance->start();
	instance->setPaused(false);

	// Store in list of active ambiences
	islandAmbiences.push_back({ instance, position, true });
}

// Updates all island ambiences based on listener position
void FMODAudioSystem::updateIslandAmbiences(const XMFLOAT3& listenerPos, int activeIslandIndex) {
	if (islandAmbiences.empty()) return;

	for (size_t i = 0; i < islandAmbiences.size(); i++) {
		auto& ambience = islandAmbiences[i];
		if (!ambience.instance) continue;

		// Calculate horizontal distance (ignore height)
		float dx = ambience.position.x - listenerPos.x;
		float dz = ambience.position.z - listenerPos.z;
		float distance = sqrtf(dx * dx + dz * dz);

		bool isActiveIsland = (i == activeIslandIndex);

		// Calculate volume: full when close, fading with distance
		float volume = 0.0f;
		if (isActiveIsland) {
			if (distance < AMBIENCE_MIN_DISTANCE) {
				volume = 1.0f; // Full volume
			}
			else if (distance < AMBIENCE_MAX_DISTANCE) {
				// Linear fade between min and max distance
				volume = 1.0f - ((distance - AMBIENCE_MIN_DISTANCE) /
					(AMBIENCE_MAX_DISTANCE - AMBIENCE_MIN_DISTANCE));
			}
		}

		ambience.instance->setVolume(volume);

		// Update 3D position (even if not audible)
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

// Stops all island ambience sounds
void FMODAudioSystem::stopAllIslandAmbience() {
	for (auto& ambience : islandAmbiences) {
		if (ambience.instance) {
			ambience.instance->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);
			ambience.instance->release();
		}
	}
	islandAmbiences.clear();
}