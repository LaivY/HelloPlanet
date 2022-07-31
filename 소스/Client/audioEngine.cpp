#include "stdafx.h"
#include "audioEngine.h"

#ifdef _XBOX //Big-Endian
#define fourccRIFF 'RIFF'
#define fourccDATA 'data'
#define fourccFMT 'fmt '
#define fourccWAVE 'WAVE'
#define fourccXWMA 'XWMA'
#define fourccDPDS 'dpds'
#endif

#ifndef _XBOX //Little-Endian
#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT ' tmf'
#define fourccWAVE 'EVAW'
#define fourccXWMA 'AMWX'
#define fourccDPDS 'sdpd'
#endif

AudioEngine::AudioEngine() : m_musicVolume{ 0.5f }, m_soundVolume{ 0.5f }, m_isChanging{ FALSE }, m_isDecreasing{ FALSE }, m_tempVolume{ 0.0f }, m_timer{ 0.0f }
{
	DX::ThrowIfFailed(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
	DX::ThrowIfFailed(XAudio2Create(&m_xAduio, 0, XAUDIO2_DEFAULT_PROCESSOR));
	auto masterVoice{ m_masterVoice.get() };
	DX::ThrowIfFailed(m_xAduio->CreateMasteringVoice(&masterVoice));
}

AudioEngine::~AudioEngine()
{
	for (auto& [k, v] : m_audios)
	{
		if (v.pSourceVoice)
			v.pSourceVoice->DestroyVoice();
	}
	if (m_masterVoice)
		m_masterVoice->DestroyVoice();
}

void AudioEngine::Update(FLOAT deltaTime)
{
	constexpr float CHANGE_TIME{ 0.5f };

	if (!m_isChanging) return;
	if (m_isDecreasing)
	{
		SetVolume(eAudioType::MUSIC, lerp(m_tempVolume, 0.0f, m_timer / CHANGE_TIME));
		if (m_timer >= CHANGE_TIME)
		{
			Stop(m_currMusicName);
			Play(m_targetMusicName, true);
			m_isDecreasing = FALSE;
			m_timer = 0.0f;
		}
	}
	else
	{
		SetVolume(eAudioType::MUSIC, lerp(0.0f, m_tempVolume, m_timer / CHANGE_TIME));
		if (m_timer >= CHANGE_TIME)
		{
			m_isChanging = FALSE;
		}
	}
	m_timer = min(CHANGE_TIME, m_timer + deltaTime);
}

HRESULT AudioEngine::Load(const string& fileName, const string& key, eAudioType audioType)
{
	AudioData audioData{};
	audioData.audioType = audioType;

	// Open the file
	HANDLE hFile = CreateFileA(fileName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (INVALID_HANDLE_VALUE == hFile)
		return HRESULT_FROM_WIN32(GetLastError());
	if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
		return HRESULT_FROM_WIN32(GetLastError());

	DWORD dwChunkSize;
	DWORD dwChunkPosition;
	//check the file type, should be fourccWAVE or 'XWMA'
	FindChunk(hFile, fourccRIFF, dwChunkSize, dwChunkPosition);
	DWORD filetype;
	ReadChunkData(hFile, &filetype, sizeof(DWORD), dwChunkPosition);
	if (filetype != fourccWAVE)
		return S_FALSE;

	FindChunk(hFile, fourccFMT, dwChunkSize, dwChunkPosition);
	ReadChunkData(hFile, &audioData.wfx, dwChunkSize, dwChunkPosition);

	//fill out the audio data buffer with the contents of the fourccDATA chunk
	FindChunk(hFile, fourccDATA, dwChunkSize, dwChunkPosition);
	audioData.pDataBuffer = make_unique<BYTE[]>(dwChunkSize);
	ReadChunkData(hFile, audioData.pDataBuffer.get(), dwChunkSize, dwChunkPosition);

	audioData.buffer.AudioBytes = dwChunkSize;  //size of the audio buffer in bytes
	audioData.buffer.pAudioData = audioData.pDataBuffer.get();  //buffer containing audio data
	audioData.buffer.Flags = XAUDIO2_END_OF_STREAM; // tell the source voice not to expect any data after this buffer

	DX::ThrowIfFailed(m_xAduio->CreateSourceVoice(&audioData.pSourceVoice, (WAVEFORMATEX*)&audioData.wfx));
	DX::ThrowIfFailed(audioData.pSourceVoice->SubmitSourceBuffer(&audioData.buffer));

	m_audios[key] = move(audioData);
	return S_OK;
}

void AudioEngine::Play(const string& key, bool isLoop)
{
	auto& audioData{ m_audios.at(key) };
	if (isLoop)
		audioData.buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
	audioData.pSourceVoice->Stop();
	audioData.pSourceVoice->FlushSourceBuffers();
	audioData.pSourceVoice->SubmitSourceBuffer(&audioData.buffer);

	switch (audioData.audioType)
	{
	case eAudioType::MUSIC:
		audioData.pSourceVoice->SetVolume(m_musicVolume);
		m_currMusicName = key;
		break;
	case eAudioType::SOUND:
		audioData.pSourceVoice->SetVolume(m_soundVolume);
		break;
	}
	audioData.pSourceVoice->Start();
}

void AudioEngine::Stop(const string& key)
{
	if (!m_audios.contains(key)) return;
	m_audios[key].pSourceVoice->Stop();
}

void AudioEngine::ChangeMusic(const string& key)
{
	if (!m_audios.contains(key)) return;
	if (m_currMusicName == key) return;
	m_isChanging = TRUE;
	m_isDecreasing = TRUE;
	m_targetMusicName = key;
	m_tempVolume = m_musicVolume;
	m_timer = 0.0f;
}

void AudioEngine::SetVolume(eAudioType audioType, FLOAT volume)
{
	for (auto& [k, v] : m_audios)
	{
		if (v.audioType != audioType) continue;
		v.pSourceVoice->SetVolume(volume);
	}
	if (audioType == eAudioType::MUSIC)
		m_musicVolume = volume;
	else if (audioType == eAudioType::SOUND)
		m_soundVolume = volume;
}

int AudioEngine::GetVolume(eAudioType audioType) const
{
	switch (audioType)
	{
	case eAudioType::MUSIC:
		return static_cast<int>(m_musicVolume * 100);
	case eAudioType::SOUND:
		return static_cast<int>(m_soundVolume * 100);
	}
	return 0;
}

int AudioEngine::GetAudioCount() const
{
	return static_cast<int>(m_audios.size());
}

HRESULT AudioEngine::FindChunk(HANDLE hFile, DWORD fourcc, DWORD& dwChunkSize, DWORD& dwChunkDataPosition)
{
	HRESULT hr = S_OK;
	if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
		return HRESULT_FROM_WIN32(GetLastError());

	DWORD dwChunkType{};
	DWORD dwChunkDataSize{};
	DWORD dwRIFFDataSize{};
	DWORD dwFileType{};
	DWORD bytesRead{};
	DWORD dwOffset{};

	while (hr == S_OK)
	{
		DWORD dwRead;
		if (0 == ReadFile(hFile, &dwChunkType, sizeof(DWORD), &dwRead, NULL))
			hr = HRESULT_FROM_WIN32(GetLastError());

		if (0 == ReadFile(hFile, &dwChunkDataSize, sizeof(DWORD), &dwRead, NULL))
			hr = HRESULT_FROM_WIN32(GetLastError());

		switch (dwChunkType)
		{
		case fourccRIFF:
			dwRIFFDataSize = dwChunkDataSize;
			dwChunkDataSize = 4;
			if (0 == ReadFile(hFile, &dwFileType, sizeof(DWORD), &dwRead, NULL))
				hr = HRESULT_FROM_WIN32(GetLastError());
			break;

		default:
			if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, dwChunkDataSize, NULL, FILE_CURRENT))
				return HRESULT_FROM_WIN32(GetLastError());
		}

		dwOffset += sizeof(DWORD) * 2;

		if (dwChunkType == fourcc)
		{
			dwChunkSize = dwChunkDataSize;
			dwChunkDataPosition = dwOffset;
			return S_OK;
		}

		dwOffset += dwChunkDataSize;

		if (bytesRead >= dwRIFFDataSize) return S_FALSE;
	}
	return S_OK;
}

HRESULT AudioEngine::ReadChunkData(HANDLE hFile, void* buffer, DWORD buffersize, DWORD bufferoffset)
{
	HRESULT hr = S_OK;
	if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, bufferoffset, NULL, FILE_BEGIN))
		return HRESULT_FROM_WIN32(GetLastError());
	DWORD dwRead;
	if (0 == ReadFile(hFile, buffer, buffersize, &dwRead, NULL))
		hr = HRESULT_FROM_WIN32(GetLastError());
	return hr;
}