#pragma once

enum class AudioType
{
	MUSIC, SOUND
};

struct AudioData
{
	AudioType				audioType;
	unique_ptr<BYTE[]>		pDataBuffer;
	IXAudio2SourceVoice*	pSourceVoice = nullptr;
	WAVEFORMATEXTENSIBLE	wfx = {};
	XAUDIO2_BUFFER			buffer = {};
};

class AudioEngine
{
public:
	AudioEngine();
	~AudioEngine();

	void Update(FLOAT deltaTime);

	HRESULT Load(const wstring& fileName, AudioType audioType);
	void Play(const wstring& fileName, bool isLoop = false);

	void SetVolume(AudioType audioType, FLOAT volume);
	void TurnOnVolume(FLOAT time);

private:
	HRESULT FindChunk(HANDLE hFile, DWORD fourcc, DWORD& dwChunkSize, DWORD& dwChunkDataPosition);
	HRESULT ReadChunkData(HANDLE hFile, void* buffer, DWORD buffersize, DWORD bufferoffset);

private:
	ComPtr<IXAudio2>					m_xAduio;
	unique_ptr<IXAudio2MasteringVoice>	m_masterVoice;
	unordered_map<wstring, AudioData>	m_audios;
	FLOAT								m_musicVolume;
	FLOAT								m_soundVolume;
	FLOAT								m_volumeTimer;
};