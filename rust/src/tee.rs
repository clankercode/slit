use std::fs::{File, OpenOptions};
use std::io::Write;
use std::path::Path;

pub struct TeeWriter {
    file: Option<File>,
}

impl TeeWriter {
    pub fn new(path: &Path, append: bool) -> Option<Self> {
        let mut opts = OpenOptions::new();
        opts.create(true).write(true);
        if append {
            opts.append(true);
        } else {
            opts.truncate(true);
        }
        match opts.open(path) {
            Ok(f) => Some(Self { file: Some(f) }),
            Err(e) => {
                eprintln!("slit: warning: cannot open tee file {:?}: {}", path, e);
                None
            }
        }
    }

    pub fn write_line(&mut self, line: &str) {
        if let Some(ref mut f) = self.file {
            let _ = writeln!(f, "{}", line);
        }
    }

    pub fn close(&mut self) {
        self.file = None;
    }
}

impl Drop for TeeWriter {
    fn drop(&mut self) {
        self.close();
    }
}
