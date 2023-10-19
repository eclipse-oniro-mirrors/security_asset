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
    definition::{Accessibility, AssetMap, AuthType, ConflictResolution, ErrCode, Result, SyncType, Tag, Value},
    impl_enum_trait, loge, logi,
};
use asset_db_operator::{
    database::Database,
    database_table_helper::{
        do_transaction, DefaultDatabaseHelper, COLUMN_ACCESSIBILITY, COLUMN_ALIAS, COLUMN_AUTH_TYPE,
        COLUMN_CREATE_TIME, COLUMN_DELETE_TYPE, COLUMN_OWNER, COLUMN_OWNER_TYPE, COLUMN_REQUIRE_PASSWORD_SET,
        COLUMN_SECRET, COLUMN_SYNC_TYPE, COLUMN_UPDATE_TIME, COLUMN_VERSION, DB_DATA_VERSION,
    },
    types::DbMap,
};

use crate::{calling_info::CallingInfo, operations::common};

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

fn resolve_conflict(calling_info: &CallingInfo, attrs: &AssetMap, query: &DbMap, db_data: &DbMap) -> Result<()> {
    match attrs.get(&Tag::ConflictResolution) {
        Some(Value::Number(num)) if *num == ConflictResolution::Overwrite as u32 => {
            replace_db_record(calling_info, query, db_data)
        },
        _ => {
            loge!("[FATAL]The specified alias already exists.");
            Err(ErrCode::Duplicated)
        },
    }
}

fn get_query_condition(calling_info: &CallingInfo, attrs: &AssetMap) -> Result<DbMap> {
    let Value::Bytes(ref alias) = attrs[&Tag::Alias] else { return Err(ErrCode::InvalidArgument) };
    let mut query = DbMap::new();
    query.insert(COLUMN_ALIAS, Value::Bytes(alias.clone()));
    query.insert(COLUMN_OWNER, Value::Bytes(calling_info.owner_info().clone()));
    query.insert(COLUMN_OWNER_TYPE, Value::Number(calling_info.owner_type()));
    Ok(query)
}

fn add_system_attrs(db_data: &mut DbMap) -> Result<()> {
    let delete_type = DeleteType::WhenUninstallApp as u32 | DeleteType::WhenRemoveUser as u32;
    db_data.insert(COLUMN_DELETE_TYPE, Value::Number(delete_type));
    db_data.insert(COLUMN_VERSION, Value::Number(DB_DATA_VERSION));

    let time = common::get_system_time()?;
    db_data.insert(COLUMN_CREATE_TIME, Value::Bytes(time.clone()));
    db_data.insert(COLUMN_UPDATE_TIME, Value::Bytes(time));
    Ok(())
}

fn add_default_attrs(db_data: &mut DbMap) {
    db_data.entry(COLUMN_ACCESSIBILITY).or_insert(Value::Number(Accessibility::DeviceFirstUnlock as u32));
    db_data.entry(COLUMN_AUTH_TYPE).or_insert(Value::Number(AuthType::None as u32));
    db_data.entry(COLUMN_SYNC_TYPE).or_insert(Value::Number(SyncType::Never as u32));
    db_data.entry(COLUMN_REQUIRE_PASSWORD_SET).or_insert(Value::Bool(false));
}

const REQUIRED_ATTRS: [Tag; 2] = [Tag::Secret, Tag::Alias];

const OPTIONAL_ATTRS: [Tag; 2] = [Tag::Secret, Tag::ConflictResolution];

fn check_arguments(attributes: &AssetMap) -> Result<()> {
    common::check_required_tags(attributes, &REQUIRED_ATTRS)?;

    let mut optional_tags = common::CRITICAL_LABEL_ATTRS.to_vec();
    optional_tags.extend_from_slice(&common::NORMAL_LABEL_ATTRS);
    optional_tags.extend_from_slice(&common::ACCESS_CONTROL_ATTRS);
    optional_tags.extend_from_slice(&OPTIONAL_ATTRS);
    common::check_optional_tags(attributes, &optional_tags)?;
    common::check_value_validity(attributes)
}

pub(crate) fn add(attributes: &AssetMap, calling_info: &CallingInfo) -> Result<()> {
    check_arguments(attributes)?;

    // Create database directory if not exists.
    common::create_user_db_dir(calling_info.user_id())?;

    // Fill all attributes to DbMap.
    let mut db_data = common::into_db_map(attributes);
    common::add_owner_info(calling_info, &mut db_data);
    add_system_attrs(&mut db_data)?;
    add_default_attrs(&mut db_data);

    let cipher = common::encrypt(calling_info, &db_data)?;
    db_data.insert(COLUMN_SECRET, Value::Bytes(cipher));

    let query = get_query_condition(calling_info, attributes)?;
    if DefaultDatabaseHelper::is_data_exists_default_once(calling_info.user_id(), &query)? {
        resolve_conflict(calling_info, attributes, &query, &db_data)
    } else {
        let insert_num = DefaultDatabaseHelper::insert_datas_default_once(calling_info.user_id(), &db_data)?;
        logi!("insert {} data", insert_num);
        Ok(())
    }
}
