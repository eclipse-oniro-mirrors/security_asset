# DB Operator 模块 Context

> 文档版本：v1.0
> 更新时间：2026-10-13
> 来源：`services/asset/db_operator/`

## 定位

DB Operator 负责关键资产密文的 SQLite 数据库存储管理，包含数据库打开/关闭、查询、批量处理、事务管理、文件升级和表结构定义。

## 快速路由

### 源码入口

| 文件 | 说明 |
|------|------|
| `services/asset/db_operator/src/lib.rs` | 模块声明 |
| `services/asset/db_operator/src/database.rs` | `Database` 结构体（preload_db、get_path、open/close/execute_query） |
| `services/asset/db_operator/src/database_file_upgrade.rs` | DB 文件升级（check_and_split_db、construct_splited_db_name） |
| `services/asset/db_operator/src/database_util.rs` | DB 工具函数 |
| `services/asset/db_operator/src/table.rs` | 表结构定义 |
| `services/asset/db_operator/src/statement.rs` | 预编译语句包装 |
| `services/asset/db_operator/src/transaction.rs` | 事务管理 |
| `services/asset/db_operator/src/types.rs` | QueryOptions、列类型 |
| `services/asset/db_operator/src/common.rs` | 公共 DB 辅助 |
| `services/asset/db_operator/src/process_batch_data.rs` | 批量数据处理 |
| `services/asset/db_operator/src/sqlite3_wrapper.c` | SQLite3 C API 直接调用 |
| `services/asset/db_operator/BUILD.gn` | 构建配置 |

### 数据流

```
operation_*.rs → Database.execute_query
  → sqlite3_wrapper.c → sqlite3 C API → 密文存储/读取
  → DB Key (db_key_operator.rs) 加密数据库
```

### Database 核心方法

| 方法 | 说明 |
|------|------|
| `preload_db` | 预载数据库映射 |
| `get_path` | 获取数据库路径 |
| `clear_db_map` | 清除数据库映射 |
| `sqlite_initialize` | SQLite 初始化 |
| `open` / `close` | 打开/关闭数据库 |
| `execute_query` | 执行查询 |

### DB 文件升级

| 函数 | 说明 |
|------|------|
| `check_and_split_db` | 检查并拆分数据库文件 |
| `construct_splited_db_name` | 构造拆分后数据库名 |
| `get_file_content` | 获取文件内容 |

### 测试入口

| 测试 | 路径 |
|------|------|
| Database 测试 | `services/asset/db_operator/src/test/test_database.rs` |
| Table 测试 | `services/asset/db_operator/src/test/test_table.rs` |

## 开发要点

- 表结构定义在 table.rs，修改字段需同步更新 statement.rs 的绑定逻辑
- 批量操作通过 process_batch_data.rs 处理，注意事务回滚
- DB 文件升级通过 database_file_upgrade.rs，新增版本需补充升级逻辑

## 常见开发场景

- 新增数据库字段时，在此查找 table.rs 表定义和 statement.rs 绑定逻辑
- 排查批量操作失败时，定位 process_batch_data.rs 的事务处理
- 修改数据库文件格式时，确认 database_file_upgrade.rs 的升级路径

## 相关主题

- [Crypto Manager 模块](./crypto-manager.md)
- [Core Service 模块](./core-service.md)
- [Asset 服务架构](../architecture/asset-service.md)
