#!/bin/bash

# Test script to verify proper shutdown behavior
echo "Testing shutdown behavior of AIMusicHardwareIntegrated..."

# Start the application in background
./build/bin/AIMusicHardwareIntegrated &
APP_PID=$!

echo "Started application with PID: $APP_PID"
echo "Waiting 3 seconds for initialization..."
sleep 3

# Send SIGTERM (graceful shutdown signal)
echo "Sending SIGTERM to trigger shutdown..."
kill -TERM $APP_PID

# Wait for the process to shut down gracefully
echo "Waiting for graceful shutdown..."
wait $APP_PID 2>/dev/null
EXIT_CODE=$?

echo "Application exited with code: $EXIT_CODE"

if [ $EXIT_CODE -eq 0 ]; then
    echo "✓ SUCCESS: Application shutdown cleanly"
else
    echo "✗ FAILURE: Application crashed or exited with error"
fi