#!/usr/bin/env python3
import argparse
import base64
import os
import sys
import time
from pathlib import Path


SETTING_TYPES = {"bool", "int", "float", "string"}


def storage_root(args):
    if args.root:
        return Path(args.root)

    env_path = os.environ.get("KR_ROOM_DB_PATH", "")
    if env_path:
        path = Path(env_path)
        if path.suffix:
            return path.parent if str(path.parent) else Path(".")
        return path

    if Path("/data").exists():
        return Path("/data/kr_room")
    return Path(".")


def encode_field(value):
    return base64.b64encode(value.encode("utf-8")).decode("ascii")


def decode_field(value, path, line_no):
    try:
        return base64.b64decode(value.encode("ascii")).decode("utf-8")
    except Exception as exc:
        raise ValueError(f"{path}:{line_no}: invalid base64/utf8 field: {exc}") from exc


def read_records(path, expected_fields):
    if not path.exists():
        return []

    records = []
    with path.open("r", encoding="utf-8") as file:
        for line_no, raw_line in enumerate(file, 1):
            line = raw_line.rstrip("\n").rstrip("\r")
            if not line or line.startswith("#"):
                continue
            parts = line.split("\t")
            if len(parts) != expected_fields:
                raise ValueError(
                    f"{path}:{line_no}: expected {expected_fields} tab-separated fields, "
                    f"got {len(parts)}"
                )
            records.append((line_no, parts))
    return records


def write_atomic(path, content):
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp_path = path.with_suffix(".tmp")
    with tmp_path.open("w", encoding="utf-8", newline="\n") as file:
        file.write(content)
        file.flush()
        os.fsync(file.fileno())
    os.replace(tmp_path, path)

    try:
        dir_fd = os.open(path.parent, os.O_RDONLY)
    except OSError:
        return
    try:
        os.fsync(dir_fd)
    finally:
        os.close(dir_fd)


def bool_text(value):
    if isinstance(value, bool):
        return "1" if value else "0"
    lowered = str(value).strip().lower()
    if lowered in {"1", "true", "yes", "on", "enabled"}:
        return "1"
    if lowered in {"0", "false", "no", "off", "disabled"}:
        return "0"
    raise ValueError(f"invalid bool value: {value}")


def bool_display(value):
    return value == "1" or value.lower() == "true"


def settings_path(root):
    return root / "settings.kv"


def wifi_path(root):
    return root / "wifi_profiles.kv"


def ipc_path(root):
    return root / "ipc_cameras.kv"


def load_settings(root):
    settings = {}
    for line_no, parts in read_records(settings_path(root), 3):
        key, value_type, encoded_value = parts
        if value_type not in SETTING_TYPES:
            raise ValueError(f"{settings_path(root)}:{line_no}: unsupported type: {value_type}")
        settings[key] = (value_type, decode_field(encoded_value, settings_path(root), line_no))
    return settings


def save_settings(root, settings):
    lines = []
    for key in sorted(settings):
        value_type, value = settings[key]
        lines.append(f"{key}\t{value_type}\t{encode_field(value)}")
    write_atomic(settings_path(root), "\n".join(lines) + ("\n" if lines else ""))


def cmd_settings_list(args):
    root = storage_root(args)
    settings = load_settings(root)
    for key in sorted(settings):
        value_type, value = settings[key]
        print(f"{key}\t{value_type}\t{value}")


def cmd_settings_get(args):
    root = storage_root(args)
    settings = load_settings(root)
    if args.key not in settings:
        raise SystemExit(f"setting not found: {args.key}")
    value_type, value = settings[args.key]
    print(f"{args.key}\t{value_type}\t{value}")


def cmd_settings_set(args):
    if args.value_type not in SETTING_TYPES:
        raise SystemExit(f"unsupported setting type: {args.value_type}")
    root = storage_root(args)
    settings = load_settings(root)
    value = bool_text(args.value) if args.value_type == "bool" else args.value
    settings[args.key] = (args.value_type, value)
    save_settings(root, settings)
    print(f"saved {args.key}")


def cmd_settings_delete(args):
    root = storage_root(args)
    settings = load_settings(root)
    if settings.pop(args.key, None) is None:
        raise SystemExit(f"setting not found: {args.key}")
    save_settings(root, settings)
    print(f"deleted {args.key}")


def load_wifi(root):
    profiles = []
    for line_no, parts in read_records(wifi_path(root), 5):
        profiles.append(
            {
                "ssid": decode_field(parts[0], wifi_path(root), line_no),
                "password": decode_field(parts[1], wifi_path(root), line_no),
                "priority": int(parts[2]) if parts[2] else 0,
                "auto_connect": bool_display(parts[3]),
                "last_connected_at": int(parts[4]) if parts[4] else 0,
            }
        )
    return profiles


def save_wifi(root, profiles):
    lines = []
    for profile in profiles:
        lines.append(
            "\t".join(
                [
                    encode_field(profile["ssid"]),
                    encode_field(profile["password"]),
                    str(profile["priority"]),
                    bool_text(profile["auto_connect"]),
                    str(profile["last_connected_at"] or 0),
                ]
            )
        )
    write_atomic(wifi_path(root), "\n".join(lines) + ("\n" if lines else ""))


def cmd_wifi_list(args):
    root = storage_root(args)
    for index, profile in enumerate(load_wifi(root), 1):
        print(
            f"{index}\tssid={profile['ssid']}\tpriority={profile['priority']}"
            f"\tauto_connect={profile['auto_connect']}\tlast_connected_at={profile['last_connected_at']}"
        )


def cmd_wifi_set(args):
    root = storage_root(args)
    profiles = load_wifi(root)
    profile = next((profile for profile in profiles if profile["ssid"] == args.ssid), None)
    if profile is None:
        profile = {
            "ssid": args.ssid,
            "password": "",
            "priority": 0,
            "auto_connect": True,
            "last_connected_at": int(time.time()),
        }
        profiles.append(profile)

    if args.password is not None:
        profile["password"] = args.password
    if args.priority is not None:
        profile["priority"] = args.priority
    if args.auto_connect is not None:
        profile["auto_connect"] = bool_display(bool_text(args.auto_connect))
    if args.last_connected_at is not None:
        profile["last_connected_at"] = args.last_connected_at

    profiles.sort(
        key=lambda item: (-item["priority"], -(item["last_connected_at"] or 0), item["ssid"])
    )
    save_wifi(root, profiles)
    print(f"saved wifi profile {args.ssid}")


def cmd_wifi_delete(args):
    root = storage_root(args)
    profiles = load_wifi(root)
    kept = [profile for profile in profiles if profile["ssid"] != args.ssid]
    if len(kept) == len(profiles):
        raise SystemExit(f"wifi profile not found: {args.ssid}")
    save_wifi(root, kept)
    print(f"deleted wifi profile {args.ssid}")


def load_ipc(root):
    cameras = []
    for line_no, parts in read_records(ipc_path(root), 9):
        cameras.append(
            {
                "id": parts[0],
                "enabled": bool_display(parts[1]),
                "prefer_substream": bool_display(parts[2]),
                "name": decode_field(parts[3], ipc_path(root), line_no),
                "ip": decode_field(parts[4], ipc_path(root), line_no),
                "user": decode_field(parts[5], ipc_path(root), line_no),
                "password": decode_field(parts[6], ipc_path(root), line_no),
                "rtsp_url": decode_field(parts[7], ipc_path(root), line_no),
                "sub_rtsp_url": decode_field(parts[8], ipc_path(root), line_no),
            }
        )
    return cameras


def save_ipc(root, cameras):
    lines = []
    for index, camera in enumerate(cameras, 1):
        camera_id = camera.get("id") or str(index)
        lines.append(
            "\t".join(
                [
                    str(camera_id),
                    bool_text(camera["enabled"]),
                    bool_text(camera["prefer_substream"]),
                    encode_field(camera["name"]),
                    encode_field(camera["ip"]),
                    encode_field(camera["user"]),
                    encode_field(camera["password"]),
                    encode_field(camera["rtsp_url"]),
                    encode_field(camera["sub_rtsp_url"]),
                ]
            )
        )
    write_atomic(ipc_path(root), "\n".join(lines) + ("\n" if lines else ""))


def cmd_ipc_list(args):
    root = storage_root(args)
    for index, camera in enumerate(load_ipc(root), 1):
        print(
            f"{index}\tid={camera['id']}\tenabled={camera['enabled']}"
            f"\tprefer_substream={camera['prefer_substream']}\tname={camera['name']}"
            f"\tip={camera['ip']}\tuser={camera['user']}"
            f"\trtsp_url={camera['rtsp_url']}\tsub_rtsp_url={camera['sub_rtsp_url']}"
        )


def apply_ipc_args(camera, args):
    for key in [
        "id",
        "name",
        "ip",
        "user",
        "password",
        "rtsp_url",
        "sub_rtsp_url",
    ]:
        value = getattr(args, key)
        if value is not None:
            camera[key] = value
    if args.enabled is not None:
        camera["enabled"] = bool_display(bool_text(args.enabled))
    if args.prefer_substream is not None:
        camera["prefer_substream"] = bool_display(bool_text(args.prefer_substream))


def cmd_ipc_add(args):
    root = storage_root(args)
    cameras = load_ipc(root)
    camera = {
        "id": args.id or str(len(cameras) + 1),
        "enabled": bool_display(bool_text(args.enabled)),
        "prefer_substream": bool_display(bool_text(args.prefer_substream)),
        "name": args.name,
        "ip": args.ip,
        "user": args.user,
        "password": args.password,
        "rtsp_url": args.rtsp_url,
        "sub_rtsp_url": args.sub_rtsp_url,
    }
    cameras.append(camera)
    save_ipc(root, cameras)
    print(f"added ipc camera {len(cameras)}")


def cmd_ipc_update(args):
    root = storage_root(args)
    cameras = load_ipc(root)
    if args.index < 1 or args.index > len(cameras):
        raise SystemExit(f"camera index out of range: {args.index}")
    apply_ipc_args(cameras[args.index - 1], args)
    save_ipc(root, cameras)
    print(f"updated ipc camera {args.index}")


def cmd_ipc_delete(args):
    root = storage_root(args)
    cameras = load_ipc(root)
    if args.index < 1 or args.index > len(cameras):
        raise SystemExit(f"camera index out of range: {args.index}")
    del cameras[args.index - 1]
    save_ipc(root, cameras)
    print(f"deleted ipc camera {args.index}")


def add_root_arg(parser):
    parser.add_argument(
        "--root",
        help="KV root directory. Defaults to KR_ROOM_DB_PATH, /data/kr_room, then current directory.",
    )


def add_ipc_fields(parser, require_name=False):
    parser.add_argument("--id")
    parser.add_argument("--enabled", default=None if not require_name else "1")
    parser.add_argument("--prefer-substream", default=None if not require_name else "1")
    parser.add_argument("--name", required=require_name)
    parser.add_argument("--ip", default=None if not require_name else "")
    parser.add_argument("--user", default=None if not require_name else "")
    parser.add_argument("--password", default=None if not require_name else "")
    parser.add_argument("--rtsp-url", default=None if not require_name else "")
    parser.add_argument("--sub-rtsp-url", default=None if not require_name else "")


def build_parser():
    parser = argparse.ArgumentParser(description="Read and edit KR_ROOM KV storage files.")
    add_root_arg(parser)
    subparsers = parser.add_subparsers(dest="section", required=True)

    settings = subparsers.add_parser("settings", help="Edit settings.kv")
    settings_sub = settings.add_subparsers(dest="action", required=True)
    settings_list = settings_sub.add_parser("list")
    settings_list.set_defaults(func=cmd_settings_list)
    settings_get = settings_sub.add_parser("get")
    settings_get.add_argument("key")
    settings_get.set_defaults(func=cmd_settings_get)
    settings_set = settings_sub.add_parser("set")
    settings_set.add_argument("key")
    settings_set.add_argument("value_type", choices=sorted(SETTING_TYPES))
    settings_set.add_argument("value")
    settings_set.set_defaults(func=cmd_settings_set)
    settings_delete = settings_sub.add_parser("delete")
    settings_delete.add_argument("key")
    settings_delete.set_defaults(func=cmd_settings_delete)

    wifi = subparsers.add_parser("wifi", help="Edit wifi_profiles.kv")
    wifi_sub = wifi.add_subparsers(dest="action", required=True)
    wifi_list = wifi_sub.add_parser("list")
    wifi_list.set_defaults(func=cmd_wifi_list)
    wifi_set = wifi_sub.add_parser("set")
    wifi_set.add_argument("ssid")
    wifi_set.add_argument("--password")
    wifi_set.add_argument("--priority", type=int)
    wifi_set.add_argument("--auto-connect")
    wifi_set.add_argument("--last-connected-at", type=int)
    wifi_set.set_defaults(func=cmd_wifi_set)
    wifi_delete = wifi_sub.add_parser("delete")
    wifi_delete.add_argument("ssid")
    wifi_delete.set_defaults(func=cmd_wifi_delete)

    ipc = subparsers.add_parser("ipc", help="Edit ipc_cameras.kv")
    ipc_sub = ipc.add_subparsers(dest="action", required=True)
    ipc_list = ipc_sub.add_parser("list")
    ipc_list.set_defaults(func=cmd_ipc_list)
    ipc_add = ipc_sub.add_parser("add")
    add_ipc_fields(ipc_add, require_name=True)
    ipc_add.set_defaults(func=cmd_ipc_add)
    ipc_update = ipc_sub.add_parser("update")
    ipc_update.add_argument("index", type=int)
    add_ipc_fields(ipc_update, require_name=False)
    ipc_update.set_defaults(func=cmd_ipc_update)
    ipc_delete = ipc_sub.add_parser("delete")
    ipc_delete.add_argument("index", type=int)
    ipc_delete.set_defaults(func=cmd_ipc_delete)

    return parser


def main():
    parser = build_parser()
    args = parser.parse_args()
    try:
        args.func(args)
    except ValueError as exc:
        raise SystemExit(str(exc)) from exc


if __name__ == "__main__":
    main()
