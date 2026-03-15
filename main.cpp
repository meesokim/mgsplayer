#define SDL_MAIN_HANDLED
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <iomanip>
#include <SDL2/SDL.h>

extern "C" {
#include "libkss/src/kssplay.h"
#include "libkss/src/kss/kss.h"
}

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>

// Function to setup file association on Windows
void setupFileAssociation() {
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);

    const char* ext = ".mgs";
    const char* progId = "MGSPlayer.File";
    const char* description = "MSX Music File (.mgs)";

    HKEY hKey;
    // 1. Create .mgs extension key
    if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Classes\\.mgs", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, "", 0, REG_SZ, (const BYTE*)progId, strlen(progId) + 1);
        RegCloseKey(hKey);
    }

    // 2. Create MGSPlayer.File key
    char command[MAX_PATH + 10];
    sprintf(command, "\"%s\" \"%%1\"", exePath);

    if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Classes\\MGSPlayer.File", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, "", 0, REG_SZ, (const BYTE*)description, strlen(description) + 1);
        
        HKEY hCmdKey;
        if (RegCreateKeyExA(hKey, "shell\\open\\command", 0, NULL, 0, KEY_WRITE, NULL, &hCmdKey, NULL) == ERROR_SUCCESS) {
            RegSetValueExA(hCmdKey, "", 0, REG_SZ, (const BYTE*)command, strlen(command) + 1);
            RegCloseKey(hCmdKey);
        }
        RegCloseKey(hKey);
    }

    // Notify Windows that file associations have changed
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
}
#else
#include <iconv.h>
#endif

// Global engine objects
KSSPLAY* kssplay = nullptr;
KSS* kss = nullptr;
std::ofstream wavFile;
uint32_t totalAudioBytes = 0;

// Shift-JIS to UTF-8 conversion using native APIs
std::string sjisToUtf8(const std::string& sjis) {
    if (sjis.empty()) return "";

#ifdef _WIN32
    // Windows implementation using MultiByteToWideChar
    int nwLen = MultiByteToWideChar(932, 0, sjis.c_str(), -1, NULL, 0);
    if (nwLen <= 0) return sjis;
    std::vector<wchar_t> wbuf(nwLen);
    MultiByteToWideChar(932, 0, sjis.c_str(), -1, wbuf.data(), nwLen);

    int nLen = WideCharToMultiByte(CP_UTF8, 0, wbuf.data(), -1, NULL, 0, NULL, NULL);
    if (nLen <= 0) return sjis;
    std::vector<char> buf(nLen);
    WideCharToMultiByte(CP_UTF8, 0, wbuf.data(), -1, buf.data(), nLen, NULL, NULL);
    return std::string(buf.data());
#else
    // Linux/POSIX implementation using iconv
    iconv_t cd = iconv_open("UTF-8", "SHIFT-JIS");
    if (cd == (iconv_t)-1) return sjis;

    size_t inLen = sjis.length();
    size_t outLen = inLen * 3 + 1;
    std::vector<char> outBuf(outLen);
    char* inPtr = const_cast<char*>(sjis.c_str());
    char* outPtr = outBuf.data();
    size_t inBytesLeft = inLen;
    size_t outBytesLeft = outLen - 1;

    if (iconv(cd, &inPtr, &inBytesLeft, &outPtr, &outBytesLeft) == (size_t)-1) {
        iconv_close(cd);
        return sjis;
    }
    *outPtr = '\0';
    iconv_close(cd);
    return std::string(outBuf.data());
#endif
}

// WAV Header structure for debugging
struct WAVHeader {
    char riff[4] = {'R', 'I', 'F', 'F'};
    uint32_t fileSize;
    char wave[4] = {'W', 'A', 'V', 'E'};
    char fmt[4] = {'f', 'm', 't', ' '};
    uint32_t fmtLen = 16;
    uint16_t format = 1; // PCM
    uint16_t channels = 2;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign = 4;
    uint16_t bitsPerSample = 16;
    char data[4] = {'d', 'a', 't', 'a'};
    uint32_t dataLen;
};

// SDL Audio Callback
void audioCallback(void* userdata, Uint8* stream, int len) {
    if (kssplay) {
        int16_t* buffer = reinterpret_cast<int16_t*>(stream);
        int samples = len / sizeof(int16_t) / 2; // Stereo = /2

        KSSPLAY_calc(kssplay, buffer, samples);

        // Write to debug WAV file if open
        if (wavFile.is_open()) {
            wavFile.write(reinterpret_cast<char*>(stream), len);
            totalAudioBytes += len;
        }
    } else {
        SDL_memset(stream, 0, len);
    }
}

std::string formatTime(int ms) {
    int totalSeconds = ms / 1000;
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(2) << minutes << ":" << std::setw(2) << seconds;
    return ss.str();
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    // Set console output to UTF-8 for Windows
    SetConsoleOutputCP(65001);
    setupFileAssociation();
#endif
    SDL_SetMainReady();
    bool debugMode = false;
    std::string filename;

    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "-debug") {
            debugMode = true;
        } else {
            filename = argv[i];
        }
    }

    if (filename.empty()) {
        std::cerr << "Usage: " << argv[0] << " [-debug] <filename.mgs>" << std::endl;
        std::cerr << "On Windows, .mgs files are automatically associated with this player on first run." << std::endl;
        return 1;
    }

    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0) {
        std::cerr << "SDL Initialization failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Load MGS file
    kss = KSS_load_file(const_cast<char*>(filename.c_str()));
    if (!kss) {
        std::cerr << "Failed to load MGS file: " << filename << std::endl;
        SDL_Quit();
        return 1;
    }

    if (kss->title[0] != '\0') {
        std::string rawTitle(reinterpret_cast<char*>(kss->title));
        std::cout << "Title: " << sjisToUtf8(rawTitle) << std::endl;
    }

    uint32_t targetFreq = 44100;
    SDL_AudioDeviceID dev = 0;

    if (!debugMode) {
        // Configure Audio for real-time
        SDL_AudioSpec want, have;
        SDL_zero(want);
        want.freq = targetFreq;
        want.format = AUDIO_S16SYS;
        want.channels = 2;
        want.samples = 4096; // Optimal buffer
        want.callback = audioCallback;

        dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
        if (dev == 0) {
            std::cerr << "Failed to open audio device: " << SDL_GetError() << std::endl;
            KSS_delete(kss);
            SDL_Quit();
            return 1;
        }
        targetFreq = have.freq;
    }

    // Open debug WAV file
    wavFile.open("debug_output.wav", std::ios::binary);
    if (wavFile.is_open()) {
        WAVHeader dummyHeader;
        wavFile.write(reinterpret_cast<char*>(&dummyHeader), sizeof(WAVHeader));
    }

    // Initialize KSSPLAY engine
    kssplay = KSSPLAY_new(targetFreq, 2, 16);
    KSSPLAY_set_data(kssplay, kss);
    
    // Maximize volume and clarity
    KSSPLAY_set_master_volume(kssplay, 80);
    for (int i = 0; i < KSS_DEVICE_MAX; i++) {
        KSSPLAY_set_device_quality(kssplay, (KSS_DEVICE)i, 1);
    }
    KSSPLAY_set_dcf(kssplay, 1);
    KSSPLAY_set_rcf(kssplay, 0, 0); // Clear digital sound
    KSSPLAY_reset(kssplay, 0, 0);

    if (debugMode) {
        std::cout << "Debug mode: Generating debug_output.wav without playback..." << std::endl;
        int16_t buffer[4096];
        while (!KSSPLAY_get_stop_flag(kssplay) && totalAudioBytes < (targetFreq * 4 * 300)) { // Max 5 mins
            KSSPLAY_calc(kssplay, buffer, 2048);
            int len = 2048 * sizeof(int16_t) * 2;
            wavFile.write(reinterpret_cast<char*>(buffer), len);
            totalAudioBytes += len;
            
            if (totalAudioBytes % (targetFreq * 4 * 10) == 0) {
                std::cout << "." << std::flush;
            }
        }
        std::cout << "\nDone." << std::endl;
    } else {
        SDL_PauseAudioDevice(dev, 0);
        std::cout << "Playing: " << filename << " (Press Ctrl+C to stop)" << std::endl;

        // Main loop
        bool running = true;
        SDL_Event event;
        uint32_t last_time = 0;

        while (running) {
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) running = false;
            }

            if (KSSPLAY_get_stop_flag(kssplay)) {
                std::cout << "\nPlayback finished." << std::endl;
                running = false;
            }

            uint32_t current_ticks = SDL_GetTicks();
            if (current_ticks - last_time >= 500) {
                int current_ms = kssplay->decoded_length * 1000LL / targetFreq;
                std::cout << "\rTime: [" << formatTime(current_ms) << "] " << std::flush;
                last_time = current_ticks;
            }
            SDL_Delay(100);
        }
    }

    // Finalize WAV Header
    if (wavFile.is_open()) {
        wavFile.seekp(0);
        WAVHeader header;
        header.sampleRate = targetFreq;
        header.byteRate = targetFreq * 4;
        header.dataLen = totalAudioBytes;
        header.fileSize = totalAudioBytes + sizeof(WAVHeader) - 8;
        wavFile.write(reinterpret_cast<char*>(&header), sizeof(WAVHeader));
        wavFile.close();
        std::cout << "\nDebug WAV saved to 'debug_output.wav'" << std::endl;
    }

    SDL_PauseAudioDevice(dev, 1);
    SDL_CloseAudioDevice(dev);
    if (kssplay) KSSPLAY_delete(kssplay);
    if (kss) KSS_delete(kss);
    SDL_Quit();

    return 0;
}
