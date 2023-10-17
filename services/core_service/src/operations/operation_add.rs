/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

//! This crate implements the asset // todo: zwz 增加补充注释

use asset_common::{
    definition::{AssetMap, ConflictResolution, ErrCode, Result, Tag, Value, AuthType, Accessibility, SyncType},
    loge, logi, impl_enum_trait,
};
use db_operator::{
    database_table_helper::{COLUMN_SECRET, COLUMN_ALIAS, COLUMN_OWNER, COLUMN_OWNER_TYPE,
        COLUMN_SYNC_TYPE, COLUMN_AUTH_TYPE, COLUMN_ACCESSIBILITY, COLUMN_REQUIRE_PASSWORD_SET,
        COLUMN_DELETE_TYPE, COLUMN_VERSION, COLUMN_CREATE_TIME, COLUMN_UPDATE_TIME, do_transaction, DefaultDatabaseHelper},
    types::DbMap, database::Database
};

use crate::{
    calling_info::CallingInfo,
    operations::operation_common::{
        add_owner_info, create_user_db_dir, encrypt, get_system_time,
        db_adapter::into_db_map
    },
    VERSION
};

impl_enum_trait! {
    enum DeleteType {
        Never = 0,
        WhenUninstallApp = 1 << 0,
        WhenRemoveUser = 1 << 1,
        WhenClearAppData = 1 << 2,
    }
}

fn replace_db_record(calling_info: &CallingInfo, query_db_data: &DbMap, replace_db_data: &DbMap) -> Result<()> {
    let replace_callback = |db: &Database| -> bool {
        if db.delete_datas_default(query_db_data).is_err() {
            loge!("remove asset in replace operation failed!");
            return false;
        }
        if db.insert_datas_default(replace_db_data).is_err() {
            loge!("insert asset in replace operation failed!");
            return false;
        }
        true
    };

    if !do_transaction(calling_info.user_id(), replace_callback)? {
        loge!("do_transaction in replace_db_record failed!");
        return Err(ErrCode::SqliteError);
    }
    Ok(())
}

fn resolve_conflict(calling_info: &CallingInfo, attrs: &AssetMap, db_data: &DbMap) -> Result<()> {
    let Value::Bytes(ref alias) = attrs[&Tag::Alias] else { return Err(ErrCode::InvalidArgument) };
    let mut query = DbMap::new();
    query.insert(COLUMN_ALIAS, Value::Bytes(alias.clone()));
    query.insert(COLUMN_OWNER, Value::Bytes(calling_info.owner_info().clone()));
    query.insert(COLUMN_OWNER_TYPE, Value::Number(calling_info.owner_type()));

    if DefaultDatabaseHelper::is_data_exists_default_once(calling_info.user_id(), &query)? {
        match attrs.get(&Tag::ConflictResolution) {
            Some(Value::Number(num)) if *num == ConflictResolution::Overwrite as u32 =>
                return replace_db_record(calling_info, &query, db_data),
            _ => {
                loge!("[FATAL]The specified alias already exists.");
                return Err(ErrCode::Duplicated);
            },
        }
    }
    Ok(())
}

fn add_system_attrs(db_data: &mut DbMap) -> Result<()> {
    let delete_type = DeleteType::WhenUninstallApp as u32 | DeleteType::WhenRemoveUser as u32;
    db_data.insert(COLUMN_DELETE_TYPE, Value::Number(delete_type));
    db_data.insert(COLUMN_VERSION, Value::Number(VERSION));

    let time = get_system_time()?;
    db_data.insert(COLUMN_CREATE_TIME, Value::Bytes(time.clone()));
    db_data.insert(COLUMN_UPDATE_TIME, Value::Bytes(time));
    Ok(())
}

fn add_default_attrs(db_data: &mut DbMap) {
    db_data.entry(COLUMN_ACCESSIBILITY).or_insert(Value::Number(Accessibility::DeviceFirstUnlock as u32));
    db_data.entry(COLUMN_AUTH_TYPE).or_insert(Value::Number(AuthType::None as u32));
    db_data.entry(COLUMN_SYNC_TYPE).or_insert(Value::Number(SyncType::Never as u32));
    db_data.entry(COLUMN_REQUIRE_PASSWORD_SET).or_insert(Value::Number(false as u32));
}

pub(crate) fn add(attributes: &AssetMap, calling_info: &CallingInfo) -> Result<()> {
    // Create database directory if not exists.
    create_user_db_dir(calling_info.user_id())?;

    // Fill all attributes to DbMap.
    let mut db_data = into_db_map(attributes);
    add_owner_info(calling_info, &mut db_data);
    add_system_attrs(&mut db_data)?;
    add_default_attrs(&mut db_data);

    let cipher = encrypt(calling_info, &db_data)?;
    db_data.insert(COLUMN_SECRET, Value::Bytes(cipher));

    resolve_conflict(calling_info, attributes, &db_data)?;

    // call sql to add
    let insert_num = DefaultDatabaseHelper::insert_datas_default_once(calling_info.user_id(), &db_data)?;

    logi!("insert {} data", insert_num);
    Ok(())
}
