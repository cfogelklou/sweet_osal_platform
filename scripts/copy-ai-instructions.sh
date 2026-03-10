#!/bin/bash

# Script to copy CLAUDE.md to GEMINI.md and .github/copilot-instructions.md
# This ensures all AI instruction files stay in sync

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

SOURCE_FILE="$PROJECT_ROOT/CLAUDE.md"
GEMINI_FILE="$PROJECT_ROOT/GEMINI.md"
COPILOT_FILE="$PROJECT_ROOT/.github/copilot-instructions.md"

if [ ! -f "$SOURCE_FILE" ]; then
    echo "Error: Source file $SOURCE_FILE does not exist"
    exit 1
fi

# Create .github directory if it doesn't exist
mkdir -p "$PROJECT_ROOT/.github"

# Copy CLAUDE.md to GEMINI.md
echo "Copying $SOURCE_FILE to $GEMINI_FILE..."
cp "$SOURCE_FILE" "$GEMINI_FILE"

# Copy CLAUDE.md to .github/copilot-instructions.md
echo "Copying $SOURCE_FILE to $COPILOT_FILE..."
cp "$SOURCE_FILE" "$COPILOT_FILE"

echo "Success: AI instruction files synchronized successfully!"
