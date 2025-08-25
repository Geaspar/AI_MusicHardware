#pragma once
#include <string>
#include <vector>
#include <memory>

namespace AIMusicHardware {

struct SegmentExitPoint { double beat; std::string name; };
struct SegmentEntryPoint { double beat; std::string name; };

struct MusicSegment {
    std::string name;
    double lengthBeats = 0.0;
    std::vector<SegmentExitPoint> exits;
    std::vector<SegmentEntryPoint> entries;
};

struct SegmentTransition {
    enum class Type { Immediate, NextBeat, NextBar, ExitPoint };
    std::string from;
    std::string to;
    Type type = Type::Immediate;
    std::string exitPoint; // if type == ExitPoint
    std::string entryPoint; // optional
    int priority = 0;
    float probability = 1.0f; // 0..1
};

// Minimal skeleton; integration planned for Phase B
class SegmentSequencer {
public:
    void addSegment(const MusicSegment& s) { segments_.push_back(s); }
    void clearSegments() { segments_.clear(); }
    const std::vector<MusicSegment>& getSegments() const { return segments_; }

    void addTransition(const SegmentTransition& t) { transitions_.push_back(t); }
    void clearTransitions() { transitions_.clear(); }
    const std::vector<SegmentTransition>& getTransitions() const { return transitions_; }

private:
    std::vector<MusicSegment> segments_;
    std::vector<SegmentTransition> transitions_;
};

} // namespace AIMusicHardware
