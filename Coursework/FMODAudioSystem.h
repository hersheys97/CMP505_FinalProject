#pragma once
#include "DXF.h"
#include <DirectXMath.h>
#include <algorithm>

#include "fmod_studio.hpp"
#include "fmod.hpp"
#include "fmod_errors.h"
#pragma comment(lib, "fmod_vc.lib") // Core FMOD library
#pragma comment(lib, "fmodstudio_vc.lib") // FMOD Studio library

using namespace std;

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

	struct IslandAmbience {
		FMOD::Studio::EventInstance* instance = nullptr;
		XMFLOAT3 position;
		bool active = false;
	};

	vector<IslandAmbience> islandAmbiences;

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

	void playGhostWhisper(const XMFLOAT3& position);
	void stopGhostWhisper();
	void updateGhostPosition(const XMFLOAT3& position);

	void updateListenerPosition(const XMFLOAT3& position, const XMFLOAT3& forward, const XMFLOAT3& up);
	void updateGhostWhisperVolume(const XMFLOAT3& listenerPosition);

	void updateGhostEffects(float deltaTime, const XMFLOAT3& listenerPosition);
	void setGhostEffectIntensity(float intensity);  // 0.0f to 1.0f

	void createIslandAmbience(const XMFLOAT3& position);
	void updateIslandAmbiences(const XMFLOAT3& listenerPosition);
	void stopAllIslandAmbience();

private:
	// Ghost effect parameters
	float ghostEffectIntensity = 0.0f;
	float ghostEffectTimer = 0.0f;

	static constexpr float BGM_DIM_VOLUME = 0.3f;

	static constexpr float MIN_WHISPER_VOLUME = 0.2f;
	static constexpr float MAX_WHISPER_VOLUME = 1.f;

	static constexpr float WHISPER_CLOSE_RANGE = 15.0f;  // Very close range (intimate)
	static constexpr float WHISPER_MID_RANGE = 30.0f;    // Medium range
	static constexpr float WHISPER_FAR_RANGE = 40.0f;    // Far range
	static constexpr float WHISPER_FADE_RANGE = 50.0f;   // Complete fade-out beyond this

	static constexpr float WHISPER_CLOSE_VOLUME = 1.0f;  // Full volume when very close
	static constexpr float WHISPER_MID_VOLUME = 0.6f;    // Medium volume
	static constexpr float WHISPER_FAR_VOLUME = 0.2f;    // Quiet when far
	static constexpr float WHISPER_MIN_VOLUME = 0.0f;    // Minimum volume before mute

	static constexpr float AMBIENCE_MIN_DISTANCE = 55.0f;
	static constexpr float AMBIENCE_MAX_DISTANCE = 70.0f;
	static constexpr float AMBIENCE_CUTOFF_DISTANCE = 60.0f;

	static constexpr float GHOST_EFFECT_INTERVAL = 1.5f;
};