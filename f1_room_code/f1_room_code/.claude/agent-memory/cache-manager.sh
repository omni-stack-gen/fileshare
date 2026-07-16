#!/bin/bash
# UI Workflow Cache Manager
# Usage: ./cache-manager.sh [command] [args]

CACHE_FILE="/home/dpower/data/KR_DOOR/.claude/agent-memory/ui-workflow-cache.json"
LAYOUT_DIR="/home/dpower/data/KR_DOOR/.claude/agent-memory/ui-layout-analyzer/layouts"
UI_DIR="/home/dpower/data/KR_DOOR/ui"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Ensure cache file exists
init_cache() {
    if [ ! -f "$CACHE_FILE" ]; then
        mkdir -p "$(dirname "$CACHE_FILE")"
        echo '{"version":"1.0","layoutCache":{},"generationCache":{}}' > "$CACHE_FILE"
        echo -e "${GREEN}初始化缓存文件${NC}"
    fi
}

# Get cache entry
get_layout_cache() {
    local node_id=$1
    cat "$CACHE_FILE" | jq -r ".layoutCache[\"$node_id\"] // empty"
}

get_generation_cache() {
    local node_id=$1
    cat "$CACHE_FILE" | jq -r ".generationCache[\"$node_id\"] // empty"
}

# Check if cache is valid (not expired)
is_cache_valid() {
    local node_id=$1
    local cache=$(get_layout_cache "$node_id")
    
    if [ -z "$cache" ]; then
        return 1  # No cache
    fi
    
    local created=$(echo "$cache" | jq -r '.createdAt')
    local now=$(date -u +"%Y-%m-%dT%H:%M:%SZ")
    
    # Check if older than 24 hours
    local created_ts=$(date -d "$created" +%s)
    local now_ts=$(date -d "$now" +%s)
    local diff=$((now_ts - created_ts))
    
    if [ $diff -gt 86400 ]; then
        return 1  # Expired
    fi
    
    # Check if report file exists
    local report_path=$(echo "$cache" | jq -r '.reportPath')
    if [ ! -f "$LAYOUT_DIR/$(basename "$report_path")" ]; then
        return 1  # File missing
    fi
    
    return 0  # Valid
}

# Update layout cache
update_layout_cache() {
    local node_id=$1
    local report_path=$2
    local content_hash=$3
    local node_count=$4
    
    local tmp=$(mktemp)
    jq --arg id "$node_id" \
       --arg path "$report_path" \
       --arg hash "$content_hash" \
       --arg count "$node_count" \
       --arg now $(date -u +"%Y-%m-%dT%H:%M:%SZ") \
       '.layoutCache[$id] = {
           "reportPath": $path,
           "contentHash": $hash,
           "nodeCount": ($count | tonumber),
           "createdAt": $now,
           "accessedAt": $now
       }' "$CACHE_FILE" > "$tmp" && mv "$tmp" "$CACHE_FILE"
}

# Update generation cache
update_generation_cache() {
    local node_id=$1
    local slint_path=$2
    local source_hash=$3
    local element_count=$4
    
    local tmp=$(mktemp)
    jq --arg id "$node_id" \
       --arg path "$slint_path" \
       --arg hash "$source_hash" \
       --arg count "$element_count" \
       --arg now $(date -u +"%Y-%m-%dT%H:%M:%SZ") \
       '.generationCache[$id] = {
           "slintPath": $path,
           "sourceHash": $hash,
           "generatedAt": $now,
           "elementCount": ($count | tonumber)
       }' "$CACHE_FILE" > "$tmp" && mv "$tmp" "$CACHE_FILE"
}

# Invalidate cache
invalidate_cache() {
    local node_id=$1
    local tmp=$(mktemp)
    jq --arg id "$node_id" 'del(.layoutCache[$id]) | del(.generationCache[$id])' "$CACHE_FILE" > "$tmp" && mv "$tmp" "$CACHE_FILE"
    echo -e "${YELLOW}已清除节点 $node_id 的缓存${NC}"
}

# Clear all cache
clear_all_cache() {
    echo '{"version":"1.0","layoutCache":{},"generationCache":{}}' > "$CACHE_FILE"
    echo -e "${GREEN}已清除所有缓存${NC}"
}

# Calculate file hash
calculate_hash() {
    local file=$1
    sha256sum "$file" | cut -d' ' -f1
}

# Show cache status
show_status() {
    echo "=== 缓存状态 ==="
    echo ""
    
    local layout_count=$(cat "$CACHE_FILE" | jq '.layoutCache | length')
    local gen_count=$(cat "$CACHE_FILE" | jq '.generationCache | length')
    
    echo -e "布局缓存: ${GREEN}$layout_count${NC} 个节点"
    echo -e "生成缓存: ${GREEN}$gen_count${NC} 个节点"
    echo ""
    
    if [ "$layout_count" -gt 0 ]; then
        echo "布局缓存列表:"
        cat "$CACHE_FILE" | jq -r '.layoutCache | to_entries[] | 
            "  - \(.key): \(.value.nodeCount) 个元素, 创建于 \(.value.createdAt)"'
        echo ""
    fi
    
    if [ "$gen_count" -gt 0 ]; then
        echo "生成缓存列表:"
        cat "$CACHE_FILE" | jq -r '.generationCache | to_entries[] | 
            "  - \(.key): \(.value.elementCount) 个元素, 生成于 \(.value.generatedAt)"'
    fi
}

# Main command handler
case "${1:-status}" in
    init)
        init_cache
        ;;
    status)
        init_cache
        show_status
        ;;
    get)
        init_cache
        get_layout_cache "$2"
        ;;
    valid)
        init_cache
        if is_cache_valid "$2"; then
            echo "valid"
        else
            echo "invalid"
        fi
        ;;
    update-layout)
        init_cache
        update_layout_cache "$2" "$3" "$4" "$5"
        echo -e "${GREEN}已更新布局缓存: $2${NC}"
        ;;
    update-gen)
        init_cache
        update_generation_cache "$2" "$3" "$4" "$5"
        echo -e "${GREEN}已更新生成缓存: $2${NC}"
        ;;
    invalidate)
        init_cache
        invalidate_cache "$2"
        ;;
    clear)
        init_cache
        clear_all_cache
        ;;
    hash)
        calculate_hash "$2"
        ;;
    *)
        echo "用法: $0 [command] [args]"
        echo ""
        echo "Commands:"
        echo "  status                    显示缓存状态"
        echo "  get <node_id>             获取布局缓存"
        echo "  valid <node_id>           检查缓存是否有效"
        echo "  update-layout <id> <path> <hash> <count>  更新布局缓存"
        echo "  update-gen <id> <path> <hash> <count>     更新生成缓存"
        echo "  invalidate <node_id>      清除指定节点缓存"
        echo "  clear                     清除所有缓存"
        echo "  hash <file>               计算文件哈希"
        ;;
esac
