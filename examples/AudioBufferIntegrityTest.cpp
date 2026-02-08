// Headless test to validate that a block-based render contains only expected
// note-on events (represented as impulses) for given patterns and buffer sizes.
#include <iostream>
#include <vector>
#include <map>
#include <cmath>
#include <string>
#include "../include/sequencer/Sequencer.h"

using namespace AIMusicHardware;

struct IntegrityResult {
    int loopsCompleted = 0;
    int totalExpected = 0;
    int totalObserved = 0;
    int unexpectedObserved = 0;
    int missedExpected = 0;
};

static void addPattern16(Sequencer& seq, int bpb, const std::vector<int>& cols16, int pitch, double stepDurBeats) {
    auto p = std::make_unique<Pattern>("buf_test");
    for (int c : cols16) {
        double start = (double)bpb/16.0 * (double)c;
        p->addNote(Note(pitch, 1.0f, start, stepDurBeats, 0));
    }
    p->setLength((double)bpb);
    seq.addPattern(std::move(p));
    seq.setCurrentPattern(seq.getNumPatterns() - 1);
}

static IntegrityResult runIntegrity(const char* name,
                                    const std::vector<int>& cols16,
                                    int loops,
                                    int tempo,
                                    int sr,
                                    int bufferFrames) {
    std::cout << "\n[Integrity] " << name << "  tempo=" << tempo
              << "  sr=" << sr << "  buf=" << bufferFrames << "\n";
    IntegrityResult out{};
    Sequencer seq((double)tempo, 4);
    if (!seq.initialize()) { std::cerr << "Sequencer init failed\n"; return out; }
    int bpb = seq.getBeatsPerBar();
    addPattern16(seq, bpb, cols16, 60, (double)bpb/16.0);

    std::map<int,int> expectedPerCol; for (int c : cols16) expectedPerCol[c] = 1;
    std::map<int,int> observedPerCol; // col -> hits per loop
    out.totalExpected = loops * (int)cols16.size();

    // Simulated block render
    const double secPerBeat = 60.0 / (double)tempo;
    const double dt = (double)bufferFrames / (double)sr;
    const double dBeats = dt / secPerBeat;
    const double stepBeats = (double)bpb / 16.0;
    double currentBeats = 0.0;

    // Collect note-ons as impulses (no actual audio written here)
    seq.setNoteCallbacks(
        [&](int /*pitch*/, float /*vel*/, int /*ch*/, const Envelope& /*env*/){
            // Approximate current column at time of callback using running time
            int col = (int)std::llround(std::fmod(currentBeats, (double)bpb) / stepBeats);
            if (col < 0) col = 0; if (col > 15) col = col % 16;
            out.totalObserved++;
            observedPerCol[col]++;
        },
        [&](int, int){}
    );

    seq.setLooping(true);
    seq.start();

    const int totalFrames = (int)std::ceil((loops * (double)bpb) / dBeats);
    int loopsSeen = 0;
    for (int f = 0; f < totalFrames; ++f) {
        seq.process(dt);
        currentBeats += dBeats;
        double posMod = std::fmod(currentBeats, (double)bpb);
        if (posMod < dBeats) loopsSeen++;
    }
    out.loopsCompleted = loopsSeen;

    // Count ghost columns
    for (const auto& kv : observedPerCol) {
        if (expectedPerCol.find(kv.first) == expectedPerCol.end()) out.unexpectedObserved += kv.second;
    }
    // Count missed expected
    for (const auto& kv : expectedPerCol) {
        int expectedHits = kv.second * loops;
        int got = observedPerCol[kv.first];
        if (got < expectedHits) out.missedExpected += (expectedHits - got);
    }

    // Report breakdown
    std::cout << "Loops completed: " << out.loopsCompleted << " (target=" << loops << ")\n";
    std::cout << "Observed total: " << out.totalObserved << " / expected: " << out.totalExpected << "\n";
    if (out.unexpectedObserved > 0) std::cout << "Ghost notes: " << out.unexpectedObserved << "\n";
    if (out.missedExpected > 0) std::cout << "Missed: " << out.missedExpected << "\n";
    std::cout << "Per-col observed:";
    for (int c = 0; c < 16; ++c) std::cout << ' ' << observedPerCol[c];
    std::cout << "\n";
    return out;
}

int main(int argc, char** argv) {
    struct Scenario { const char* name; std::vector<int> cols; } cases[] = {
        {"Spaced [1,6,10,12]", {1,6,10,12}},
        {"Consecutive [3,4,5,6]", {3,4,5,6}},
    };
    int tempos[] = {120};
    int srs[] = {44100};
    int bufs[] = {128, 256, 512, 1024};

    bool allOk = true;
    for (auto& sc : cases) {
        for (int tempo : tempos) {
            for (int sr : srs) {
                for (int buf : bufs) {
                    auto r = runIntegrity(sc.name, sc.cols, /*loops*/4, tempo, sr, buf);
                    bool ok = (r.unexpectedObserved == 0 && r.missedExpected == 0 && r.loopsCompleted >= 3);
                    std::cout << (ok?"PASS":"FAIL") << "\n";
                    if (!ok) allOk = false;
                }
            }
        }
    }
    return allOk ? 0 : 1;
}

