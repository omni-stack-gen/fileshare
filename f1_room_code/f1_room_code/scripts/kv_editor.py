#!/usr/bin/env python3
import tkinter as tk
import time
from tkinter import filedialog, messagebox, ttk
from pathlib import Path
from types import SimpleNamespace

import kv_tool


SETTING_COLUMNS = ("key", "type", "value")
WIFI_COLUMNS = ("ssid", "password", "priority", "auto_connect", "last_connected_at")
IPC_COLUMNS = (
    "id",
    "enabled",
    "prefer_substream",
    "name",
    "ip",
    "user",
    "password",
    "rtsp_url",
    "sub_rtsp_url",
)


class KvEditor(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("KR_ROOM KV Editor")
        self.geometry("1180x720")
        self.minsize(920, 560)

        self.root_var = tk.StringVar(
            value=str(kv_tool.storage_root(SimpleNamespace(root=None)))
        )
        self.settings = {}
        self.wifi_profiles = []
        self.ipc_cameras = []

        self._build_layout()
        self.reload_all()

    def _build_layout(self):
        top = ttk.Frame(self, padding=8)
        top.pack(fill=tk.X)

        ttk.Label(top, text="KV root").pack(side=tk.LEFT)
        root_entry = ttk.Entry(top, textvariable=self.root_var)
        root_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=8)
        ttk.Button(top, text="Browse", command=self.browse_root).pack(side=tk.LEFT)
        ttk.Button(top, text="Reload", command=self.reload_all).pack(side=tk.LEFT, padx=(8, 0))
        ttk.Button(top, text="Save All", command=self.save_all).pack(side=tk.LEFT, padx=(8, 0))

        self.notebook = ttk.Notebook(self)
        self.notebook.pack(fill=tk.BOTH, expand=True, padx=8, pady=(0, 8))

        self.settings_tree = self._create_tab(
            "Settings",
            SETTING_COLUMNS,
            [("Add", self.add_setting), ("Edit", self.edit_setting), ("Delete", self.delete_setting)],
        )
        self.wifi_tree = self._create_tab(
            "WiFi",
            WIFI_COLUMNS,
            [("Add", self.add_wifi), ("Edit", self.edit_wifi), ("Delete", self.delete_wifi)],
        )
        self.ipc_tree = self._create_tab(
            "IPC Cameras",
            IPC_COLUMNS,
            [("Add", self.add_ipc), ("Edit", self.edit_ipc), ("Delete", self.delete_ipc)],
        )

        self.status_var = tk.StringVar(value="")
        ttk.Label(self, textvariable=self.status_var, anchor=tk.W, padding=(8, 0)).pack(fill=tk.X)

    def _create_tab(self, title, columns, buttons):
        frame = ttk.Frame(self.notebook, padding=8)
        self.notebook.add(frame, text=title)

        tree = ttk.Treeview(frame, columns=columns, show="headings", selectmode="browse")
        y_scroll = ttk.Scrollbar(frame, orient=tk.VERTICAL, command=tree.yview)
        x_scroll = ttk.Scrollbar(frame, orient=tk.HORIZONTAL, command=tree.xview)
        tree.configure(yscrollcommand=y_scroll.set, xscrollcommand=x_scroll.set)

        for column in columns:
            tree.heading(column, text=column)
            tree.column(column, width=150, minwidth=80, stretch=True)

        tree.grid(row=0, column=0, sticky="nsew")
        y_scroll.grid(row=0, column=1, sticky="ns")
        x_scroll.grid(row=1, column=0, sticky="ew")

        button_bar = ttk.Frame(frame)
        button_bar.grid(row=2, column=0, sticky="ew", pady=(8, 0))
        for text, command in buttons:
            ttk.Button(button_bar, text=text, command=command).pack(side=tk.LEFT, padx=(0, 8))

        frame.rowconfigure(0, weight=1)
        frame.columnconfigure(0, weight=1)
        tree.bind("<Double-1>", lambda _event, tab_title=title: self.edit_current(tab_title))
        return tree

    def root_path(self):
        return Path(self.root_var.get()).expanduser()

    def browse_root(self):
        path = filedialog.askdirectory(initialdir=str(self.root_path()))
        if path:
            self.root_var.set(path)
            self.reload_all()

    def reload_all(self):
        root = self.root_path()
        try:
            self.settings = kv_tool.load_settings(root)
            self.wifi_profiles = kv_tool.load_wifi(root)
            self.ipc_cameras = kv_tool.load_ipc(root)
        except Exception as exc:
            messagebox.showerror("Load failed", str(exc))
            return
        self.refresh_settings()
        self.refresh_wifi()
        self.refresh_ipc()
        self.status_var.set(f"Loaded from {root}")

    def save_all(self):
        root = self.root_path()
        try:
            kv_tool.save_settings(root, self.settings)
            kv_tool.save_wifi(root, self.wifi_profiles)
            kv_tool.save_ipc(root, self.ipc_cameras)
        except Exception as exc:
            messagebox.showerror("Save failed", str(exc))
            return
        self.status_var.set(f"Saved to {root}")

    def refresh_settings(self):
        replace_rows(
            self.settings_tree,
            [(key, value_type, value) for key, (value_type, value) in sorted(self.settings.items())],
        )

    def refresh_wifi(self):
        replace_rows(
            self.wifi_tree,
            [
                (
                    profile["ssid"],
                    profile["password"],
                    profile["priority"],
                    profile["auto_connect"],
                    profile["last_connected_at"],
                )
                for profile in self.wifi_profiles
            ],
        )

    def refresh_ipc(self):
        replace_rows(
            self.ipc_tree,
            [tuple(camera[column] for column in IPC_COLUMNS) for camera in self.ipc_cameras],
        )

    def selected_index(self, tree):
        selected = tree.selection()
        if not selected:
            return None
        return tree.index(selected[0])

    def edit_current(self, title):
        if title == "Settings":
            self.edit_setting()
        elif title == "WiFi":
            self.edit_wifi()
        else:
            self.edit_ipc()

    def add_setting(self):
        values = edit_dialog(
            self,
            "Add Setting",
            [
                Field("key"),
                Field("type", "string", choices=sorted(kv_tool.SETTING_TYPES)),
                Field("value"),
            ],
        )
        if values is None:
            return
        try:
            value = normalize_value(values["type"], values["value"])
        except ValueError as exc:
            messagebox.showerror("Invalid setting", str(exc))
            return
        self.settings[values["key"]] = (values["type"], value)
        self.refresh_settings()

    def edit_setting(self):
        index = self.selected_index(self.settings_tree)
        if index is None:
            return
        key = sorted(self.settings.keys())[index]
        value_type, value = self.settings[key]
        values = edit_dialog(
            self,
            "Edit Setting",
            [
                Field("key", key),
                Field("type", value_type, choices=sorted(kv_tool.SETTING_TYPES)),
                Field("value", value),
            ],
        )
        if values is None:
            return
        try:
            value = normalize_value(values["type"], values["value"])
        except ValueError as exc:
            messagebox.showerror("Invalid setting", str(exc))
            return
        if values["key"] != key:
            self.settings.pop(key, None)
        self.settings[values["key"]] = (values["type"], value)
        self.refresh_settings()

    def delete_setting(self):
        index = self.selected_index(self.settings_tree)
        if index is None:
            return
        key = sorted(self.settings.keys())[index]
        if messagebox.askyesno("Delete setting", f"Delete {key}?"):
            self.settings.pop(key, None)
            self.refresh_settings()

    def add_wifi(self):
        values = edit_dialog(
            self,
            "Add WiFi Profile",
            [
                Field("ssid"),
                Field("password"),
                Field("priority", "0"),
                Field("auto_connect", "true", choices=["true", "false"]),
                Field("last_connected_at", str(int(time.time()))),
            ],
        )
        if values is None:
            return
        try:
            self.wifi_profiles.append(parse_wifi(values))
        except ValueError as exc:
            messagebox.showerror("Invalid WiFi profile", str(exc))
            return
        self.sort_wifi()
        self.refresh_wifi()

    def edit_wifi(self):
        index = self.selected_index(self.wifi_tree)
        if index is None:
            return
        profile = self.wifi_profiles[index]
        values = edit_dialog(
            self,
            "Edit WiFi Profile",
            [
                Field("ssid", profile["ssid"]),
                Field("password", profile["password"]),
                Field("priority", str(profile["priority"])),
                Field("auto_connect", str(profile["auto_connect"]).lower(), choices=["true", "false"]),
                Field("last_connected_at", str(profile["last_connected_at"])),
            ],
        )
        if values is None:
            return
        try:
            self.wifi_profiles[index] = parse_wifi(values)
        except ValueError as exc:
            messagebox.showerror("Invalid WiFi profile", str(exc))
            return
        self.sort_wifi()
        self.refresh_wifi()

    def delete_wifi(self):
        index = self.selected_index(self.wifi_tree)
        if index is None:
            return
        ssid = self.wifi_profiles[index]["ssid"]
        if messagebox.askyesno("Delete WiFi profile", f"Delete {ssid}?"):
            del self.wifi_profiles[index]
            self.refresh_wifi()

    def sort_wifi(self):
        self.wifi_profiles.sort(
            key=lambda item: (-item["priority"], -(item["last_connected_at"] or 0), item["ssid"])
        )

    def add_ipc(self):
        values = edit_dialog(
            self,
            "Add IPC Camera",
            [
                Field("id", str(len(self.ipc_cameras) + 1)),
                Field("enabled", "true", choices=["true", "false"]),
                Field("prefer_substream", "true", choices=["true", "false"]),
                Field("name", f"Camera {len(self.ipc_cameras) + 1}"),
                Field("ip"),
                Field("user"),
                Field("password"),
                Field("rtsp_url"),
                Field("sub_rtsp_url"),
            ],
        )
        if values is None:
            return
        try:
            self.ipc_cameras.append(parse_ipc(values))
        except ValueError as exc:
            messagebox.showerror("Invalid IPC camera", str(exc))
            return
        self.refresh_ipc()

    def edit_ipc(self):
        index = self.selected_index(self.ipc_tree)
        if index is None:
            return
        camera = self.ipc_cameras[index]
        values = edit_dialog(
            self,
            "Edit IPC Camera",
            [
                Field("id", camera["id"]),
                Field("enabled", str(camera["enabled"]).lower(), choices=["true", "false"]),
                Field(
                    "prefer_substream",
                    str(camera["prefer_substream"]).lower(),
                    choices=["true", "false"],
                ),
                Field("name", camera["name"]),
                Field("ip", camera["ip"]),
                Field("user", camera["user"]),
                Field("password", camera["password"]),
                Field("rtsp_url", camera["rtsp_url"]),
                Field("sub_rtsp_url", camera["sub_rtsp_url"]),
            ],
        )
        if values is None:
            return
        try:
            self.ipc_cameras[index] = parse_ipc(values)
        except ValueError as exc:
            messagebox.showerror("Invalid IPC camera", str(exc))
            return
        self.refresh_ipc()

    def delete_ipc(self):
        index = self.selected_index(self.ipc_tree)
        if index is None:
            return
        name = self.ipc_cameras[index]["name"]
        if messagebox.askyesno("Delete IPC camera", f"Delete {name}?"):
            del self.ipc_cameras[index]
            self.refresh_ipc()


class Field:
    def __init__(self, name, value="", choices=None):
        self.name = name
        self.value = value
        self.choices = choices


def replace_rows(tree, rows):
    for item in tree.get_children():
        tree.delete(item)
    for row in rows:
        tree.insert("", tk.END, values=row)


def edit_dialog(parent, title, fields):
    dialog = tk.Toplevel(parent)
    dialog.title(title)
    dialog.transient(parent)
    dialog.grab_set()
    dialog.resizable(True, False)

    vars_by_name = {}
    for row, field in enumerate(fields):
        ttk.Label(dialog, text=field.name).grid(row=row, column=0, sticky=tk.W, padx=8, pady=4)
        var = tk.StringVar(value=field.value)
        vars_by_name[field.name] = var
        if field.choices:
            editor = ttk.Combobox(dialog, textvariable=var, values=field.choices, state="readonly")
        else:
            editor = ttk.Entry(dialog, textvariable=var, width=72)
        editor.grid(row=row, column=1, sticky="ew", padx=8, pady=4)

    result = {"values": None}

    def save():
        result["values"] = {name: var.get() for name, var in vars_by_name.items()}
        dialog.destroy()

    def cancel():
        dialog.destroy()

    buttons = ttk.Frame(dialog)
    buttons.grid(row=len(fields), column=0, columnspan=2, sticky=tk.E, padx=8, pady=8)
    ttk.Button(buttons, text="Cancel", command=cancel).pack(side=tk.RIGHT)
    ttk.Button(buttons, text="OK", command=save).pack(side=tk.RIGHT, padx=(0, 8))
    dialog.columnconfigure(1, weight=1)
    dialog.bind("<Return>", lambda _event: save())
    dialog.bind("<Escape>", lambda _event: cancel())
    dialog.wait_window()
    return result["values"]


def normalize_value(value_type, value):
    return kv_tool.bool_text(value) if value_type == "bool" else value


def parse_wifi(values):
    return {
        "ssid": values["ssid"],
        "password": values["password"],
        "priority": parse_int(values["priority"], "priority"),
        "auto_connect": kv_tool.bool_display(kv_tool.bool_text(values["auto_connect"])),
        "last_connected_at": parse_int(values["last_connected_at"], "last_connected_at"),
    }


def parse_ipc(values):
    return {
        "id": values["id"],
        "enabled": kv_tool.bool_display(kv_tool.bool_text(values["enabled"])),
        "prefer_substream": kv_tool.bool_display(kv_tool.bool_text(values["prefer_substream"])),
        "name": values["name"],
        "ip": values["ip"],
        "user": values["user"],
        "password": values["password"],
        "rtsp_url": values["rtsp_url"],
        "sub_rtsp_url": values["sub_rtsp_url"],
    }


def parse_int(value, name):
    try:
        return int(value)
    except ValueError as exc:
        raise ValueError(f"{name} must be an integer") from exc


if __name__ == "__main__":
    KvEditor().mainloop()
