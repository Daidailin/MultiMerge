# MultiMerge 测试数据规范

> **版本**: v1.1
> **更新时间**: 2026-03-21
> **变更**: 统一使用 .txt 格式

---

## 一、目录结构

```
test_data/
├── input/                 # 测试输入数据
│   ├── README.md          # 本文件
│   ├── 01_dot_ms.txt      # 点号毫秒格式 (HH:MM:SS.fff)
│   ├── 02_dot_us.txt      # 点号微秒格式 (HH:MM:SS.ffffff)
│   ├── 03_colon_ms.txt    # 冒号毫秒格式 (HH:MM:SS:fff)
│   ├── 04_colon_us.txt    # 冒号微秒格式 (HH:MM:SS:ffffff)
│   ├── sensor1.txt        # 原始测试数据（点号格式）
│   └── sensor2.txt        # 原始测试数据（点号格式）
│
├── output/                 # 测试输出结果（自动生成）
│   └── README.md
│
└── scripts/              # 测试脚本
    └── run_format_tests.ps1
```

---

## 二、输入文件命名规范

### 格式：`{序号}_{分隔符}_{精度}.txt`

| 序号 | 分隔符 | 精度 | 格式 | 示例 |
|------|--------|------|------|------|
| 01 | dot | ms | `HH:MM:SS.fff` | `12:30:45.100` |
| 02 | dot | us | `HH:MM:SS.ffffff` | `12:30:45.100000` |
| 03 | colon | ms | `HH:MM:SS:fff` | `12:30:45:100` |
| 04 | colon | us | `HH:MM:SS:ffffff` | `12:30:45:100000` |

---

## 三、输出文件命名规范

### 格式：`test{序号}_{输入1}_{输入2}.txt`

| 测试场景 | 命名示例 | 说明 |
|---------|---------|------|
| 格式混合测试 | `test01_dotms_colonus.txt` | 验证不同格式文件合并 |
| 精度混合测试 | `test02_dotus_colonms.txt` | 验证毫秒/微秒混合 |
| 插值方法测试 | `test05_sensor_merge.txt` | 验证最近邻插值 |

---

## 四、测试数据内容规范

### 4.1 文件头格式

```
Time,Temp,Pressure,Humidity
```

### 4.2 数据行格式

```
时间戳,数值1,数值2,...
```

### 4.3 测试数据要求

- 每个文件至少 5 行数据
- 时间戳必须单调递增
- 数值使用标准浮点数格式

---

## 五、运行测试

### 使用测试脚本

```powershell
cd E:\TraeCN\TraeCN_project\test\QT_Project\MultiMerge\test_data\scripts
.\run_format_tests.ps1
```

---

## 六、测试覆盖矩阵

| 测试编号 | 输入1 | 输入2 | 验证点 |
|---------|-------|-------|--------|
| test01 | 01_dot_ms | 04_colon_us | 点号毫秒 + 冒号微秒 |
| test02 | 02_dot_us | 03_colon_ms | 点号微秒 + 冒号毫秒 |
| test03 | 03_colon_ms | 02_dot_us | 冒号毫秒 + 点号微秒 |
| test04 | 04_colon_us | 01_dot_ms | 冒号微秒 + 点号毫秒 |
| test05 | sensor1 | sensor2 | 原始数据合并 |
