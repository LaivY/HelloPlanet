#pragma once

enum class eAudioType
{
	MUSIC, SOUND
};

struct AudioData
{
	eAudioType				audioType;
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

	HRESULT Load(const wstring& fileName, eAudioType audioType);
	void Play(const wstring& fileName, bool isLoop = false);

	void SetVolume(eAudioType audioType, FLOAT volume);
	int GetVolume(eAudioType audioType);

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