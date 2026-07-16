# lUC51 DataBackup2 布局分析报告

- Pencil 文件: `/home/dpower/data/KR_DOOR/door_ui_2.pen`
- Node ID: `lUC51`
- Name: `DataBackup2`
- 尺寸: 800 x 1280
- 交互来源: `work.md` 中未发现 `lUC51` 或 `DataBackup2` 的专项逻辑；按用户要求实现为 DataBackup 点击后的确认弹窗。

## Visual Hierarchy Tree

- `lUC51` frame `DataBackup2` 800x1280, 背景渐变 `#1e2c4d -> #2b374a -> #221e48`
- 标题 `M3Ttx` text `数据同步备份`, x=280, y=26, font 40, Source Han Sans CN, white
- 主卡片 `BQ5N1` rectangle, x=45, y=112, w=710, h=300, fill `#314158ff`, radius 21.3224, shadow
- 行 `NiEtG`, x=74, y=112, w=652, h=100, text `备份`, right caret `WgNqr`
- 行 `2u0WM`, x=74, y=212, w=652, h=100, text `恢复`, right caret `8jva5`
- 行 `8Tepa`, x=74, y=312, w=652, h=100, text `同步`, right caret `gTGlT`
- 返回按钮 `CUQmD`, x=300, y=1138, w=200, h=98, fill `#45556cff`, text `返回`
- 遮罩 `ubg49`, x=0, y=0, w=800, h=1280, fill `#0000004f`
- 弹窗容器 `mafug`, x=141, y=516, w=517, h=249, fill `#314158ff`, radius 21.3224
- 弹窗标题 `Gz6Em`, x=232, y=15, text `备份`, font 26
- 弹窗正文 `TjC9V`, x=54, y=79, text `是否把本地数据库备份到SD卡中？`, font 26.653
- 横线 `arXyQ`, x=43, y=159, w=430, stroke `#dadadaff`
- 竖线 `H7wih`, x=258, y=169, h=64, stroke `#dadadaff`
- 弹窗按钮 `lreWQ`, x=122, y=179, text `否`, font 26
- 弹窗按钮 `NCuT7`, x=368.716, y=179, text `是`, font 26

## Color Palette

- 页面背景渐变: `#1e2c4dff`, `#2b374aff`, `#221e48ff`
- 卡片/弹窗: `#314158ff`
- 遮罩: `#0000004f`
- 返回按钮: `#45556cff`
- 分割线: `#dadadaff`
- 文本: `#ffffffff`

## Typography

- 标题: Source Han Sans CN, 40, weight 500
- 列表/弹窗按钮: Source Han Sans CN, 26, normal
- 弹窗正文: Source Han Sans CN, 26.653, normal
- 返回: Inter, 29.4, weight 600

## 生成说明

- 弹窗无新图标节点；沿用现有 DataBackup 行图标资源。
- `work.md` 未发现 `lUC51` 对应交互逻辑，按用户指定将 DataBackup 列表点击实现为确认弹窗。
