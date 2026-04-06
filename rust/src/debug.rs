use std::fs::File;
use std::io::Write;
use std::sync::Mutex;

pub struct DebugLogger {
    file: Mutex<Option<File>>,
}

impl DebugLogger {
    pub fn new(path: Option<&std::path::Path>) -> Option<Self> {
        let resolved = path.map(|p| p.to_path_buf()).unwrap_or_else(|| {
            std::env::temp_dir().join(format!("slit-{}.log", std::process::id()))
        });
        match File::options().create(true).append(true).open(&resolved) {
            Ok(f) => Some(Self {
                file: Mutex::new(Some(f)),
            }),
            Err(_) => None,
        }
    }

    pub fn log(&self, msg: &str) {
        if let Ok(mut guard) = self.file.lock() {
            if let Some(ref mut f) = *guard {
                let now = chrono::Local::now();
                let _ = writeln!(f, "{} {}", now.format("%H:%M:%S%.3f"), msg);
            }
        }
    }

    pub fn close(&self) {
        if let Ok(mut guard) = self.file.lock() {
            *guard = None;
        }
    }
}
