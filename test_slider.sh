#!/bin/bash
echo "Testing filter sliders..."
echo "1. Try moving the CUTOFF slider up and down"
echo "2. Watch the console output to see which callbacks are triggered"
echo "3. Note if RESONANCE SLIDER messages appear when moving cutoff"
echo ""
./bin/AIMusicHardwareIntegrated 2>&1 | grep -E "(CUTOFF|RESONANCE|Filter visualizer)"