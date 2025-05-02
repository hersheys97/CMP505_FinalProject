#include "FMODAudioSystem.h"

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

// Dim BGM with optional lowpass effect
void FMODAudioSystem::dimBGM(float duration, float targetVolume) {
	if (!bgm.started || !events.bgm1) return;

	bgm.targetVolume = targetVolume;
	bgm.restoreTimer = duration;
	bgm.dimmed = true;

	// Add lowpass effect if not already present
	if (!bgm.lowpass) {
		FMOD::System* coreSystem = nullptr;
		studioSystem->getCoreSystem(&coreSystem);
		if (!coreSystem) return;

		coreSystem->createDSPByType(FMOD_DSP_TYPE_LOWPASS, &bgm.lowpass);
		if (bgm.lowpass) {
			bgm.lowpass->setParameterFloat(FMOD_DSP_LOWPASS_CUTOFF, 800.0f);

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
	if (!studioSystem || events.girlWhisper) return; // Already playing

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

		// Set max distance (radius)
		events.girlWhisper->setProperty(FMOD_STUDIO_EVENT_PROPERTY_MAXIMUM_DISTANCE, 50.0f);

		// Start the event
		events.girlWhisper->start();
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
		attributes.forward = { 0, 0, 1 };
		attributes.up = { 0, 1, 0 };
		events.girlWhisper->set3DAttributes(&attributes);
	}
}