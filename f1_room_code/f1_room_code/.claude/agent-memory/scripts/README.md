# 工作流状态管理脚本

## 简介

`workflow-state.py` 用于管理 Slint UI 工作流的状态追踪，支持断点续作、进度追踪和统计报表。

## 使用方法

### 1. 查看当前状态

```bash
python3 .claude/agent-memory/scripts/workflow-state.py status
```

### 2. 开始新任务

```bash
python3 .claude/agent-memory/scripts/workflow-state.py start TQQlc --phase analysis
```

### 3. 更新阶段

```bash
python3 .claude/agent-memory/scripts/workflow-state.py phase generation
```

### 4. 添加产物

```bash
python3 .claude/agent-memory/scripts/workflow-state.py artifact layoutReport layouts/TQQlc-20260408-120000.md
```

### 5. 记录错误

```bash
python3 .claude/agent-memory/scripts/workflow-state.py error "Pencil MCP连接超时"
```

### 6. 完成任务

```bash
# 标记为成功
python3 .claude/agent-memory/scripts/workflow-state.py complete --success

# 标记为失败
python3 .claude/agent-memory/scripts/workflow-state.py complete --fail
```

### 7. 队列管理

```bash
# 添加单个任务到队列
python3 .claude/agent-memory/scripts/workflow-state.py queue Rzj87 --priority high

# 批量添加
python3 .claude/agent-memory/scripts/workflow-state.py queue-batch TQQlc Rzj87 k016Y --priority medium

# 取出下一个任务
python3 .claude/agent-memory/scripts/workflow-state.py next
```

### 8. 查看历史

```bash
# 查看所有历史
python3 .claude/agent-memory/scripts/workflow-state.py history

# 查看特定节点历史
python3 .claude/agent-memory/scripts/workflow-state.py history --node TQQlc
```

## 状态文件结构

状态存储在 `.claude/agent-memory/workflow-state.json`：

```json
{
  "version": "1.0.0",
  "currentTask": {
    "nodeId": "TQQlc",
    "phase": "generation",
    "status": "in_progress",
    "artifacts": {
      "layoutReport": "layouts/TQQlc-xxx.md"
    }
  },
  "queue": [...],
  "history": [...],
  "stats": {...}
}
```

## Python API

```python
from workflow_state import WorkflowState

state = WorkflowState()

# 开始任务
state.start_task("TQQlc", phase="analysis")

# 更新阶段
state.update_phase("generation")

# 添加产物
state.add_artifact("slintFile", "ui/home.slint")

# 完成任务
state.complete_task(success=True)
```
