/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//! This module implements Asset usage statistics collection.

use std::fs;
use std::path::Path;

use asset_crypto_manager::db_key_operator::get_db_key;
use asset_db_operator::database::Database;
use asset_db_operator::types::column;
use asset_db_operator::types::DbMap;
use asset_definition::{macros_lib, ErrCode, Extension, Result};
use asset_file_operator::ce_operator::is_db_key_cipher_file_exist;
use asset_file_operator::common::{BACKUP_SUFFIX, CE_ROOT_PATH, DB_SUFFIX, DE_ROOT_PATH};
use asset_log::{loge, logi};

use crate::common_event::get_first_unlock_userids;
use crate::sys_event::upload_usage_statistics;

const THRESHOLD_LARGE_OWNER: u32 = 100;
const THRESHOLD_LARGE_DB_SIZE_KB: u64 = 500;
const MAX_LARGE_OWNERS: usize = 300;
const BYTES_PER_KB: u64 = 1024;

#[derive(Debug, Clone)]
pub struct OwnerStats {
    pub db_name: String,
    pub total_count: u32,
    pub persistent_count: u32,
    pub db_size_kb: u64,
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub enum DbType {
    De,
    Ce,
}

const USAGE_STATISTICS_DE: &str = "asset_usage_statistics_de";
const USAGE_STATISTICS_CE: &str = "asset_usage_statistics_ce";

#[derive(Debug, Clone, Default)]
pub struct DataStats {
    pub owner_count: u32,
    pub max_count: u32,
    pub max_owner: String,
    pub avg_count: u32,
    pub total_count: u32,
}

impl DataStats {
    pub fn calculate(counts: &[u32], owner_names: &[&String]) -> Self {
        let owners_with_data: Vec<_> = counts.iter().enumerate().filter(|(_, &c)| c > 0).collect();

        let owner_count = owners_with_data.len() as u32;
        let total_count: u32 = counts.iter().sum();

        let (max_count, max_owner) = owners_with_data
            .iter()
            .max_by_key(|(_, &c)| c)
            .map(|(i, &c)| {
                let name = owner_names.get(*i).cloned().unwrap_or(&String::new()).clone();
                (c, name)
            })
            .unwrap_or((0, String::new()));

        let avg_count = if owner_count > 0 {
            (total_count as f64 / owner_count as f64).round() as u32
        } else {
            0
        };

        Self { owner_count, max_count, max_owner, avg_count, total_count }
    }
}

#[derive(Debug, Clone)]
pub struct UsageStatistics {
    pub db_type: DbType,
    pub user_id: i32,
    pub db_size_kb: u64,
    pub total_owner_count: u32,
    pub empty_db_count: u32,
    pub total_data_stats: DataStats,
    pub persistent_stats: DataStats,
    pub large_owners: Vec<OwnerStats>,
}

pub(crate) fn collect_all_usage_stats() {
    logi!("Start collecting asset usage statistics.");

    let user_ids = get_first_unlock_userids();

    for user_id in user_ids {
        if let Ok(stats) = collect_de_usage_stats(user_id) {
            if let Err(e) = upload_usage_statistics(&stats, USAGE_STATISTICS_DE) {
                loge!("Failed to upload DE usage statistics for user {}: {}", user_id, e);
            }
        }

        if is_db_key_cipher_file_exist(user_id).unwrap_or(false) {
            if let Ok(stats) = collect_ce_usage_stats(user_id) {
                if let Err(e) = upload_usage_statistics(&stats, USAGE_STATISTICS_CE) {
                    loge!("Failed to upload CE usage statistics for user {}: {}", user_id, e);
                }
            }
        }
    }

    logi!("Finish collecting asset usage statistics.");
}

fn collect_de_usage_stats(user_id: i32) -> Result<UsageStatistics> {
    let db_dir = format!("{}/{}", DE_ROOT_PATH, user_id);
    if !Path::new(&db_dir).exists() {
        return macros_lib::log_throw_error!(macros_lib::hisysevent::function!(),
            ErrCode::NotFound, "DE db not found for user {}", user_id);
    }

    let (all_owner_stats, db_size) = collect_db_stats_from_dir(&db_dir, user_id, &None)?;
    Ok(build_usage_statistics(DbType::De, user_id, all_owner_stats, db_size))
}

fn collect_ce_usage_stats(user_id: i32) -> Result<UsageStatistics> {
    let db_dir = format!("{}/{}/asset_service", CE_ROOT_PATH, user_id);
    if !Path::new(&db_dir).exists() {
        return macros_lib::log_throw_error!(macros_lib::hisysevent::function!(),
            ErrCode::NotFound, "CE db not found for user {}", user_id);
    }

    let db_key = get_db_key(user_id, true)?;

    let (all_owner_stats, db_size) = collect_db_stats_from_dir(&db_dir, user_id, &db_key)?;
    Ok(build_usage_statistics(DbType::Ce, user_id, all_owner_stats, db_size))
}

fn collect_db_stats_from_dir(
    db_dir: &str,
    user_id: i32,
    db_key: &Option<Vec<u8>>,
) -> Result<(Vec<OwnerStats>, u64)> {
    let mut all_owner_stats = Vec::new();
    let mut db_size = 0u64;

    for entry in fs::read_dir(db_dir)? {
        let entry = entry?;
        let path = entry.path();
        let file_name = path.file_name().map(|s| s.to_string_lossy().to_string()).unwrap_or("".to_string());
        if file_name.is_empty() {
            continue;
        }

        let file_size = match fs::metadata(&path) {
            Ok(m) => m.len(),
            Err(_e) => continue,
        };
        db_size += file_size;

        if file_name.ends_with(BACKUP_SUFFIX) || !file_name.ends_with(DB_SUFFIX) {
            continue;
        }

        let db_name = file_name.trim_end_matches(DB_SUFFIX);
        if file_name.is_empty() {
            continue;
        }

        match Database::build_with_file_name_without_lock(user_id, db_name, db_key) {
            Ok(mut db) => {
                if let Ok(owner_stats) = query_db_statistics(&mut db, db_name, file_size) {
                    all_owner_stats.push(owner_stats);
                }
            }
            Err(e) => {
                loge!("Failed to open database {} for user {}: {}", db_name, user_id, e);
            }
        }
    }
    Ok((all_owner_stats, db_size))
}

fn build_usage_statistics(
    db_type: DbType,
    user_id: i32,
    all_owner_stats: Vec<OwnerStats>,
    db_size_bytes: u64,
) -> UsageStatistics {
    let total_owner_count = all_owner_stats.len() as u32;
    let db_size_kb = db_size_bytes / BYTES_PER_KB;

    let empty_db_count = all_owner_stats.iter().filter(|s| s.total_count == 0).count() as u32;

    let mut db_names: Vec<&String> = Vec::with_capacity(total_owner_count as usize);
    let mut total_counts: Vec<u32> = Vec::with_capacity(total_owner_count as usize);
    let mut persistent_counts: Vec<u32> = Vec::with_capacity(total_owner_count as usize);

    for s in &all_owner_stats {
        db_names.push(&s.db_name);
        total_counts.push(s.total_count);
        persistent_counts.push(s.persistent_count);
    }

    let total_data_stats = DataStats::calculate(&total_counts, &db_names);
    let persistent_stats = DataStats::calculate(&persistent_counts, &db_names);

    let large_owners: Vec<OwnerStats> = all_owner_stats
        .into_iter()
        .filter(|s| s.total_count > THRESHOLD_LARGE_OWNER || s.db_size_kb > THRESHOLD_LARGE_DB_SIZE_KB)
        .take(MAX_LARGE_OWNERS)
        .collect();

    UsageStatistics {
        db_type,
        user_id,
        db_size_kb,
        total_owner_count,
        empty_db_count,
        total_data_stats,
        persistent_stats,
        large_owners,
    }
}

fn query_db_statistics(db: &mut Database, db_name: &str, db_file_size: u64) -> Result<OwnerStats> {
    let total_count = db.query_data_count(&DbMap::new())?;

    let mut persistent_condition = DbMap::new();
    persistent_condition.insert_attr(column::IS_PERSISTENT, true);
    let persistent_count = db.query_data_count(&persistent_condition)?;

    let db_size_kb = db_file_size / BYTES_PER_KB;

    Ok(OwnerStats {
        db_name: db_name.to_string(),
        total_count,
        persistent_count,
        db_size_kb,
    })
}

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    fn test_data_stats_calculate_empty() {
        let counts: Vec<u32> = vec![];
        let owner_names: Vec<&String> = vec![];
        let stats = DataStats::calculate(&counts, &owner_names);

        assert_eq!(stats.owner_count, 0);
        assert_eq!(stats.total_count, 0);
        assert_eq!(stats.max_count, 0);
        assert_eq!(stats.max_owner, "");
        assert_eq!(stats.avg_count, 0);
    }

    #[test]
    fn test_data_stats_calculate_single_owner() {
        let owner = String::from("com.test.owner1");
        let counts: Vec<u32> = vec![100];
        let owner_names: Vec<&String> = vec![&owner];
        let stats = DataStats::calculate(&counts, &owner_names);

        assert_eq!(stats.owner_count, 1);
        assert_eq!(stats.total_count, 100);
        assert_eq!(stats.max_count, 100);
        assert_eq!(stats.max_owner, "com.test.owner1");
        assert_eq!(stats.avg_count, 100);
    }

    #[test]
    fn test_data_stats_calculate_multiple_owners() {
        let owner1 = String::from("com.test.owner1");
        let owner2 = String::from("com.test.owner2");
        let owner3 = String::from("com.test.owner3");
        let counts: Vec<u32> = vec![50, 150, 100];
        let owner_names: Vec<&String> = vec![&owner1, &owner2, &owner3];
        let stats = DataStats::calculate(&counts, &owner_names);

        assert_eq!(stats.owner_count, 3);
        assert_eq!(stats.total_count, 300);
        assert_eq!(stats.max_count, 150);
        assert_eq!(stats.max_owner, "com.test.owner2");
        assert_eq!(stats.avg_count, 100);
    }

    #[test]
    fn test_data_stats_calculate_with_zeros() {
        let owner1 = String::from("com.test.owner1");
        let owner2 = String::from("com.test.owner2");
        let owner3 = String::from("com.test.owner3");
        let counts: Vec<u32> = vec![100, 0, 50];
        let owner_names: Vec<&String> = vec![&owner1, &owner2, &owner3];
        let stats = DataStats::calculate(&counts, &owner_names);

        assert_eq!(stats.owner_count, 2);
        assert_eq!(stats.total_count, 150);
        assert_eq!(stats.max_count, 100);
        assert_eq!(stats.max_owner, "com.test.owner1");
        assert_eq!(stats.avg_count, 75);
    }

    #[test]
    fn test_data_stats_calculate_average() {
        let owner1 = String::from("com.test.owner1");
        let owner2 = String::from("com.test.owner2");
        let counts: Vec<u32> = vec![99, 101];
        let owner_names: Vec<&String> = vec![&owner1, &owner2];
        let stats = DataStats::calculate(&counts, &owner_names);

        assert_eq!(stats.owner_count, 2);
        assert_eq!(stats.total_count, 200);
        assert_eq!(stats.avg_count, 100);
    }

    #[test]
    fn test_build_usage_statistics() {
        let db1 = String::from("Native_asset_service_8100");
        let db2 = String::from("Native_asset_service_8200");
        let db3 = String::from("Native_asset_service_8300");

        let all_owner_stats = vec![
            OwnerStats { db_name: db1.clone(), total_count: 150, persistent_count: 50, db_size_kb: 1024 },
            OwnerStats { db_name: db2.clone(), total_count: 80, persistent_count: 20, db_size_kb: 1024 },
            OwnerStats { db_name: db3.clone(), total_count: 50, persistent_count: 0, db_size_kb: 1024 },
        ];

        let usage_stats = build_usage_statistics(DbType::De, 0, all_owner_stats, 3 * 1024 * 1024);

        assert_eq!(usage_stats.db_type, DbType::De);
        assert_eq!(usage_stats.user_id, 0);
        assert_eq!(usage_stats.db_size_kb, 3 * 1024);
        assert_eq!(usage_stats.total_owner_count, 3);
        assert_eq!(usage_stats.empty_db_count, 0);

        assert_eq!(usage_stats.total_data_stats.owner_count, 3);
        assert_eq!(usage_stats.total_data_stats.total_count, 280);
        assert_eq!(usage_stats.total_data_stats.max_count, 150);
        assert_eq!(usage_stats.total_data_stats.max_owner, "Native_asset_service_8100");

        assert_eq!(usage_stats.persistent_stats.owner_count, 2);
        assert_eq!(usage_stats.persistent_stats.total_count, 70);

        assert_eq!(usage_stats.large_owners.len(), 1);
        assert_eq!(usage_stats.large_owners[0].db_name, "Native_asset_service_8100");
        assert_eq!(usage_stats.large_owners[0].total_count, 150);
    }

    #[test]
    fn test_build_usage_statistics_with_empty_db() {
        let db1 = String::from("Native_asset_service_8100");
        let db2 = String::from("Native_asset_service_8200");
        let db3 = String::from("Native_asset_service_8300");
        let db4 = String::from("Native_asset_service_8400");

        let all_owner_stats = vec![
            OwnerStats { db_name: db1.clone(), total_count: 150, persistent_count: 50, db_size_kb: 1024 },
            OwnerStats { db_name: db2.clone(), total_count: 0, persistent_count: 0, db_size_kb: 0 },
            OwnerStats { db_name: db3.clone(), total_count: 50, persistent_count: 0, db_size_kb: 1024 },
            OwnerStats { db_name: db4.clone(), total_count: 0, persistent_count: 0, db_size_kb: 0 },
        ];

        let usage_stats = build_usage_statistics(DbType::De, 0, all_owner_stats, 2 * 1024 * 1024);

        assert_eq!(usage_stats.total_owner_count, 4);
        assert_eq!(usage_stats.empty_db_count, 2);
        assert_eq!(usage_stats.total_data_stats.owner_count, 2);
        assert_eq!(usage_stats.total_data_stats.total_count, 200);
    }

    #[test]
    fn test_build_usage_statistics_large_owners_limit() {
        let mut all_owner_stats = Vec::new();
        for i in 0..500 {
            let db_name = format!("Native_asset_service_{}", 8100 + i);
            all_owner_stats.push(OwnerStats { db_name, total_count: 150, persistent_count: 50, db_size_kb: 1024 });
        }

        let usage_stats = build_usage_statistics(DbType::Ce, 100, all_owner_stats, 500 * 1024 * 1024);

        assert_eq!(usage_stats.db_type, DbType::Ce);
        assert_eq!(usage_stats.user_id, 100);
        assert_eq!(usage_stats.large_owners.len(), 300);
    }

    #[test]
    fn test_build_usage_statistics_no_large_owners() {
        let db1 = String::from("Native_asset_service_8100");
        let db2 = String::from("Native_asset_service_8200");

        let all_owner_stats = vec![
            OwnerStats { db_name: db1.clone(), total_count: 50, persistent_count: 10, db_size_kb: 100 },
            OwnerStats { db_name: db2.clone(), total_count: 30, persistent_count: 5, db_size_kb: 200 },
        ];

        let usage_stats = build_usage_statistics(DbType::De, 0, all_owner_stats, 300 * 1024);

        assert_eq!(usage_stats.large_owners.len(), 0);
    }

    #[test]
    fn test_build_usage_statistics_large_by_size() {
        let db1 = String::from("Native_asset_service_8100");
        let db2 = String::from("Native_asset_service_8200");

        let all_owner_stats = vec![
            OwnerStats { db_name: db1.clone(), total_count: 50, persistent_count: 10, db_size_kb: 600 },
            OwnerStats { db_name: db2.clone(), total_count: 30, persistent_count: 5, db_size_kb: 200 },
        ];

        let usage_stats = build_usage_statistics(DbType::De, 0, all_owner_stats, 800 * 1024);

        assert_eq!(usage_stats.large_owners.len(), 1);
        assert_eq!(usage_stats.large_owners[0].db_name, "Native_asset_service_8100");
        assert_eq!(usage_stats.large_owners[0].db_size_kb, 600);
    }

    #[test]
    fn test_db_type_equality() {
        assert_eq!(DbType::De, DbType::De);
        assert_eq!(DbType::Ce, DbType::Ce);
        assert_ne!(DbType::De, DbType::Ce);
    }

    #[test]
    fn test_owner_stats_clone() {
        let db_name = String::from("Native_asset_service_8100");
        let stats = OwnerStats { db_name: db_name.clone(), total_count: 100, persistent_count: 50, db_size_kb: 2048 };
        let cloned = stats.clone();

        assert_eq!(stats.db_name, cloned.db_name);
        assert_eq!(stats.total_count, cloned.total_count);
        assert_eq!(stats.persistent_count, cloned.persistent_count);
        assert_eq!(stats.db_size_kb, cloned.db_size_kb);
    }
}