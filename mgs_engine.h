#ifndef MGS_ENGINE_H
#define MGS_ENGINE_H

#include <vector>
#include <string>
#include <cstdint>

// 실제 구현에서는 여기에 각 사운드 칩(emu2413, emu2149 등) 헤더가 포함됩니다.
// 예: #include "emu2413.h", #include "emu2149.h"

class MGSEngine {
public:
    MGSEngine(uint32_t sampleRate = 44100);
    ~MGSEngine();

    bool loadMGS(const std::string& filename);
    void play();
    void stop();
    
    // SDL2 오디오 콜백에서 호출됨
    void render(int16_t* buffer, int length);

private:
    uint32_t sampleRate;
    bool isPlaying = false;

    // Z80 가상 메모리 (64KB)
    uint8_t memory[65536];
    uint16_t pc; // Program Counter

    // 사운드 칩 상태 (실제 구현 시 라이브러리 객체로 대체)
    // OPLL* opll;
    // PSG* psg;
    // SCC* scc;

    void initializeZ80();
    void stepZ80(); // 1스텝 실행 또는 V-Blank 인터럽트 처리
    void updateSound(int samples);
};

#endif // MGS_ENGINE_H
