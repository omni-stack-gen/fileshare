use crate::{AppWindow, State};
use libc::{self, time_t, tm};
use slint::{ComponentHandle, Image, ModelRc, SharedString, VecModel};
use std::fs;
use std::mem::MaybeUninit;
use std::path::{Path, PathBuf};
use std::sync::{Arc, Mutex};

const PAGE_SIZE: usize = 6;
const UNREAD_PREFIX: &str = "unread__";
const READ_PREFIX: &str = "read__";

#[derive(Clone, Debug)]
struct CaptureRecord {
    path: PathBuf,
    label: String,
    unread: bool,
    timestamp: u64,
    date_text: String,
    time_text: String,
}

#[derive(Debug)]
struct CaptureGallery {
    capture_dir: PathBuf,
    records: Vec<CaptureRecord>,
    page_index: usize,
    selected_index: Option<usize>,
    full_index: Option<usize>,
}

impl CaptureGallery {
    fn new(capture_dir: impl Into<PathBuf>) -> Self {
        Self {
            capture_dir: capture_dir.into(),
            records: Vec::new(),
            page_index: 0,
            selected_index: None,
            full_index: None,
        }
    }

    fn refresh(&mut self) {
        self.records = scan_capture_records(&self.capture_dir);
        if self.page_index >= self.page_count() {
            self.page_index = self.page_count().saturating_sub(1);
        }
        if self
            .selected_index
            .map(|index| index >= self.records.len())
            .unwrap_or(false)
        {
            self.selected_index = None;
        }
        if self
            .full_index
            .map(|index| index >= self.records.len())
            .unwrap_or(false)
        {
            self.full_index = None;
        }
    }

    fn page_count(&self) -> usize {
        ((self.records.len() + PAGE_SIZE - 1) / PAGE_SIZE).max(1)
    }

    fn current_page_range(&self) -> std::ops::Range<usize> {
        let start = self.page_index * PAGE_SIZE;
        let end = (start + PAGE_SIZE).min(self.records.len());
        start..end
    }

    fn current_page_records(&self) -> &[CaptureRecord] {
        let range = self.current_page_range();
        &self.records[range]
    }

    fn select_page_item(&mut self, page_slot: usize) {
        let absolute = self.page_index * PAGE_SIZE + page_slot;
        if absolute >= self.records.len() {
            self.selected_index = None;
            return;
        }
        if self.selected_index == Some(absolute) {
            self.selected_index = None;
        } else {
            self.selected_index = Some(absolute);
        }
    }

    fn open_page_item(&mut self, page_slot: usize) {
        let absolute = self.page_index * PAGE_SIZE + page_slot;
        if absolute >= self.records.len() {
            return;
        }
        self.full_index = Some(absolute);
        self.mark_read(absolute);
    }

    fn delete_selected(&mut self) {
        let Some(index) = self.selected_index else {
            return;
        };
        self.delete_at(index);
    }

    fn delete_current_full(&mut self) {
        let Some(index) = self.full_index else {
            return;
        };
        self.delete_at(index);
    }

    fn delete_all(&mut self) {
        for record in &self.records {
            let _ = fs::remove_file(&record.path);
        }
        self.records.clear();
        self.page_index = 0;
        self.selected_index = None;
        self.full_index = None;
    }

    fn next_page(&mut self) {
        if self.page_index + 1 < self.page_count() {
            self.page_index += 1;
        }
    }

    fn prev_page(&mut self) {
        if self.page_index > 0 {
            self.page_index -= 1;
        }
    }

    fn next_full(&mut self) {
        let Some(index) = self.full_index else {
            return;
        };
        if index + 1 >= self.records.len() {
            return;
        }
        self.full_index = Some(index + 1);
        self.mark_read(index + 1);
    }

    fn prev_full(&mut self) {
        let Some(index) = self.full_index else {
            return;
        };
        if index == 0 {
            return;
        }
        self.full_index = Some(index - 1);
        self.mark_read(index - 1);
    }

    fn selected_on_current_page(&self, page_slot: usize) -> bool {
        self.selected_index == Some(self.page_index * PAGE_SIZE + page_slot)
    }

    fn current_full(&self) -> Option<&CaptureRecord> {
        self.full_index.and_then(|index| self.records.get(index))
    }

    fn delete_at(&mut self, index: usize) {
        if index >= self.records.len() {
            return;
        }

        let removed_path = self.records[index].path.clone();
        let _ = fs::remove_file(&removed_path);
        self.records.remove(index);

        self.selected_index = match self.selected_index {
            Some(selected) if selected == index => None,
            Some(selected) if selected > index => Some(selected - 1),
            other => other,
        };

        self.full_index = match self.full_index {
            Some(_full) if self.records.is_empty() => None,
            Some(full) if full == index => {
                if index < self.records.len() {
                    Some(index)
                } else {
                    Some(self.records.len() - 1)
                }
            }
            Some(full) if full > index => Some(full - 1),
            other => other,
        };

        if self.page_index >= self.page_count() {
            self.page_index = self.page_count().saturating_sub(1);
        }
    }

    fn mark_read(&mut self, index: usize) {
        let Some(record) = self.records.get_mut(index) else {
            return;
        };
        if !record.unread {
            return;
        }

        let Some(file_name) = record.path.file_name().and_then(|value| value.to_str()) else {
            record.unread = false;
            return;
        };
        let Some(stripped) = file_name.strip_prefix(UNREAD_PREFIX) else {
            record.unread = false;
            return;
        };

        let new_path = record
            .path
            .with_file_name(format!("{READ_PREFIX}{stripped}"));
        if fs::rename(&record.path, &new_path).is_ok() {
            record.path = new_path;
        }
        record.unread = false;
    }
}

pub fn configure_capture_record_bindings(app: &AppWindow, capture_dir: String) {
    let state = app.global::<State>();
    let gallery = Arc::new(Mutex::new(CaptureGallery::new(capture_dir)));

    state.on_capture_records_refresh({
        let app_weak = app.as_weak();
        let gallery = gallery.clone();
        move || {
            if let Some(app) = app_weak.upgrade() {
                if let Ok(mut gallery) = gallery.lock() {
                    gallery.refresh();
                    sync_capture_record_state(&app, &gallery);
                }
            }
        }
    });

    state.on_capture_records_open({
        let app_weak = app.as_weak();
        let gallery = gallery.clone();
        move |page_slot| {
            if page_slot < 0 {
                return;
            }
            if let Some(app) = app_weak.upgrade() {
                if let Ok(mut gallery) = gallery.lock() {
                    gallery.open_page_item(page_slot as usize);
                    sync_capture_record_state(&app, &gallery);
                    if gallery.current_full().is_some() {
                        app.global::<State>()
                            .set_current_page(SharedString::from("hEzDU"));
                    }
                }
            }
        }
    });

    state.on_capture_records_select({
        let app_weak = app.as_weak();
        let gallery = gallery.clone();
        move |page_slot| {
            if page_slot < 0 {
                return;
            }
            if let Some(app) = app_weak.upgrade() {
                if let Ok(mut gallery) = gallery.lock() {
                    gallery.select_page_item(page_slot as usize);
                    sync_capture_record_state(&app, &gallery);
                }
            }
        }
    });

    state.on_capture_records_delete_selected({
        let app_weak = app.as_weak();
        let gallery = gallery.clone();
        move || {
            if let Some(app) = app_weak.upgrade() {
                if let Ok(mut gallery) = gallery.lock() {
                    gallery.delete_selected();
                    sync_capture_record_state(&app, &gallery);
                }
            }
        }
    });

    state.on_capture_records_delete_all({
        let app_weak = app.as_weak();
        let gallery = gallery.clone();
        move || {
            if let Some(app) = app_weak.upgrade() {
                if let Ok(mut gallery) = gallery.lock() {
                    gallery.delete_all();
                    sync_capture_record_state(&app, &gallery);
                }
            }
        }
    });

    state.on_capture_records_prev_page({
        let app_weak = app.as_weak();
        let gallery = gallery.clone();
        move || {
            if let Some(app) = app_weak.upgrade() {
                if let Ok(mut gallery) = gallery.lock() {
                    gallery.prev_page();
                    sync_capture_record_state(&app, &gallery);
                }
            }
        }
    });

    state.on_capture_records_next_page({
        let app_weak = app.as_weak();
        let gallery = gallery.clone();
        move || {
            if let Some(app) = app_weak.upgrade() {
                if let Ok(mut gallery) = gallery.lock() {
                    gallery.next_page();
                    sync_capture_record_state(&app, &gallery);
                }
            }
        }
    });

    state.on_capture_records_full_prev({
        let app_weak = app.as_weak();
        let gallery = gallery.clone();
        move || {
            if let Some(app) = app_weak.upgrade() {
                if let Ok(mut gallery) = gallery.lock() {
                    gallery.prev_full();
                    sync_capture_record_state(&app, &gallery);
                }
            }
        }
    });

    state.on_capture_records_full_next({
        let app_weak = app.as_weak();
        let gallery = gallery.clone();
        move || {
            if let Some(app) = app_weak.upgrade() {
                if let Ok(mut gallery) = gallery.lock() {
                    gallery.next_full();
                    sync_capture_record_state(&app, &gallery);
                }
            }
        }
    });

    state.on_capture_records_full_delete({
        let app_weak = app.as_weak();
        let gallery = gallery.clone();
        move || {
            if let Some(app) = app_weak.upgrade() {
                if let Ok(mut gallery) = gallery.lock() {
                    gallery.delete_current_full();
                    sync_capture_record_state(&app, &gallery);
                    if gallery.current_full().is_none() {
                        app.global::<State>()
                            .set_current_page(SharedString::from("NeeP0"));
                    }
                }
            }
        }
    });
}

pub fn clear_capture_record_images(app: &AppWindow) {
    let state = app.global::<State>();
    state.set_capture_record_images(ModelRc::new(VecModel::from(Vec::<Image>::new())));
    state.set_capture_full_image(Image::default());
}

pub fn new_capture_output_path(base_dir: &str, camera_name: &str) -> String {
    let mut safe_name = String::with_capacity(camera_name.len());
    for ch in camera_name.chars() {
        if ch.is_ascii_alphanumeric() {
            safe_name.push(ch);
        } else if !safe_name.ends_with('_') {
            safe_name.push('_');
        }
    }
    if safe_name.trim_matches('_').is_empty() {
        safe_name = "camera".to_string();
    }

    let now = std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .unwrap_or_default()
        .as_secs();
    format!(
        "{}/{}{}__{}.jpg",
        base_dir.trim_end_matches('/'),
        UNREAD_PREFIX,
        safe_name.trim_matches('_'),
        now
    )
}

fn sync_capture_record_state(app: &AppWindow, gallery: &CaptureGallery) {
    let state = app.global::<State>();
    let page_records = gallery.current_page_records();
    let images = page_records
        .iter()
        .map(|record| load_image_or_default(&record.path))
        .collect::<Vec<_>>();
    let date_texts = page_records
        .iter()
        .map(|record| SharedString::from(record.date_text.clone()))
        .collect::<Vec<_>>();
    let time_texts = page_records
        .iter()
        .map(|record| SharedString::from(record.time_text.clone()))
        .collect::<Vec<_>>();
    let labels = page_records
        .iter()
        .map(|record| SharedString::from(record.label.clone()))
        .collect::<Vec<_>>();
    let unread_flags = page_records
        .iter()
        .map(|record| record.unread)
        .collect::<Vec<_>>();
    let selected_flags = page_records
        .iter()
        .enumerate()
        .map(|(index, _)| gallery.selected_on_current_page(index))
        .collect::<Vec<_>>();

    state.set_capture_record_images(ModelRc::new(VecModel::from(images)));
    state.set_capture_record_date_texts(ModelRc::new(VecModel::from(date_texts)));
    state.set_capture_record_time_texts(ModelRc::new(VecModel::from(time_texts)));
    state.set_capture_record_labels(ModelRc::new(VecModel::from(labels)));
    state.set_capture_record_unread_flags(ModelRc::new(VecModel::from(unread_flags)));
    state.set_capture_record_selected_flags(ModelRc::new(VecModel::from(selected_flags)));
    state.set_capture_record_page_text(SharedString::from(format!(
        "{} / {}",
        if gallery.records.is_empty() {
            0
        } else {
            gallery.page_index + 1
        },
        gallery.page_count()
    )));
    state.set_capture_record_has_prev_page(gallery.page_index > 0);
    state.set_capture_record_has_next_page(gallery.page_index + 1 < gallery.page_count());
    state.set_capture_record_has_selection(gallery.selected_index.is_some());

    if let Some(record) = gallery.current_full() {
        state.set_capture_full_image(load_image_or_default(&record.path));
        state.set_capture_full_date_text(SharedString::from(record.date_text.clone()));
        state.set_capture_full_time_text(SharedString::from(record.time_text.clone()));
        state.set_capture_full_label(SharedString::from(record.label.clone()));
        state.set_capture_full_unread(record.unread);
        state.set_capture_full_position_text(SharedString::from(format!(
            "{} / {}",
            gallery.full_index.unwrap_or(0) + 1,
            gallery.records.len()
        )));
        state.set_capture_full_can_prev(gallery.full_index.unwrap_or(0) > 0);
        state.set_capture_full_can_next(
            gallery
                .full_index
                .map(|index| index + 1 < gallery.records.len())
                .unwrap_or(false),
        );
    } else {
        state.set_capture_full_image(Image::default());
        state.set_capture_full_date_text(SharedString::from(""));
        state.set_capture_full_time_text(SharedString::from(""));
        state.set_capture_full_label(SharedString::from(""));
        state.set_capture_full_unread(false);
        state.set_capture_full_position_text(SharedString::from("0 / 0"));
        state.set_capture_full_can_prev(false);
        state.set_capture_full_can_next(false);
    }
}

fn scan_capture_records(capture_dir: &Path) -> Vec<CaptureRecord> {
    let entries = match fs::read_dir(capture_dir) {
        Ok(entries) => entries,
        Err(_) => return Vec::new(),
    };

    let mut records = entries
        .filter_map(|entry| entry.ok())
        .map(|entry| entry.path())
        .filter(|path| is_supported_capture_file(path))
        .filter_map(|path| build_capture_record(path))
        .collect::<Vec<_>>();

    records.sort_by(|left, right| right.timestamp.cmp(&left.timestamp));
    records
}

fn is_supported_capture_file(path: &Path) -> bool {
    path.extension()
        .and_then(|value| value.to_str())
        .map(|value| {
            value.eq_ignore_ascii_case("jpg")
                || value.eq_ignore_ascii_case("jpeg")
                || value.eq_ignore_ascii_case("png")
        })
        .unwrap_or(false)
}

fn build_capture_record(path: PathBuf) -> Option<CaptureRecord> {
    let file_name = path.file_name()?.to_str()?;
    let (unread, label, timestamp) = parse_capture_name(file_name).or_else(|| {
        read_timestamp_from_metadata(&path).map(|timestamp| (false, "CCTV".to_string(), timestamp))
    })?;
    let (date_text, time_text) = format_timestamp(timestamp);

    Some(CaptureRecord {
        path,
        label,
        unread,
        timestamp,
        date_text,
        time_text,
    })
}

fn parse_capture_name(file_name: &str) -> Option<(bool, String, u64)> {
    let stem = file_name.rsplit_once('.')?.0;
    let (unread, remainder) = if let Some(value) = stem.strip_prefix(UNREAD_PREFIX) {
        (true, value)
    } else if let Some(value) = stem.strip_prefix(READ_PREFIX) {
        (false, value)
    } else {
        return None;
    };
    let (label_part, ts_part) = remainder.rsplit_once("__")?;
    let timestamp = ts_part.parse::<u64>().ok()?;
    let label = label_part
        .trim_matches('_')
        .replace('_', " ")
        .trim()
        .to_string();
    Some((
        unread,
        if label.is_empty() {
            "CCTV".to_string()
        } else {
            label
        },
        timestamp,
    ))
}

fn read_timestamp_from_metadata(path: &Path) -> Option<u64> {
    let modified = fs::metadata(path).ok()?.modified().ok()?;
    modified
        .duration_since(std::time::UNIX_EPOCH)
        .ok()
        .map(|duration| duration.as_secs())
}

fn format_timestamp(timestamp: u64) -> (String, String) {
    let mut raw_time = MaybeUninit::<time_t>::new(timestamp as time_t);
    let mut raw_tm = MaybeUninit::<tm>::uninit();

    unsafe {
        if libc::localtime_r(raw_time.as_mut_ptr(), raw_tm.as_mut_ptr()).is_null() {
            return ("----.--.--".to_string(), "--:--:--".to_string());
        }
        let raw_tm = raw_tm.assume_init();
        (
            format!(
                "{:04}-{:02}-{:02}",
                raw_tm.tm_year + 1900,
                raw_tm.tm_mon + 1,
                raw_tm.tm_mday
            ),
            format!(
                "{:02}:{:02}:{:02}",
                raw_tm.tm_hour, raw_tm.tm_min, raw_tm.tm_sec
            ),
        )
    }
}

fn load_image_or_default(path: &Path) -> Image {
    Image::load_from_path(path).unwrap_or_else(|_| Image::default())
}
