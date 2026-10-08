import json
import sys

LIMIT = 60


def entry(count, short, changelog):
	return {"versionCode": int(count), "versionName": f"0.1.{count} ({short})", "changelog": changelog}


def seed():
	releases = []
	for line in sys.stdin:
		count, short, subject = line.rstrip("\n").split("\t", 2)
		releases.append(entry(count, short, f"- {subject}"))
	print(json.dumps(releases, ensure_ascii=False, indent=1))


def manifest(count, short, url, sha, size, changelog, history_path):
	with open(history_path) as handle:
		history = json.load(handle)
	latest = entry(count, short, changelog)
	history = [item for item in history if item["versionCode"] != latest["versionCode"]]
	history.append(latest)
	history.sort(key=lambda item: item["versionCode"], reverse=True)
	history = history[:LIMIT]
	with open(history_path + ".next", "w") as handle:
		json.dump(history, handle, ensure_ascii=False, indent=1)
	print(json.dumps({
		"versionCode": int(count),
		"versionName": latest["versionName"],
		"apk": url,
		"sha256": sha,
		"size": int(size),
		"changelog": changelog,
		"releases": history,
	}, ensure_ascii=False, indent=1))


if __name__ == "__main__":
	if sys.argv[1] == "seed":
		seed()
	else:
		manifest(*sys.argv[2:])
