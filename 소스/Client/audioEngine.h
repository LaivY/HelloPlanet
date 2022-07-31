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

	HRESULT Load(const string& fileName, const string& key, eAudioType audioType);
	void Play(const string& key, bool isLoop = false);
	void Stop(const string& key);
	void ChangeMusic(const string& key);

	void SetVolume(eAudioType audioType, FLOAT volume);
	int GetVolume(eAudioType audioType) const;
	int GetAudioCount() const;

private:
	HRESULT FindChunk(HANDLE hFile, DWORD fourcc, DWORD& dwChunkSize, DWORD& dwChunkDataPosition);
	HRESULT ReadChunkData(HANDLE hFile, void* buffer, DWORD buffersize, DWORD bufferoffset);

private:
	ComPtr<IXAudio2>					m_xAduio;
	unique_ptr<IXAudio2MasteringVoice>	m_masterVoice;
	unordered_map<string, AudioData>	m_audios;
	string								m_currMusicName;
	FLOAT								m_musicVolume;
	FLOAT								m_soundVolume;

	BOOL								m_isChanging;
	BOOL								m_isDecreasing;
	string								m_targetMusicName;
	FLOAT								m_tempVolume;
	FLOAT								m_timer;
};