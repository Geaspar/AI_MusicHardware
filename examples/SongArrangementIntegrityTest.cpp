// Headless test to validate note-on timing in Song playback mode,
// specifically around song loop boundaries to ensure no ghosts or misses.
#include <iostream>
#include <vector>
#include <map>
#include <cmath>
#include "../include/sequencer/Sequencer.h"

using namespace AIMusicHardware;

static void addPattern16(Sequencer& seq, int bpb, const std::vector<int>& cols16, int pitch, double stepDurBeats, size_t& outIndex) {
    auto p = std::make_unique<Pattern>("song_test");
    for (int c : cols16) {
        double start = (double)bpb/16.0 * (double)c;
        p->addNote(Note(pitch, 1.0f, start, stepDurBeats, 0));
    }
    p->setLength((double)bpb);
    seq.addPattern(std::move(p));
    outIndex = seq.getNumPatterns() - 1;
}

int main() {
    Sequencer seq(120.0, 4);
    if (!seq.initialize()) { std::cerr << "Sequencer init failed\n"; return 1; }
    const int bpb = seq.getBeatsPerBar();
    const double stepBeats = (double)bpb / 16.0;

    // Pattern with consecutive 16ths to stress retrigger boundaries
    std::vector<int> cols = {3,4,5,6};
    size_t patIdx = 0;
    addPattern16(seq, bpb, cols, 64, stepBeats, patIdx);

    // Arrange as a song with a single instance (exercises processSongArrangement path)
    seq.setPlaybackMode(PlaybackMode::Song);
    seq.clearSong();
    seq.addPatternToSong(patIdx, 0.0);

    // Observe callbacks
    int observed = 0;
    std::map<int,int> perCol;
    seq.setNoteCallbacks(
        [&](int /*pitch*/, float /*vel*/, int /*ch*/, const Envelope&){
            // Compute current column estimate from position
            double pos = seq.getPositionInBeats();
            double posMod = std::fmod(pos, (double)bpb);
            if (posMod < 0.0) posMod += (double)bpb;
            int col = (int)std::llround(posMod / stepBeats);
            if (col < 0) col = 0; if (col > 15) col = col % 16;
            observed++; perCol[col]++;
        },
        [&](int,int){}
    );

    // Run for N loops
    const int loops = 4;
    const double secPerBeat = 60.0 / 120.0;
    const int sr = 44100;
    const int bufferFrames = 512;
    const double dt = (double)bufferFrames / (double)sr;
    const double dBeats = dt / secPerBeat;
    const int totalFrames = (int)std::ceil((loops * (double)bpb) / dBeats);

    seq.setLooping(true);
    seq.start();

    for (int f=0; f<totalFrames; ++f) {
        seq.process(dt);
    }

    const int expected = loops * (int)cols.size();
    bool ok = (observed == expected);
    std::cout << (ok?"PASS":"FAIL") << " observed=" << observed << " expected=" << expected << "\n";
    return ok ? 0 : 1;
}

