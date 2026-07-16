# UI Layout Analyzer Agent Memory

## Agent Purpose
Capture UI layouts using the Pencil MCP tool and output structured analysis reports.

## Standard Operating Procedure (SOP)

1. **Read Target Node**: Use `pencil` MCP tools to capture node data
2. **Capture Layout**: Use `snapshot_layout` with maxDepth=5 for coordinates
3. **Extract Properties**: Use `batch_get` with readDepth=4 for node properties
4. **Build Hierarchy**: Construct Visual Hierarchy Tree with coordinates
5. **Validate**: Run `print_hierarchy.py` to validate tree matches raw JSON
6. **Save Report**: Output to `.claude/agent-memory/ui-layout-analyzer/layouts/[NodeID]-[timestamp].md`

## Output Format

### Report Structure
1. Overview (resolution, total elements, layout type)
2. Visual Hierarchy Tree (with coordinates)
3. Complete Element Coordinates Table
4. Layout Observations

### Tree Format
```
NodeID Name (type, WxH) @ (x, y)
├── ChildID Name (type, WxH) @ (x, y)
└── ChildID Name (type, WxH) @ (x, y)
```

### Table Format
| # | ID | Name | Type | X (rel) | Y (rel) | W | H | Text |

## Project Context
This agent is part of a Slint UI workflow for analyzing door/entry system UIs.
