#!/usr/bin/env python3
"""Reserve and access the Jenkins x86+usrp0 bench without exposing its password."""

from __future__ import annotations

import argparse
import base64
import json
import os
from pathlib import Path
import ssl
import subprocess
import sys
import urllib.error
import urllib.parse
import urllib.request


RESOURCE = "x86+usrp0"
JENKINS = "https://zeld-lpor-ci.elvees.com"
TOKEN_FILE = Path.home() / "jenkins.token"
KEY_FILE = Path.home() / ".ssh" / "id_ed25519_x86_usrp0"
USERS = ("nmotin", "developer")


def die(message: str) -> "None":
    raise SystemExit(message)


def token() -> str:
    try:
        value = TOKEN_FILE.read_text().strip()
    except OSError as exc:
        die(f"cannot read {TOKEN_FILE}: {exc}")
    if not value:
        die(f"empty Jenkins token: {TOKEN_FILE}")
    return value


def request(path: str, *, method: str = "GET") -> tuple[str, bytes]:
    secret = token()
    context = ssl._create_unverified_context()
    last_error: Exception | None = None
    for user in USERS:
        auth = base64.b64encode(f"{user}:{secret}".encode()).decode()
        req = urllib.request.Request(
            JENKINS + path,
            data=b"" if method == "POST" else None,
            method=method,
            headers={"Authorization": "Basic " + auth},
        )
        try:
            with urllib.request.urlopen(req, context=context, timeout=30) as response:
                return user, response.read()
        except urllib.error.HTTPError as exc:
            last_error = exc
            if exc.code not in (401, 403):
                raise
    die(f"Jenkins authentication failed for {USERS}: {last_error}")


def resource() -> tuple[str, dict]:
    user, body = request("/lockable-resources/api/json?depth=2")
    data = json.loads(body)
    try:
        item = next(x for x in data.get("resources", []) if x.get("name") == RESOURCE)
    except StopIteration:
        die(f"Jenkins resource not found: {RESOURCE}")
    return user, item


def properties(item: dict) -> dict[str, str]:
    result = {}
    for prop in item.get("properties", []) or []:
        name = prop.get("name") or prop.get("key")
        if name:
            result[name] = prop.get("value")
    return result


def connection(*, include_password: bool = False) -> tuple[str, str] | tuple[str, str, str]:
    _, item = resource()
    props = properties(item)
    required = ["user", "hostname"] + (["password"] if include_password else [])
    missing = [name for name in required if not props.get(name)]
    if missing:
        die("missing Jenkins Properties: " + ", ".join(missing))
    if include_password:
        return props["user"], props["hostname"], props["password"]
    return props["user"], props["hostname"]


def print_status() -> None:
    jenkins_user, item = resource()
    props = properties(item)
    safe = {
        "resource": RESOURCE,
        "jenkins_user": jenkins_user,
        "reserved": bool(item.get("reserved")),
        "locked": bool(item.get("locked")),
        "reservedBy": item.get("reservedBy"),
        "hostname": props.get("hostname"),
        "user": props.get("user"),
        "property_keys": sorted(props),
    }
    print(json.dumps(safe, ensure_ascii=False, indent=2))


def change_reservation(action: str) -> None:
    quoted = urllib.parse.quote(RESOURCE, safe="")
    request(f"/lockable-resources/{action}?resource={quoted}", method="POST")
    _, item = resource()
    expected = action == "reserve"
    actual = bool(item.get("reserved"))
    print(
        f"resource={RESOURCE} reserved={actual} locked={bool(item.get('locked'))} "
        f"reservedBy={item.get('reservedBy')}"
    )
    if actual != expected:
        die(f"Jenkins did not apply {action}")


def require_our_reservation() -> None:
    _, item = resource()
    owner = item.get("reservedBy") or ""
    if not item.get("reserved") or "Мотин" not in owner:
        die(f"refusing bench access: reserved={item.get('reserved')} reservedBy={owner or None}")


def ssh_base() -> tuple[list[str], str]:
    user, host = connection()
    return [
        "ssh",
        "-i",
        str(KEY_FILE),
        "-o",
        "BatchMode=yes",
        "-o",
        "StrictHostKeyChecking=accept-new",
        f"{user}@{host}",
    ], f"{user}@{host}"


def install_key() -> None:
    require_our_reservation()
    user, host = connection()
    KEY_FILE.parent.mkdir(mode=0o700, parents=True, exist_ok=True)
    if not KEY_FILE.exists():
        subprocess.run(
            ["ssh-keygen", "-q", "-t", "ed25519", "-N", "", "-f", str(KEY_FILE),
             "-C", "zeld-lpor-nmotin-to-x86-usrp0"],
            check=True,
        )
    existing = subprocess.run(
        ["ssh", "-i", str(KEY_FILE), "-o", "BatchMode=yes", "-o",
         "StrictHostKeyChecking=accept-new", f"{user}@{host}", "true"],
        stdin=subprocess.DEVNULL,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    if existing.returncode == 0:
        command, target = ssh_base()
        subprocess.run(command + ["printf 'key_login_ok host='; hostname"], check=True)
        print(f"key={KEY_FILE} target={target} already_installed=true")
        return
    env = os.environ.copy()
    env.update(
        DISPLAY=":0",
        SSH_ASKPASS_REQUIRE="force",
        SSH_ASKPASS=str(Path(__file__).resolve()),
        X86_USRP0_ASKPASS="1",
    )
    subprocess.run(
        ["ssh-copy-id", "-f", "-i", str(KEY_FILE) + ".pub",
         "-o", "StrictHostKeyChecking=accept-new", f"{user}@{host}"],
        stdin=subprocess.DEVNULL,
        env=env,
        start_new_session=True,
        check=True,
    )
    command, target = ssh_base()
    subprocess.run(command + ["printf 'key_login_ok host='; hostname"], check=True)
    print(f"key={KEY_FILE} target={target}")


def run_ssh(arguments: list[str]) -> None:
    require_our_reservation()
    if not KEY_FILE.exists():
        die(f"key not found; run install-key: {KEY_FILE}")
    command, _ = ssh_base()
    if arguments and arguments[0] == "--":
        arguments = arguments[1:]
    os.execvp(command[0], command + arguments)


def probe() -> None:
    require_our_reservation()
    command, target = ssh_base()
    print(f"target={target}")
    remote = (
        "set -u; printf 'host='; hostname; "
        "echo network:; ip -br addr; "
        "echo usrp:; timeout 30 uhd_find_devices"
    )
    subprocess.run(command + [remote], check=True)


def main() -> None:
    if os.environ.get("X86_USRP0_ASKPASS") == "1":
        _, _, password = connection(include_password=True)
        sys.stdout.write(password)
        return

    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="action", required=True)
    sub.add_parser("status")
    sub.add_parser("reserve")
    sub.add_parser("unreserve")
    sub.add_parser("install-key")
    sub.add_parser("probe")
    ssh_parser = sub.add_parser("ssh")
    ssh_parser.add_argument("arguments", nargs=argparse.REMAINDER)
    args = parser.parse_args()

    if args.action == "status":
        print_status()
    elif args.action in ("reserve", "unreserve"):
        change_reservation(args.action)
    elif args.action == "install-key":
        install_key()
    elif args.action == "probe":
        probe()
    elif args.action == "ssh":
        run_ssh(args.arguments)


if __name__ == "__main__":
    main()
