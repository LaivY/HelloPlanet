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

AudioEngine::AudioEngine() : m_musicVolume{ 1.0f }, m_soundVolume{ 1.0f }, m_volumeTimer{ 0.0f }
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

}

HRESULT AudioEngine::Load(const wstring& fileName, AudioType audioType)
{
	AudioData audioData{};
	audioData.audioType = audioType;

	// Open the file
	HANDLE hFile = CreateFile(fileName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
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

	m_audios[fileName] = move(audioData);
	return S_OK;
}

void AudioEngine::Play(const wstring& fileName, bool isLoop)
{
	auto& audioData{ m_audios.at(fileName) };
	if (isLoop)
	{
		audioData.buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
	}
	audioData.pSourceVoice->Stop();
	audioData.pSourceVoice->FlushSourceBuffers();
	audioData.pSourceVoice->SubmitSourceBuffer(&audioData.buffer);
	m_audios.at(fileName).pSourceVoice->Start();
}

void AudioEngine::SetVolume(AudioType audioType, FLOAT volume)
{
	for (auto& [k, v] : m_audios)
	{
		if (v.audioType != audioType) continue;
		v.pSourceVoice->SetVolume(volume);
	}
}

void AudioEngine::TurnOnVolume(FLOAT time)
{

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