// Performance metrics macro for SAF service
// Inspired by asset service's execute macro design

use crate::wrapper::{notify_error, notify_performance_metrics};
use saf_log::{loge, logi};
use std::time::Instant;

macro_rules! execute_with_metrics {
    // Pattern 1: Function call with metrics tracking (no permission check)
    // Usage: execute_with_metrics!("func_name", func_path, item_count, arg1, arg2, ...)
    ($func_name:expr, $func:path, $item_count:expr, $($arg:expr),+) => {{
        let start_time = Instant::now();
        let function_name = $func_name.to_string();

        let result = $func($($arg),+);

        match &result {
            Ok(items) => {
                let elapsed_time = start_time.elapsed().as_millis() as i32;
                notify_performance_metrics(
                    $item_count,
                    elapsed_time,
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
                    function_name.clone()
                );
                loge!("{} failed: code={}, elapsed={}ms",
                      $func_name, e.code, elapsed_time);
            }
        }

        result
    }};
}
