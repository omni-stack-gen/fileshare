# Slint UI 生成完整参考

> 本文档供智能体按需读取，不作为系统提示词加载。

## 目录
1. [颜色系统](#颜色系统)
2. [布局系统](#布局系统)
3. [Icon 导出协议](#icon-导出协议)
4. [常见问题](#常见问题)

---

## 颜色系统

### 格式规范
- HEX + Alpha: `#RRGGBBAA`
- 渐变: `@linear-gradient(角度deg, #开始 0%, #结束 100%)`

### 项目常用色
| 名称 | HEX | 用途 |
|-----|-----|-----|
| Dark Blue | #0f172bff | 主背景 |
| Medium Blue | #1d293dff | 容器背景 |
| Light Gray | #cad5e2ff | 次要文字 |
| White | #ffffffff | 主文字 |
| Purple | #9810faff | 渐变开始 |
| Dark Purple | #8200dbff | 渐变结束 |

---

## 布局系统

### 定位模式

**绝对定位**（默认）
```slint
Rectangle {
    x: 32px;  // 相对父元素
    y: 24px;
    width: 100px;
    height: 50px;
}
```

**居中计算**
```slint
// 水平居中
x: (parent.width - self.width) / 2

// 垂直居中  
y: (parent.height - self.height) / 2

// 双轴居中
x: (parent.width - self.width) / 2
y: (parent.height - self.height) / 2
```

**垂直布局**
```slint
VerticalLayout {
    spacing: 20px;
    alignment: center;
    
    Rectangle { height: 50px; }
    Rectangle { height: 50px; }
}
```

### 阴影系统
```slint
// 单层阴影
drop-shadow-blur: 4px;
drop-shadow-color: #0000001a;
drop-shadow-offset-y: 2px;

// 多层阴影（需要时）
// 注：Slint 只支持单层，取最明显的一层
```

---

## Icon 导出协议

### 步骤
1. 识别所有 Name=Icon 的节点
2. 使用 `pencil export_nodes`:
   ```json
   {
     "nodes": ["nodeID1", "nodeID2"],
     "format": "png",
     "scale": 1
   }
   ```
3. 保存到 `assets/images/[nodeID].png`
4. 引用: `@image-url("../assets/images/[nodeID].png")`

### 禁止行为
- ❌ `text: "🎤"` - emoji 占位符
- ❌ `Rectangle { background: #xxx }` - 色块替代
- ❌ `Path { ... }` - 手写路径重绘

---

## 常见问题

### Q: 文本被裁剪
**原因**: 容器高度不够或 Text 无足够空间
**解决**: 
```slint
// 给 Text 明确的尺寸
Text {
    width: parent.width;
    height: 30px;
    vertical-alignment: center;
}
```

### Q: 图片不显示
**原因**: 路径错误
**解决**: 使用相对 `ui/` 目录的路径
```slint
// ui/ 文件中的正确写法
source: @image-url("../assets/images/icon.png")
```

### Q: 渐变方向不对
**原因**: 角度计算方式不同
**解决**: 
- 0deg = 从左到右
- 90deg = 从上到下  
- 180deg = 从右到左
- 270deg/-90deg = 从下到上
- 对角线: 45deg, -45deg, 135deg, -135deg

### Q: VerticalLayout 中的子元素不居中
**原因**: `alignment: center` 只控制主轴
**解决**: 子元素使用显式 x 偏移
```slint
VerticalLayout {
    Rectangle {
        x: (parent.width - self.width) / 2; // 手动居中
        width: 100px;
    }
}
```

---

## 编译检查

```bash
# 语法检查
slint-compiler --check ui/file.slint

# 完整编译（不输出）
slint-compiler -o /dev/null ui/file.slint

# 查看错误
slint-compiler ui/file.slint 2>&1
```

---

## 模板速查

### 弹窗
```slint
export component Popup inherits Window {
    width: 430px;
    height: 303px;
    
    Rectangle {
        width: 100%;
        height: 100%;
        background: #1d293dff;
        border-radius: 16px;
        drop-shadow-blur: 5px;
        drop-shadow-color: #0000001a;
    }
}
```

### 按钮
```slint
Rectangle {
    width: 180px;
    height: 56px;
    background: @linear-gradient(-135deg, #9810fa 0%, #8200db 100%);
    border-radius: 14px;
    
    Text {
        text: "按钮文字";
        color: #ffffffff;
        font-size: 16px;
        font-weight: 600;
        x: (parent.width - self.preferred-width) / 2;
        y: (parent.height - self.preferred-height) / 2;
    }
    
    TouchArea { width: 100%; height: 100%; }
}
```

### 带图标的按钮
```slint
Rectangle {
    width: 384px;
    height: 56px;
    background: #314158;
    border-radius: 14px;
    
    Image {
        x: 164px;
        y: (parent.height - 20px) / 2;
        width: 20px;
        height: 20px;
        source: @image-url("../assets/images/close.png");
    }
    
    Text {
        x: 191px;
        text: "취소";
        color: #ffffffff;
        font-size: 16px;
        font-weight: 600;
    }
    
    TouchArea { width: 100%; height: 100%; }
}
```
