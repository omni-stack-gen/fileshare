#!/usr/bin/env python3
"""Print ID hierarchy from Pencil JSON export."""
import json
import sys
from pathlib import Path


def print_tree(node, indent=0, is_last=True, prefix=""):
    """Recursively print node hierarchy."""
    id_ = node.get("id", "N/A")
    name = node.get("name", "")
    type_ = node.get("type", "")
    layout = node.get("layout", "")
    x = node.get("x", 0)
    y = node.get("y", 0)
    w = node.get("width", "?")
    h = node.get("height", "?")

    branch = "└── " if is_last else "├── "
    display_name = f"{name}" if name else "(unnamed)"
    layout_info = f" [{layout}]" if layout and layout != "none" else ""

    print(f"{prefix}{branch}{id_} {display_name} ({type_}) @ ({x}, {y}) size={w}x{h}{layout_info}")

    children = node.get("children", [])
    if isinstance(children, list):
        for i, child in enumerate(children):
            last = i == len(children) - 1
            extension = "    " if is_last else "│   "
            print_tree(child, indent + 1, last, prefix + extension)


def find_node(node, target_id):
    """Recursively find a node by ID."""
    if node.get("id") == target_id:
        return node
    for child in node.get("children", []):
        result = find_node(child, target_id)
        if result:
            return result
    return None


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else "1.json"
    target_id = sys.argv[2] if len(sys.argv) > 2 else None
    data = json.loads(Path(path).read_text())

    # Handle both bare object and array-wrapped JSON
    roots = data if isinstance(data, list) else [data]

    if target_id:
        # Find the target node and print only its children
        target_node = None
        for root in roots:
            target_node = find_node(root, target_id)
            if target_node:
                break

        if target_node is None:
            print(f"Node with ID '{target_id}' not found.", file=sys.stderr)
            sys.exit(1)

        # Print the target node itself first
        print_tree(target_node, is_last=True)
    else:
        for root in roots:
            print_tree(root, is_last=True)


if __name__ == "__main__":
    main()
