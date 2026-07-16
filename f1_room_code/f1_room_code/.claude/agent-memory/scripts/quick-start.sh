#!/bin/bash
# 快速开始工作流的辅助脚本

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

function show_help() {
    echo "Slint UI 工作流 - 快速开始"
    echo ""
    echo "用法: $0 [命令] [参数]"
    echo ""
    echo "命令:"
    echo "  status              查看当前状态"
    echo "  start <node_id>     开始处理新节点"
    echo "  continue            继续当前进行中的任务"
    echo "  batch <plan>        批量处理 (plan1|plan2|plan3|plan4)"
    echo "  reset               重置状态（谨慎使用）"
    echo ""
    echo "示例:"
    echo "  $0 status"
    echo "  $0 start TQQlc"
    echo "  $0 batch plan1"
}

function check_python() {
    if ! command -v python3 &> /dev/null; then
        echo "错误: 需要 Python 3"
        exit 1
    fi
}

function show_status() {
    cd "$PROJECT_ROOT"
    python3 .claude/agent-memory/scripts/workflow-state.py status
}

function start_node() {
    local node_id=$1
    if [ -z "$node_id" ]; then
        echo "错误: 请指定 Node ID"
        echo "用法: $0 start <node_id>"
        exit 1
    fi
    
    cd "$PROJECT_ROOT"
    
    # 检查是否已有进行中的任务
    current_task=$(python3 .claude/agent-memory/scripts/workflow-state.py status 2>/dev/null | grep "当前任务" || true)
    if [ -n "$current_task" ] && [[ ! "$current_task" =~ "没有进行中的任务" ]]; then
        echo "警告: 已有进行中的任务"
        read -p "是否继续并开始新任务? (y/N) " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            echo "已取消"
            exit 0
        fi
    fi
    
    echo "🚀 开始处理节点: $node_id"
    python3 .claude/agent-memory/scripts/workflow-state.py start "$node_id" --phase analysis
    echo ""
    echo "✅ 任务已启动"
    echo "下一步: 运行 ui-layout-analyzer 智能体分析布局"
}

function batch_process() {
    local plan=$1
    
    # Plan 对应的 Node IDs
    declare -A PLAN_NODES
    PLAN_NODES[plan1]="Rzj87 k016Y KW3lm K7Jgw"
    PLAN_NODES[plan2]="TQQlc 8TvZW BTy4s mTdn6"
    PLAN_NODES[plan3]="2jMG5 WKF8g r93l7 dyulI gdp75"
    PLAN_NODES[plan4]="4glnf APrBb cWB4g"
    
    if [ -z "${PLAN_NODES[$plan]}" ]; then
        echo "错误: 未知的 Plan: $plan"
        echo "可用的 Plan: plan1, plan2, plan3, plan4"
        exit 1
    fi
    
    echo "📦 批量处理: $plan"
    echo "包含节点: ${PLAN_NODES[$plan]}"
    echo ""
    
    cd "$PROJECT_ROOT"
    python3 .claude/agent-memory/scripts/workflow-state.py queue-batch ${PLAN_NODES[$plan]} --priority high
    
    echo ""
    echo "✅ 已加入队列"
    echo "使用 'workflow-state.py next' 取出下一个任务"
}

function reset_state() {
    echo "⚠️  警告: 这将清除所有状态和历史记录！"
    read -p "确定要继续吗? 输入 'RESET' 确认: " confirm
    
    if [ "$confirm" = "RESET" ]; then
        cd "$PROJECT_ROOT"
        cat > .claude/agent-memory/workflow-state.json << 'EOF'
{
  "version": "1.0.0",
  "currentTask": null,
  "history": [],
  "queue": [],
  "stats": {
    "totalProcessed": 0,
    "successCount": 0,
    "failCount": 0
  }
}
EOF
        echo "✅ 状态已重置"
    else
        echo "已取消"
    fi
}

# 主逻辑
check_python

case "${1:-status}" in
    status)
        show_status
        ;;
    start)
        start_node "$2"
        ;;
    batch)
        batch_process "$2"
        ;;
    continue)
        show_status
        echo ""
        echo "请手动继续当前任务..."
        ;;
    reset)
        reset_state
        ;;
    help|--help|-h)
        show_help
        ;;
    *)
        echo "未知命令: $1"
        show_help
        exit 1
        ;;
esac
