#!/bin/bash

echo "Testing effects bypass and mix controls..."

# Start the application in background
./build/bin/AIMusicHardwareIntegrated &
APP_PID=$!

# Give it time to start
sleep 3

# The app is now running, we'll let it run for a bit then kill it
echo "Application started with PID $APP_PID"
echo "Please test the effects manually:"
echo "1. Select an effect from the dropdown (e.g., Distortion)"
echo "2. Adjust the mix slider"
echo "3. Toggle the bypass button"
echo ""
echo "Press Enter to stop the test..."
read

# Kill the app
kill $APP_PID 2>/dev/null
echo "Test completed"