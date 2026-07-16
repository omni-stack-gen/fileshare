#!/bin/bash
# 批量处理辅助脚本 - 简化批量操作的快捷命令

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BATCH_SCRIPT="$PROJECT_ROOT/scripts/batch-processor.py"

function show_help() {
    cat << EOF
批量处理辅助工具

用法: $0 [命令] [参数]

命令:
    plan1 [options]      处理 Plan 1 (房号呼叫)
    plan2 [options]      处理 Plan 2 (二维码开门)
    plan3 [options]      处理 Plan 3 (密码开门)
    plan4 [options]      处理 Plan 4 (人脸识别)
    all [options]        处理所有 Plan
    queue [options]      处理队列中的任务
    nodes N1 N2 ...      处理指定节点
    status               显示批量处理状态

选项:
    -p, --parallel N     设置并行数 (默认: 1)
    -d, --dry-run        试运行模式
    -f, --force          强制重新处理（不跳过已完成）
    -v, --verbose        详细输出

示例:
    $0 plan1                              # 串行处理 Plan 1
    $0 plan1 -p 2                         # 2个并行处理 Plan 1
    $0 plan2 --dry-run                    # 试运行 Plan 2
    $0 nodes TQQlc Rzj87                  # 处理指定节点
    $0 queue -p 3                         # 并行处理队列
    $0 all -p 2 --force                   # 强制重新处理所有 Plan

EOF
}

function check_python() {
    if ! command -v python3 &> /dev/null; then
        echo "错误: 需要 Python 3"
        exit 1
    fi
}

function run_batch() {
    local args=("$@")
    cd "$PROJECT_ROOT"
    python3 "$BATCH_SCRIPT" "${args[@]}"
}

function process_plan() {
    local plan=$1
    shift
    local extra_args=("$@")
    
    echo "📦 批量处理: $plan"
    echo "================================"
    run_batch --plan "$plan" "${extra_args[@]}"
}

function process_all() {
    local extra_args=("$@")
    
    echo "🚀 处理所有 Plan"
    echo "================================"
    
    for plan in plan1 plan2 plan3 plan4; do
        echo ""
        process_plan "$plan" "${extra_args[@]}"
    done
    
    echo ""
    echo "================================"
    echo "✅ 所有 Plan 处理完成"
}

function show_status() {
    echo "📊 批量处理状态"
    echo "================================"
    
    # 显示队列状态
    cd "$PROJECT_ROOT"
    python3 .claude/agent-memory/scripts/workflow-state.py status
    
    echo ""
    echo "Plan 节点分布:"
    echo "  Plan 1 (房号呼叫): Rzj87, k016Y, KW3lm, K7Jgw"
    echo "  Plan 2 (二维码): TQQlc, 8TvZW, BTy4s, mTdn6"
    echo "  Plan 3 (密码): 2jMG5, WKF8g, r93l7, dyulI, gdp75"
    echo "  Plan 4 (人脸): 4glnf, APrBb, cWB4g"
}

# 解析全局选项
PARALLEL=""
DRY_RUN=""
FORCE=""
VERBOSE=""

parse_options() {
    local args=()
    while [[ $# -gt 0 ]]; do
        case $1 in
            -p|--parallel)
                PARALLEL="--parallel $2"
                shift 2
                ;;
            -d|--dry-run)
                DRY_RUN="--dry-run"
                shift
                ;;
            -f|--force)
                FORCE="--no-skip"
                shift
                ;;
            -v|--verbose)
                VERBOSE="--verbose"
                shift
                ;;
            *)
                args+=("$1")
                shift
                ;;
        esac
    done
    echo "${args[@]}"
}

# 主逻辑
check_python

# 获取命令
COMMAND="${1:-help}"
shift || true

# 解析剩余选项
REMAINING=$(parse_options "$@")
EXTRA_ARGS="$PARALLEL $DRY_RUN $FORCE $VERBOSE"

case "$COMMAND" in
    plan1|plan2|plan3|plan4)
        process_plan "$COMMAND" $EXTRA_ARGS
        ;;
    all)
        process_all $EXTRA_ARGS
        ;;
    queue)
        echo "📋 处理队列任务"
        run_batch --from-queue $EXTRA_ARGS
        ;;
    nodes)
        if [ -z "$REMAINING" ]; then
            echo "错误: 需要指定节点"
            echo "用法: $0 nodes NODE1 NODE2 ..."
            exit 1
        fi
        echo "🎯 处理指定节点: $REMAINING"
        run_batch --nodes $REMAINING $EXTRA_ARGS
        ;;
    status)
        show_status
        ;;
    help|--help|-h)
        show_help
        ;;
    *)
        echo "未知命令: $COMMAND"
        show_help
        exit 1
        ;;
esac
