#!/bin/bash
set -euo pipefail
HERE=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
REPO=$(cd "$HERE/.." && pwd)
SERVER="auser@ilusha-basic.online"
SSH_PORT=2233
REMOTE_DIR=/srv/stroke2d
PUBLIC_URL=https://ilusha-basic.online/stroke2d
STATE=$HOME/.stroke2d-release/last-published
HISTORY=$HOME/.stroke2d-release/history.json
ABIS="${STROKE2D_ABIS:-arm64-v8a,x86_64}"
export JAVA_HOME=/usr/lib/jvm/java-17-openjdk ANDROID_HOME=$HOME/Android/Sdk

cd "$REPO"
COUNT=$(git rev-list --count HEAD)
HASH=$(git rev-parse --short HEAD)
NAME="stroke2d-$COUNT-$HASH.apk"

(cd android && ./gradlew --no-daemon -q assembleRelease -Pstroke2dAbis="$ABIS" > /tmp/stroke2d-apk-release.log 2>&1)
APK=android/app/build/outputs/apk/release/app-release.apk
SHA=$(sha256sum "$APK" | cut -d' ' -f1)
SIZE=$(stat -c %s "$APK")

PREVIOUS=$(cat "$STATE" 2>/dev/null || true)
if [ -n "$PREVIOUS" ] && git cat-file -e "$PREVIOUS" 2>/dev/null; then
	RANGE="$PREVIOUS..HEAD"
else
	RANGE="-15"
fi
CHANGELOG=$(git log --no-merges --format='- %s' $RANGE | head -40)
[ -n "$CHANGELOG" ] || CHANGELOG="- Maintenance build"

if [ ! -f "$HISTORY" ]; then
	git log --no-merges --format='%H' -60 HEAD~1 | while read -r commit; do
		printf '%s\t%s\t%s\n' "$(git rev-list --count "$commit")" "$(git rev-parse --short "$commit")" "$(git log -1 --format=%s "$commit")"
	done | python3 "$HERE/release-history.py" seed > "$HISTORY"
fi

python3 "$HERE/release-history.py" manifest "$COUNT" "$HASH" "$PUBLIC_URL/$NAME" "$SHA" "$SIZE" "$CHANGELOG" "$HISTORY" > /tmp/stroke2d-latest.json

scp -q -P $SSH_PORT "$APK" "$SERVER:$REMOTE_DIR/$NAME.part"
ssh -p $SSH_PORT "$SERVER" "mv '$REMOTE_DIR/$NAME.part' '$REMOTE_DIR/$NAME'"
scp -q -P $SSH_PORT /tmp/stroke2d-latest.json "$SERVER:$REMOTE_DIR/latest.json.part"
ssh -p $SSH_PORT "$SERVER" "mv '$REMOTE_DIR/latest.json.part' '$REMOTE_DIR/latest.json'; ls -1t $REMOTE_DIR/stroke2d-*.apk | tail -n +6 | xargs -r rm -f"

REMOTE_SHA=$(curl -s -m 600 "$PUBLIC_URL/$NAME" | sha256sum | cut -d' ' -f1)
[ "$REMOTE_SHA" = "$SHA" ] || { echo "published APK checksum mismatch" >&2; exit 1; }
git rev-parse HEAD > "$STATE"
mv "$HISTORY.next" "$HISTORY"
echo "published $NAME versionCode $COUNT sha256 $SHA"
