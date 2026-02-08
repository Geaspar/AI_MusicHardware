#include <iostream>
#include <vector>
#include <map>
#include <cmath>
#include <string>
#include "../include/sequencer/Sequencer.h"

using namespace AIMusicHardware;

struct RunStats {
    int frames = 0;
    int loopsCompleted = 0;
    std::map<int,int> expectedPerColumn; // column -> hits per loop
    std::map<int,int> observedPerColumn;
    int totalExpected = 0;
    int totalObserved = 0;
    int unexpectedObserved = 0;
    int missedExpected = 0;
};

static void makePattern(Sequencer& seq, int bpb, const std::vector<int>& cols16, int pitch, double durBeats) {
    auto p = std::make_unique<Pattern>("test");
    for (int c : cols16) {
        double start = (double)bpb/16.0 * (double)c;
        p->addNote(Note(pitch, 1.0f, start, durBeats, 0));
    }
    p->setLength((double)bpb); // 1 bar
    seq.addPattern(std::move(p));
    // Select the newly added test pattern (last index)
    size_t last = std::max<size_t>(0, seq.getNumPatterns() > 0 ? seq.getNumPatterns() - 1 : 0);
    seq.setCurrentPattern(last);
}

static bool runTest(const char* name,
                    const std::vector<int>& cols16,
                    int loops,
                    int tempo,
                    RunStats& out) {
    std::cout << "\n[Test] " << name << "\n";
    Sequencer seq((double)tempo, 4);
    if (!seq.initialize()) { std::cerr << "Sequencer init failed\n"; return false; }
    int bpb = seq.getBeatsPerBar();
    makePattern(seq, bpb, cols16, 60, (double)bpb/16.0); // C4, 16th duration
    out.expectedPerColumn.clear(); out.observedPerColumn.clear();
    for (int c : cols16) out.expectedPerColumn[c] = 1; // once per loop
    out.totalExpected = loops * (int)cols16.size();

    // Observed events
    std::vector<std::pair<int,double>> observed; // (column, timeBeats)
    double currentBeats = 0.0;
    const double sr = 44100.0; const int buf = 512; const double dt = (double)buf/sr; // ~11.6ms
    const double secPerBeat = 60.0 / (double)tempo;
    const double dBeats = dt / secPerBeat;
    const double stepBeats = (double)bpb/16.0;

    seq.setNoteCallbacks(
        [&](int pitch, float velocity, int channel, const Envelope&){
            // Map current position to nearest 16th column for logging
            int col = (int)std::llround(std::fmod(currentBeats, (double)bpb) / stepBeats);
            if (col < 0) col = 0; if (col > 15) col = col % 16;
            observed.emplace_back(col, currentBeats);
            out.totalObserved++;
            out.observedPerColumn[col]++;
        },
        [&](int,int){ }
    );

    seq.setLooping(true);
    seq.start();

    int totalFrames = (int)std::ceil((loops * (double)bpb) / dBeats);
    int loopsSeen = 0;
    for (int f=0; f<totalFrames; ++f) {
        seq.process(dt);
        currentBeats += dBeats;
        // Count loops by wrap-around modulo bar
        double posMod = std::fmod(currentBeats, (double)bpb);
        if (posMod < dBeats) loopsSeen++;
        out.frames++;
    }
    out.loopsCompleted = loopsSeen;

    // Analyze unexpected columns
    for (auto& kv : out.observedPerColumn) {
        int col = kv.first;
        if (out.expectedPerColumn.find(col) == out.expectedPerColumn.end()) {
            out.unexpectedObserved += kv.second; // ghost
        }
    }
    // Analyze missed expected
    for (auto& kv : out.expectedPerColumn) {
        int col = kv.first;
        int expectedHits = kv.second * loops;
        int got = out.observedPerColumn[col];
        if (got < expectedHits) out.missedExpected += (expectedHits - got);
    }

    // Report
    std::cout << "Loops completed: " << out.loopsCompleted << " (target " << loops << ")\n";
    std::cout << "Observed total note-ons: " << out.totalObserved << "/ expected " << out.totalExpected << "\n";
    if (out.unexpectedObserved > 0) std::cout << "Ghost notes: " << out.unexpectedObserved << "\n";
    if (out.missedExpected > 0) std::cout << "Missed retriggers: " << out.missedExpected << "\n";
    std::cout << "Per-column observed:";
    for (int c=0;c<16;++c) std::cout << " " << out.observedPerColumn[c];
    std::cout << "\n";

    bool pass = (out.unexpectedObserved == 0) && (out.missedExpected == 0) && (out.loopsCompleted >= loops-1);
    std::cout << (pass?"PASS":"FAIL") << "\n";
    return pass;
}

int main(int argc, char** argv) {
    int loops = 8; int tempo = 120;
    // Test A: C4 at columns 1,6,10,12
    RunStats a; runTest("Pattern C4 at [1,6,10,12]", {1,6,10,12}, loops, tempo, a);
    // Test B: consecutive 3,4,5,6
    RunStats b; runTest("Consecutive [3,4,5,6]", {3,4,5,6}, loops, tempo, b);
    // Exit nonzero if either failed
    bool ok = (a.unexpectedObserved==0 && a.missedExpected==0) && (b.unexpectedObserved==0 && b.missedExpected==0);
    return ok ? 0 : 1;
}
