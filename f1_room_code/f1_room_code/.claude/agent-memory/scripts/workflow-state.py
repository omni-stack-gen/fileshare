#!/usr/bin/env python3
"""
Workflow state management script for UI workflow with verification.
Updates artifact paths in the workflow state file.
"""

import sys
import json
import os
from datetime import datetime

STATE_FILE = "/home/dpower/data/KR_DOOR/.claude/agent-memory/ui-workflow-state.json"

def load_state():
    """Load current workflow state or create new one."""
    if os.path.exists(STATE_FILE):
        with open(STATE_FILE, 'r') as f:
            return json.load(f)
    return {
        "version": "1.0",
        "created": datetime.now().isoformat(),
        "artifacts": {}
    }

def save_state(state):
    """Save workflow state to file."""
    os.makedirs(os.path.dirname(STATE_FILE), exist_ok=True)
    with open(STATE_FILE, 'w') as f:
        json.dump(state, f, indent=2)

def update_artifact(artifact_type, path):
    """Update an artifact path in the state."""
    state = load_state()

    if artifact_type not in state["artifacts"]:
        state["artifacts"][artifact_type] = {}

    state["artifacts"][artifact_type]["path"] = path
    state["artifacts"][artifact_type]["updated"] = datetime.now().isoformat()

    save_state(state)
    print(f"Updated {artifact_type}: {path}")

def main():
    if len(sys.argv) < 4 or sys.argv[1] != "artifact":
        print("Usage: python workflow-state.py artifact <type> <path>")
        print("Types: layoutReport, slintFile, verificationReport")
        sys.exit(1)

    artifact_type = sys.argv[2]
    path = sys.argv[3]

    update_artifact(artifact_type, path)

if __name__ == "__main__":
    main()
