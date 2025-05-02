#pragma once
#include "DXF.h"
#include <DirectXMath.h>

#include "fmod_studio.hpp"
#include "fmod.hpp"
#include "fmod_errors.h"
#pragma comment(lib, "fmod_vc.lib") // Core FMOD library
#pragma comment(lib, "fmodstudio_vc.lib") // FMOD Studio library

class FMODAudioSystem {
public:
	FMODAudioSystem();
	~FMODAudioSystem() { release(); }

	// Core system
	FMOD::Studio::System* studioSystem = nullptr;

	// BGM management
	struct BGM {
		FMOD::Studio::EventInstance* instance = nullptr;
		FMOD::ChannelGroup* channelGroup = nullptr;
		FMOD::DSP* lowpass = nullptr;

		bool started = false;
		bool dimmed = false;
		float targetVolume = 1.0f;
		float restoreTimer = 0.0f;
	} bgm;

	// Event instances
	struct EventInstances {
		FMOD::Studio::EventInstance* bgm1 = nullptr;
		FMOD::Studio::EventInstance* heavyWhisper = nullptr;
		FMOD::Studio::EventInstance* girlWhisper = nullptr;
	} events;

	// Constants
	static constexpr float ECHO_EFFECT_DURATION = 3.0f;

	// Check
	bool ghostAudioPlaying = false;
	bool isWhisperPlaying() const { return events.girlWhisper != nullptr; }

	// Methods
	bool init();
	void release();
	void playBGM1();
	void stopBGM1();
	void update(float deltaTime);
	void playOneShot(const std::string& eventPath);
	void dimBGM(float duration, float targetVolume = 0.3f);

	void playFireflyWhisper(const XMFLOAT3& position);
	void stopFireflyWhisper();
	void updateFireflyPosition(const XMFLOAT3& position);
};