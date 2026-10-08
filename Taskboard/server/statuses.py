"""Task statuses. The dispatcher starts 'queued' tasks whose dependencies are done; an agent that cannot finish
marks its task 'blocked'. 'daemon' marks a task whose session is a long-running background watcher instead of a
one-shot job: the dispatcher never starts one and never counts it against its worker slots."""

STATUSES = ("backlog", "queued", "wip", "daemon", "review", "done", "blocked")

LIVE_STATUSES = ("wip", "daemon")

RENAMED_STATUSES = {"running": "wip", "ready": "queued", "failed": "blocked"}


def current_status(name):
    return RENAMED_STATUSES.get(name, name)
