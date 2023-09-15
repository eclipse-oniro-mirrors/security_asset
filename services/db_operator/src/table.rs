//! Copyright (C) 2023 Huawei Device Co., Ltd.
//! Licensed under the Apache License, Version 2.0 (the "License");
//! you may not use this file except in compliance with the License.
//! You may obtain a copy of the License at
//!
//! http://www.apache.org/licenses/LICENSE-2.0
//!
//! Unless required by applicable law or agreed to in writing, software
//! distributed under the License is distributed on an "AS IS" BASIS,
//! WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//! See the License for the specific language governing permissions and
//! limitations under the License.
use crate::{
    database::Database,
    sqlite3_changes_func,
    statement::Statement,
    types::{
        from_data_value_to_str_value, from_datatype_to_str, ColumnInfo, Condition, DataValue, Pair,
        ResultDataValue, ResultSet,
    },
    SqliteErrCode, SQLITE_BLOB, SQLITE_DONE, SQLITE_ERROR, SQLITE_FLOAT, SQLITE_INTEGER,
    SQLITE_NULL, SQLITE_OK, SQLITE_ROW, SQLITE_TEXT,
};

/// a database table
#[repr(C)]
pub struct Table<'a> {
    /// table name
    pub table_name: String,
    /// point to db
    pub db: &'a Database<'a>,
}

impl<'a> Table<'a> {
    pub(crate) fn new(table_name: &str, db: &'a Database) -> Table<'a> {
        Table { table_name: table_name.to_string(), db }
    }

    /// the param conditions is to build sql after 'where'
    /// the param datas is to build sql between 'set' and 'where'
    /// like this sql: update table_test set alias='test_update' where id=2
    /// the code like:
    ///
    /// let conditions = &vec![Pair {
    ///     column_name: "id",
    ///     value: DataValue::Integer(2),
    /// }];
    ///
    /// let datas = &vec![Pair {
    ///     column_name: "alias",
    ///     value: DataValue::Text(b"test_update"),
    /// }];
    ///
    /// let ret = table.update_row(conditions, datas);
    pub fn update_row(&self,
                      conditions: &Condition,
                      datas: &Vec<Pair>)
                      -> Result<i32, SqliteErrCode> {
        let mut sql = format!("update {} set ", self.table_name);
        for i in 0..datas.len() {
            let data = &datas[i];
            sql.push_str(data.column_name);
            sql.push_str("=?");
            if i != datas.len() - 1 {
                sql.push(',');
            }
        }
        if !conditions.is_empty() {
            sql.push_str(" where ");
            for i in 0..conditions.len() {
                let pair = &conditions[i];
                sql.push_str(pair.column_name);
                sql.push_str("=?");
                if i != conditions.len() - 1 {
                    sql.push_str(" and ");
                }
            }
        }
        #[cfg(test)]
        {
            println!("{}", sql);
        }
        let stmt = match Statement::<true>::prepare(sql.as_str(), self.db) {
            Ok(s) => s,
            Err(e) => {
                #[cfg(test)]
                {
                    let msg = self.db.get_errmsg().unwrap();
                    println!("prepare update row fail ret {}, info: {}", e, msg.s);
                }
                return Err(e);
            },
        };
        let mut index = 1;
        for data in datas {
            let ret = stmt.bind_data(index, &data.value);
            if ret != SQLITE_OK {
                return Err(ret);
            }
            index += 1;
        }
        for pair in conditions {
            let ret = stmt.bind_data(index, &pair.value);
            if ret != SQLITE_OK {
                return Err(ret);
            }
            index += 1;
        }
        let ret = stmt.step();
        if ret != SQLITE_DONE {
            return Err(ret);
        }
        let count = sqlite3_changes_func(self.db.handle);
        Ok(count)
    }

    /// update single data
    /// sql like: update table_name set column_name=data_new [where conditions]
    pub fn update_row_column(&self,
                             conditions: &Condition,
                             column_name: &str,
                             data_new: DataValue)
                             -> Result<i32, SqliteErrCode> {
        let datas = vec![Pair { column_name, value: data_new }];
        self.update_row(conditions, &datas)
    }

    /// delete row from table
    /// like this sql: delete from table_test where id=2
    /// the code like:
    ///
    /// let conditions = &vec![Pair {
    ///     column_name: "id",
    ///     value: DataValue::Integer(2),
    /// }];
    ///
    /// let ret = table.delete_row(conditions);
    pub fn delete_row(&self, conditions: &Condition) -> Result<i32, SqliteErrCode> {
        let mut sql = format!("delete from {}", self.table_name);
        if !conditions.is_empty() {
            sql.push_str(" where ");
            for i in 0..conditions.len() {
                let cond = &conditions[i];
                sql.push_str(cond.column_name);
                sql.push_str("=?");
                if i != conditions.len() - 1 {
                    sql.push_str(" and ")
                }
            }
        }
        #[cfg(test)]
        {
            println!("{}", sql);
        }
        let stmt = match Statement::<true>::prepare(sql.as_str(), self.db) {
            Ok(s) => s,
            Err(e) => {
                #[cfg(test)]
                {
                    let msg = self.db.get_errmsg().unwrap();
                    println!("prepare delete row fail ret {}, info: {}", e, msg.s);
                }
                return Err(e);
            },
        };
        let mut index = 1;
        for cond in conditions {
            let ret = stmt.bind_data(index, &cond.value);
            if ret != SQLITE_OK {
                return Err(ret);
            }
            index += 1;
        }
        let ret = stmt.step();
        if ret != SQLITE_DONE {
            return Err(ret);
        }
        let count = sqlite3_changes_func(self.db.handle);
        Ok(count)
    }

    /// insert into table, datas is the value to be insert.
    /// sql like: insert into table_test (id,alias) values (3,'alias1')
    /// code like this:
    ///
    /// let datas = &vec![
    ///     Pair {
    ///         column_name: "id",
    ///         value: DataValue::Integer(3),
    ///     },
    ///     Pair {
    ///         column_name: "alias",
    ///         value: DataValue::Text(b"alias1"),
    ///     },
    /// ];
    /// let ret = table.insert_row(datas);
    pub fn insert_row(&self, datas: &Vec<Pair>) -> Result<i32, SqliteErrCode> {
        let mut sql = format!("insert into {} (", self.table_name);
        for i in 0..datas.len() {
            let data = &datas[i];
            sql.push_str(data.column_name);
            if i != datas.len() - 1 {
                sql.push(',');
            }
        }
        sql.push_str(") values (");
        for i in 0..datas.len() {
            sql.push('?');
            if i != datas.len() - 1 {
                sql.push(',');
            }
        }
        sql.push(')');
        #[cfg(test)]
        {
            println!("{}", sql);
        }
        let stmt = match Statement::<true>::prepare(sql.as_str(), self.db) {
            Ok(s) => s,
            Err(e) => {
                #[cfg(test)]
                {
                    let msg = self.db.get_errmsg().unwrap();
                    println!("prepare insert row fail ret {}, info: {}", e, msg.s);
                }
                return Err(e);
            },
        };
        let mut index = 1;
        for data in datas {
            let ret = stmt.bind_data(index, &data.value);
            if ret != SQLITE_OK {
                return Err(ret);
            }
            index += 1;
        }
        let ret = stmt.step();
        if ret != SQLITE_DONE {
            return Err(ret);
        }
        let count = sqlite3_changes_func(self.db.handle);
        Ok(count)
    }

    /// insert into table, datas is the value to be insert.
    /// sql like: insert into table_test values (3,'alias1')
    /// code like this:
    ///
    /// let datas = &vec![DataValue::Integer(3), DataValue::Text(b"alias1")];
    /// let ret = table.insert_row_datas(datas);
    pub fn insert_row_datas(&self, datas: &Vec<DataValue>) -> Result<i32, SqliteErrCode> {
        let mut sql = format!("insert into {} ", self.table_name);
        sql.push_str("values (");
        for i in 0..datas.len() {
            sql.push('?');
            if i != datas.len() - 1 {
                sql.push(',');
            }
        }
        sql.push(')');
        #[cfg(test)]
        {
            println!("{}", sql);
        }
        let stmt = match Statement::<true>::prepare(sql.as_str(), self.db) {
            Ok(s) => s,
            Err(e) => {
                #[cfg(test)]
                {
                    let msg = self.db.get_errmsg().unwrap();
                    println!("prepare insert row data fail ret {}, info: {}", e, msg.s);
                }
                return Err(e);
            },
        };
        let mut index = 1;
        for data in datas {
            let ret = stmt.bind_data(index, data);
            if ret != SQLITE_OK {
                return Err(ret);
            }
            index += 1;
        }
        let ret = stmt.step();
        if ret != SQLITE_DONE {
            return Err(ret);
        }
        let count = sqlite3_changes_func(self.db.handle);
        Ok(count)
    }

    /// insert into table, dataset is the value to be insert.
    /// sql like: insert into table_test values (3,'alias1')
    /// code like this:
    ///
    /// let columns = &vec!["AppId", "Alias", "value"];
    /// let dataset = vec![
    ///     vec![
    ///         DataValue::Text(b"appid1"),
    ///         DataValue::Text(b"alias1"),
    ///         DataValue::Text(b"a"),
    ///     ],
    ///     vec![
    ///         DataValue::Text(b"appid2"),
    ///         DataValue::Text(b"alias2"),
    ///         DataValue::Text(b"b"),
    ///     ],
    ///     vec![
    ///         DataValue::Text(b"appid3"),
    ///         DataValue::Text(b"alias3"),
    ///         DataValue::Text(b"c"),
    ///     ],
    /// ];
    /// let count = table.insert_multi_row_datas(columns, &dataset);
    pub fn insert_multi_row_datas(&self,
                                  columns: &Vec<&str>,
                                  dataset: &Vec<Vec<DataValue>>)
                                  -> Result<i32, SqliteErrCode> {
        let mut sql = format!("insert into {} (", self.table_name);
        for i in 0..columns.len() {
            let column = &columns[i];
            sql.push_str(column);
            if i != columns.len() - 1 {
                sql.push(',');
            }
        }
        sql.push_str(") values (");
        for i in 0..columns.len() {
            sql.push('?');
            if i != columns.len() - 1 {
                sql.push(',');
            }
        }
        sql.push(')');
        #[cfg(test)]
        {
            println!("{}", sql);
        }
        let stmt = match Statement::<true>::prepare(sql.as_str(), self.db) {
            Ok(s) => s,
            Err(e) => {
                #[cfg(test)]
                {
                    let msg = self.db.get_errmsg().unwrap();
                    println!("prepare insert row data fail ret {}, info: {}", e, msg.s);
                }
                return Err(e);
            },
        };
        let mut count = 0;
        for datas in dataset {
            let ret = stmt.reset();
            if ret != SQLITE_OK {
                return Err(ret);
            }
            let mut index = 1;
            for data in datas {
                let ret = stmt.bind_data(index, data);
                if ret != SQLITE_OK {
                    return Err(ret);
                }
                index += 1;
            }
            let ret = stmt.step();
            if ret != SQLITE_DONE {
                return Err(ret);
            }
            count += 1;
        }
        Ok(count)
    }

    /// rename table name
    pub fn rename(&mut self, name: &str) -> SqliteErrCode {
        let sql = format!("ALTER TABLE {} RENAME TO {}", self.table_name, name);
        #[cfg(test)]
        {
            println!("{}", sql);
        }
        let stmt = &Statement::<false>::new(sql.as_str(), self.db);
        let ret = stmt.exec(None, 0);
        if ret == SQLITE_OK {
            self.table_name = name.to_string();
        }
        ret
    }

    /// add new column for table
    /// 1. can not add primary key
    /// 2. can not add not null key if no default value
    /// 3. only double/int/text support default value, blob data default value is always null
    ///
    /// code like:
    /// let ret = table.add_new_column(
    ///     ColumnInfo {
    ///         name: "id",
    ///         data_type: DataType::INTEGER,
    ///         is_primary_key: false,
    ///         not_null: true,
    ///     },
    ///     Some(DataValue::Integer(0)),
    /// );
    pub fn add_new_column(&self,
                          column: ColumnInfo,
                          default_value: Option<DataValue>)
                          -> SqliteErrCode {
        if column.is_primary_key {
            return SQLITE_ERROR;
        }
        if column.not_null && default_value.is_none() {
            return SQLITE_ERROR;
        }
        let datatype = from_datatype_to_str(column.data_type);
        let mut sql =
            format!("ALTER TABLE {} ADD COLUMN {} {}", self.table_name, column.name, datatype);
        if let Some(data) = default_value {
            sql.push_str(" DEFAULT ");
            sql.push_str(&from_data_value_to_str_value(data));
        }
        if column.not_null {
            sql.push_str(" NOT NULL");
        }
        #[cfg(test)]
        {
            println!("{}", sql);
        }
        let stmt = Statement::<false>::new(sql.as_str(), self.db);

        stmt.exec(None, 0)
    }

    /// query datas from table,
    /// if length of columns is 0, will select *.
    /// if length of conditions is 0, will select all data.
    ///
    /// code like:
    /// let result_set = table.query_row(&vec!["alias", "blobs"], &vec![]);
    ///
    /// means sql like: select alias,blobs from table_name
    pub fn query_row(&self,
                     columns: &Vec<&str>,
                     conditions: &Condition)
                     -> Result<ResultSet, SqliteErrCode> {
        let mut sql = String::from("select ");
        if !columns.is_empty() {
            for i in 0..columns.len() {
                sql.push_str(columns[i]);
                if i != columns.len() - 1 {
                    sql.push(',');
                }
            }
        } else {
            sql.push('*');
        }
        sql.push_str(" from ");
        sql.push_str(self.table_name.as_str());
        if !conditions.is_empty() {
            sql.push_str(" where ");
            for i in 0..conditions.len() {
                let cond = &conditions[i];
                sql.push_str(cond.column_name);
                sql.push_str("=?");
                if i != conditions.len() - 1 {
                    sql.push_str(" and ")
                }
            }
        }
        #[cfg(test)]
        {
            println!("{}", sql);
        }
        let stmt = match Statement::<true>::prepare(sql.as_str(), self.db) {
            Ok(s) => s,
            Err(e) => {
                #[cfg(test)]
                {
                    let msg = self.db.get_errmsg().unwrap();
                    println!("prepare query row fail ret {}, info: {}", e, msg.s);
                }
                return Err(e);
            },
        };
        let mut index = 1;
        for cond in conditions {
            let ret = stmt.bind_data(index, &cond.value);
            if ret != SQLITE_OK {
                return Err(ret);
            }
            index += 1;
        }
        let mut result = vec![];
        while stmt.step() == SQLITE_ROW {
            let mut data_line = Vec::<ResultDataValue>::new();
            let n = stmt.data_count();
            for i in 0..n {
                let tp = stmt.column_type(i);
                let data = match tp {
                    SQLITE_TEXT => {
                        let text = stmt.query_column_text(i);
                        ResultDataValue::Text(if text.is_empty() {
                                                  None
                                              } else {
                                                  Some(Box::new(text.to_vec()))
                                              })
                    },
                    SQLITE_INTEGER => ResultDataValue::Integer(stmt.query_column_int(i)),
                    SQLITE_FLOAT => ResultDataValue::Double(stmt.query_column_double(i)),
                    SQLITE_BLOB => {
                        let blob = stmt.query_column_blob(i);
                        ResultDataValue::Blob(if blob.is_empty() {
                                                  None
                                              } else {
                                                  Some(Box::new(blob.to_vec()))
                                              })
                    },
                    SQLITE_NULL => ResultDataValue::Blob(None),
                    _ => return Err(SQLITE_ERROR),
                };
                data_line.push(data);
            }
            result.push(data_line);
        }
        Ok(result)
    }

    /// return the count of datas with conditions, the length of conditions may be 0.
    ///
    /// code like:
    /// let count = table
    ///     .count_datas(&vec![Pair {
    ///         column_name: "id",
    ///         value: DataValue::Integer(3),
    ///     }]);
    ///
    /// the sql is like : select count(*) as count from table_name where id=3
    pub fn count_datas(&self, conditions: &Condition) -> Result<u32, SqliteErrCode> {
        let mut sql = format!("select count(*) as count from {}", self.table_name);
        if !conditions.is_empty() {
            sql.push_str(" where ");
            for i in 0..conditions.len() {
                let cond = &conditions[i];
                sql.push_str(cond.column_name);
                sql.push_str("=?");
                if i != conditions.len() - 1 {
                    sql.push_str(" and ")
                }
            }
        }
        #[cfg(test)]
        {
            println!("{}", sql);
        }
        let stmt = match Statement::<true>::prepare(sql.as_str(), self.db) {
            Ok(s) => s,
            Err(e) => {
                #[cfg(test)]
                {
                    let msg = self.db.get_errmsg().unwrap();
                    println!("prepare count datas fail ret {}, info: {}", e, msg.s);
                }
                return Err(e);
            },
        };
        let mut index = 1;
        for cond in conditions {
            let ret = stmt.bind_data(index, &cond.value);
            if ret != SQLITE_OK {
                return Err(ret);
            }
            index += 1;
        }
        let ret = stmt.step();
        if ret != SQLITE_ROW {
            return Err(ret);
        }
        let count = stmt.query_column_int(0);
        Ok(count)
    }

    /// return if the data exists
    ///
    /// code like:
    /// let exits = table
    ///     .is_data_exists(&vec![
    ///         Pair {
    ///             column_name: "id",
    ///             value: DataValue::Integer(3),
    ///         },
    ///         Pair {
    ///             column_name: "alias",
    ///             value: DataValue::Text(b"test test"),
    ///         },
    ///     ]);
    ///
    /// the sql is like: select count(*) as count from table_name where id=3 and alias='test test'
    /// if count > 0, data exists
    pub fn is_data_exists(&self, cond: &Condition) -> Result<bool, SqliteErrCode> {
        let ret = self.count_datas(cond);
        match ret {
            Ok(count) => Ok(count > 0),
            Err(e) => Err(e),
        }
    }
}
