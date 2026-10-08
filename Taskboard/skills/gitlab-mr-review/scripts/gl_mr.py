#!/usr/bin/env python3
"""GitLab MR: сбор контекста и постинг ревью отдельными тредами.

Запускать НА хосте с доступом к git.elvees.com и токеном (~/git.token):

    scp scripts/gl_mr.py supermarket-wm:/tmp/
    ssh supermarket-wm "python3 /tmp/gl_mr.py info dis/sunrise/sunrise-platform/runtime 123"

PROJECT — путь ("dis/sunrise/sunrise-platform/runtime", кодируется сам) или числовой id.

Команды:
    info      PROJECT IID                 — заголовок, ветки, автор, diff_refs, описание
    files     PROJECT IID                 — список изменённых файлов (+ признак overflow)
    diff      PROJECT IID [PATH]          — диффы файлов из MR
    threads   PROJECT IID                 — существующие треды (кто, где, resolved)
    post      PROJECT IID FINDINGS.json   — по треду на замечание, привязка к строке
    mark      PROJECT IID STATE           — отметить своё ревью: changes | approve | unapprove
    handoff   PROJECT IID                 — вернуть MR автору: снять себя с assignee, добавить автора
    note      PROJECT IID BODY            — общий комментарий к MR (без привязки)
    reply     PROJECT IID DISC_ID BODY    — ответ в существующий тред
    resolve   PROJECT IID DISC_ID...      — закрыть треды

FINDINGS.json — список объектов:
    [{"path": "src/a.cpp", "line": 42, "body": "**Заголовок.**\\n\\nТекст..."},
     {"path": "src/b.cpp", "line": 17, "side": "old", "body": "..."}]
`side` — "new" (по умолчанию) или "old" (замечание к удалённой строке).
У post есть --dry-run: печатает вычисленные позиции, ничего не постит.
"""

import argparse
import json
import os
import re
import sys
import urllib.error
import urllib.parse
import urllib.request

API = "https://git.elvees.com/api/v4"
HUNK_RE = re.compile(r"^@@ -(\d+)(?:,\d+)? \+(\d+)(?:,\d+)? @@")


def token():
    tok = os.environ.get("GITLAB_TOKEN")
    if tok:
        return tok.strip()
    for path in (os.path.expanduser("~/git.token"), "/home/nmotin/git.token"):
        if os.path.exists(path):
            with open(path) as f:
                return f.read().strip()
    sys.exit("нет токена: ни $GITLAB_TOKEN, ни ~/git.token")


def call(method, path, payload=None):
    url = f"{API}/{path}"
    data = None
    headers = {"PRIVATE-TOKEN": token()}
    if payload is not None:
        data = json.dumps(payload).encode()
        headers["Content-Type"] = "application/json"
    req = urllib.request.Request(url, data=data, headers=headers, method=method)
    try:
        with urllib.request.urlopen(req) as resp:
            return resp.status, json.loads(resp.read().decode() or "null")
    except urllib.error.HTTPError as e:
        body = e.read().decode()
        try:
            body = json.loads(body)
        except ValueError:
            pass
        return e.code, body


def mr_path(project, iid, tail=""):
    pid = project if str(project).isdigit() else urllib.parse.quote(project, safe="")
    return f"projects/{pid}/merge_requests/{iid}{tail}"


def get_changes(project, iid):
    # access_raw_diffs=true — иначе GitLab отдаёт "collapsed" файлы с пустым diff
    status, data = call("GET", mr_path(project, iid, "/changes?access_raw_diffs=true"))
    if status != 200:
        sys.exit(f"GET changes -> {status}: {data}")
    return data


def locate(diff_text, target, side="new"):
    """Классифицировать строку в диффе файла.

    Возвращает {"type", "old_line", "new_line"} либо None, если строка вне дифф-хунков
    (GitLab отвергает такие позиции с 400 "line_code must be a valid line code").
    """
    old_line = new_line = None
    for raw in diff_text.split("\n"):
        m = HUNK_RE.match(raw)
        if m:
            old_line, new_line = int(m.group(1)), int(m.group(2))
            continue
        if old_line is None:
            continue
        if raw.startswith("\\"):
            continue
        if raw.startswith("+"):
            if side == "new" and new_line == target:
                return {"type": "added", "old_line": None, "new_line": new_line}
            new_line += 1
        elif raw.startswith("-"):
            if side == "old" and old_line == target:
                return {"type": "removed", "old_line": old_line, "new_line": None}
            old_line += 1
        else:
            cur = new_line if side == "new" else old_line
            if cur == target:
                # Context-строка: GitLab требует ОБА номера, иначе 400.
                return {"type": "context", "old_line": old_line, "new_line": new_line}
            old_line += 1
            new_line += 1
    return None


def nearby(diff_text, target, side="new", span=12):
    """Подсказка: какие строки рядом реально есть в диффе."""
    found = []
    old_line = new_line = None
    for raw in diff_text.split("\n"):
        m = HUNK_RE.match(raw)
        if m:
            old_line, new_line = int(m.group(1)), int(m.group(2))
            continue
        if old_line is None or raw.startswith("\\"):
            continue
        if raw.startswith("+"):
            cur, kind = new_line, "+"
            new_line += 1
        elif raw.startswith("-"):
            cur, kind = old_line, "-"
            old_line += 1
        else:
            cur, kind = (new_line if side == "new" else old_line), " "
            old_line += 1
            new_line += 1
        if abs(cur - target) <= span:
            found.append(f"{kind}{cur}")
    return " ".join(found)


def cmd_info(a):
    status, d = call("GET", mr_path(a.project, a.iid))
    if status != 200:
        sys.exit(f"{status}: {d}")
    for k in ("iid", "title", "state", "draft", "source_branch", "target_branch",
              "web_url", "sha", "has_conflicts", "detailed_merge_status",
              "blocking_discussions_resolved"):
        print(f"{k}: {d.get(k)}")
    print("author:", (d.get("author") or {}).get("username"))
    print("reviewers:", [r["username"] for r in d.get("reviewers") or []])
    print("diff_refs:", json.dumps(d.get("diff_refs"), indent=2))
    print("--- description ---")
    print(d.get("description") or "(пусто)")


def cmd_files(a):
    d = get_changes(a.project, a.iid)
    changes = d.get("changes", [])
    print(f"files: {len(changes)}  overflow: {d.get('overflow')}")
    if d.get("overflow"):
        print("!! дифф обрезан API — брать через git diff base..head в клоне репозитория")
    for c in changes:
        flags = "".join(k[0].upper() for k in ("new_file", "deleted_file", "renamed_file") if c.get(k))
        print(f"  {c['old_path']} -> {c['new_path']} {flags}")


def cmd_diff(a):
    d = get_changes(a.project, a.iid)
    for c in d.get("changes", []):
        if a.path and a.path not in c["new_path"]:
            continue
        print(f"=== {c['old_path']} -> {c['new_path']}")
        print(c["diff"])


def cmd_threads(a):
    out = []
    page = 1
    while True:
        status, d = call("GET", mr_path(a.project, a.iid, f"/discussions?per_page=100&page={page}"))
        if status != 200:
            sys.exit(f"{status}: {d}")
        out.extend(d)
        if len(d) < 100:
            break
        page += 1
    for disc in out:
        n0 = disc["notes"][0]
        if n0.get("system"):
            continue
        pos = n0.get("position") or {}
        loc = f"{pos.get('new_path')}:{pos.get('new_line') or pos.get('old_line')}" if pos else "(общий)"
        print(f"[{disc['id']}] resolved={n0.get('resolved')} {loc}")
        for n in disc["notes"]:
            if n.get("system"):
                continue
            print(f"    {n['author']['username']}: {n['body'][:300].splitlines()[0] if n['body'] else ''}")


def cmd_post(a):
    findings = json.load(open(a.findings))
    status, mr = call("GET", mr_path(a.project, a.iid))
    if status != 200:
        sys.exit(f"{status}: {mr}")
    refs = mr["diff_refs"]
    changes = {c["new_path"]: c for c in get_changes(a.project, a.iid).get("changes", [])}

    prepared, bad = [], []
    for f in findings:
        c = changes.get(f["path"])
        if c is None:
            bad.append((f, "файла нет в изменениях MR"))
            continue
        side = f.get("side", "new")
        loc = locate(c["diff"], f["line"], side)
        if loc is None:
            bad.append((f, "строка вне дифф-хунков; рядом в диффе: "
                        + nearby(c["diff"], f["line"], side)))
            continue
        pos = {
            "base_sha": refs["base_sha"],
            "start_sha": refs["start_sha"],
            "head_sha": refs["head_sha"],
            "position_type": "text",
            "old_path": c["old_path"],
            "new_path": c["new_path"],
        }
        if loc["old_line"] is not None:
            pos["old_line"] = loc["old_line"]
        if loc["new_line"] is not None:
            pos["new_line"] = loc["new_line"]
        prepared.append((f, pos, loc["type"]))

    for f, why in bad:
        print(f"SKIP {f['path']}:{f['line']} — {why}")
    for f, pos, kind in prepared:
        print(f"{'DRY ' if a.dry_run else ''}{f['path']}:{f['line']} [{kind}] "
              f"old={pos.get('old_line')} new={pos.get('new_line')}")
        if a.dry_run:
            continue
        status, resp = call("POST", mr_path(a.project, a.iid, "/discussions"),
                            {"body": f["body"], "position": pos})
        if status == 201:
            print(f"    OK {resp['id']}")
        else:
            print(f"    FAIL {status}: {resp}")
    if bad and not a.dry_run:
        sys.exit(f"не запощено замечаний: {len(bad)} — поправить строки и повторить только их")


QUICK_ACTIONS = {
    "changes": "/request_changes",
    "approve": "/approve",
    "unapprove": "/unapprove",
}


def graphql(query):
    data = json.dumps({"query": query}).encode()
    req = urllib.request.Request("https://git.elvees.com/api/graphql", data=data,
                                 headers={"PRIVATE-TOKEN": token(),
                                          "Content-Type": "application/json"}, method="POST")
    with urllib.request.urlopen(req) as resp:
        return json.loads(resp.read().decode())


def review_state(project, iid):
    """Состояние ревью владельца токена. REST его не отдаёт — только GraphQL."""
    status, me = call("GET", "user")
    if status != 200:
        return None, None
    path = project if not str(project).isdigit() else None
    if path is None:
        status, mr = call("GET", mr_path(project, iid))
        path = "/".join(mr["web_url"].split("/-/")[0].split("/")[3:])
    d = graphql('{ project(fullPath: "%s") { mergeRequest(iid: "%s") '
                '{ reviewers { nodes { username mergeRequestInteraction { reviewState } } } } } }'
                % (path, iid))
    nodes = (((d.get("data") or {}).get("project") or {}).get("mergeRequest") or {}) \
        .get("reviewers", {}).get("nodes", [])
    mine = next((n for n in nodes if n["username"] == me["username"]), None)
    return me["username"], (mine or {}).get("mergeRequestInteraction", {}).get("reviewState")


def cmd_mark(a):
    """Отметить своё ревью. Reviewer state в GitLab 18.x меняется ТОЛЬКО quick action
    в теле заметки: REST /reviewers/:id/state и GraphQL-мутации отсутствуют (404)."""
    action = QUICK_ACTIONS.get(a.state)
    if action is None:
        sys.exit(f"state должен быть одним из: {', '.join(QUICK_ACTIONS)}")
    status, d = call("POST", mr_path(a.project, a.iid, "/notes"), {"body": action})
    # Распознанный quick action -> 202 и summary, комментарий при этом НЕ создаётся.
    if status not in (200, 201, 202):
        sys.exit(f"{status}: {d}")
    if isinstance(d, dict) and d.get("id"):
        # Команда не распознана — GitLab создал обычный комментарий с её текстом.
        sys.exit(f"quick action не сработал, создан комментарий {d['id']} — удалить и разобраться")
    for line in (d or {}).get("summary") or []:
        print(line)
    user, state = review_state(a.project, a.iid)
    print(f"reviewer {user}: reviewState={state}")
    if a.state == "changes" and state != "REQUESTED_CHANGES":
        sys.exit("состояние не изменилось — проверить права/версию GitLab")


def cmd_handoff(a):
    """Вернуть MR автору: убрать себя из assignee, добавить автора, остальных не трогать."""
    status, me = call("GET", "user")
    if status != 200:
        sys.exit(f"GET /user -> {status}: {me}")
    status, mr = call("GET", mr_path(a.project, a.iid))
    if status != 200:
        sys.exit(f"{status}: {mr}")

    author = mr["author"]
    current = [x["id"] for x in mr.get("assignees") or []]
    wanted = [x for x in current if x != me["id"]]
    if author["id"] not in wanted:
        wanted.append(author["id"])

    names = {x["id"]: x["username"] for x in (mr.get("assignees") or []) + [author, me]}
    print("было:", [names.get(i, i) for i in current])
    if wanted == current:
        print("менять нечего")
        return
    status, resp = call("PUT", mr_path(a.project, a.iid), {"assignee_ids": wanted})
    if status != 200:
        sys.exit(f"PUT assignee_ids -> {status}: {resp}")
    print("стало:", [x["username"] for x in resp.get("assignees") or []])


def cmd_note(a):
    status, d = call("POST", mr_path(a.project, a.iid, "/notes"), {"body": a.body})
    print(status, d.get("id") if isinstance(d, dict) else d)


def cmd_reply(a):
    status, d = call("POST", mr_path(a.project, a.iid, f"/discussions/{a.disc_id}/notes"),
                     {"body": a.body})
    print(status, d.get("id") if isinstance(d, dict) else d)


def cmd_resolve(a):
    for did in a.disc_id:
        status, d = call("PUT", mr_path(a.project, a.iid, f"/discussions/{did}?resolved=true"))
        print(did, status, "" if status == 200 else d)


def main():
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="cmd", required=True)

    def add(name, fn, *extra):
        s = sub.add_parser(name)
        s.add_argument("project")
        s.add_argument("iid")
        for args, kwargs in extra:
            s.add_argument(*args, **kwargs)
        s.set_defaults(func=fn)
        return s

    add("info", cmd_info)
    add("files", cmd_files)
    add("diff", cmd_diff, (("path",), {"nargs": "?"}))
    add("threads", cmd_threads)
    add("post", cmd_post, (("findings",), {}), (("--dry-run",), {"action": "store_true"}))
    add("mark", cmd_mark, (("state",), {"choices": list(QUICK_ACTIONS)}))
    add("handoff", cmd_handoff)
    add("note", cmd_note, (("body",), {}))
    add("reply", cmd_reply, (("disc_id",), {}), (("body",), {}))
    add("resolve", cmd_resolve, (("disc_id",), {"nargs": "+"}))

    a = p.parse_args()
    a.func(a)


if __name__ == "__main__":
    main()
