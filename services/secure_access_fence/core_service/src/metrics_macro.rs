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

// Performance metrics macro for SAF service
// Inspired by asset service's execute macro design

macro_rules! execute_with_metrics {
    // Pattern 1: Function call with metrics tracking (no permission check)
    // Usage: execute_with_metrics!("func_name", func_path, item_count, arg1, arg2, ...)
    ($func_name:expr, $func:path, $item_count:expr, $os_account_id:expr, $($arg:expr),+) => {{
        let start_time = Instant::now();
        let function_name = $func_name.to_string();

        let result = $func($os_account_id, $($arg),+);

        match &result {
            Ok(items) => {
                let elapsed_time = start_time.elapsed().as_millis() as i32;
                notify_performance_metrics(
                    $item_count,
                    elapsed_time,
                    $os_account_id,
                    function_name.clone()
                );
                logi!("{} success: count={}, elapsed={}ms",
                      $func_name, items.len(), elapsed_time);
            },
            Err(e) => {
                let elapsed_time = start_time.elapsed().as_millis() as i32;
                notify_error(
                    format!("{} failed: {:?}", $func_name, e),
                    e.code as i32,
                    $os_account_id,
                    function_name.clone()
                );
                loge!("{} failed: code={}, elapsed={}ms",
                      $func_name, e.code, elapsed_time);
            }
        }

        result
    }};
}

// Sibling macro for batch operations returning a struct with an `error_code` field
// (BatchGenerateResult / BatchVerifyResult). Same call signature as execute_with_metrics!,
// but branches on `result.error_code == ErrCode::Success` instead of Ok/Err, because
// partial failures are encoded in the error_code field rather than as Err.
// Usage: execute_with_metrics_batch!("func_name", func_path, item_count, os_account_id, arg1, arg2, ...)
macro_rules! execute_with_metrics_batch {
    ($func_name:expr, $func:path, $item_count:expr, $os_account_id:expr, $($arg:expr),+) => {{
        let start_time = Instant::now();
        let function_name = $func_name.to_string();
        let item_count = $item_count;

        let result = $func($os_account_id, $($arg),+);

        let elapsed_time = start_time.elapsed().as_millis() as i32;

        if result.error_code == ErrCode::Success as i32 {
            notify_performance_metrics(
                item_count,
                elapsed_time,
                $os_account_id,
                function_name.clone()
            );
            logi!("{} success: count={}, elapsed={}ms",
                  $func_name, item_count, elapsed_time);
        } else {
            notify_error(
                format!("{} failed: error_code={}", $func_name, result.error_code),
                result.error_code,
                $os_account_id,
                function_name.clone()
            );
            loge!("{} failed: error_code={}, elapsed={}ms",
                  $func_name, result.error_code, elapsed_time);
        }

        result
    }};
}
